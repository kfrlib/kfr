/*
  Copyright (C) 2016-2026 Dan Casarin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.
 */
#include <kfr/io/python_plot.hpp>

#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <system_error>

#if !defined(KFR_OS_MOBILE) && !defined(__EMSCRIPTEN__)
#ifdef KFR_OS_WIN
#include <direct.h>
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif
#endif

namespace kfr::internal_generic
{
namespace
{
void report_system_error(const std::string& action, int error)
{
    errorln("Python plotting: unable to ", action, ": ",
            std::error_code(error, std::generic_category()).message());
}

#if !defined(KFR_OS_MOBILE) && !defined(__EMSCRIPTEN__)
#ifdef KFR_OS_WIN
bool run_python(const char* executable, const std::string& filename, bool& not_found)
{
    std::string command = std::string(executable) + " \"" + filename + "\"";
    STARTUPINFOA startup{ sizeof(startup) };
    PROCESS_INFORMATION process{};
    if (!CreateProcessA(nullptr, command.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup,
                        &process))
    {
        const DWORD error = GetLastError();
        not_found         = error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
        if (!not_found)
            errorln("Python plotting: unable to start ", executable, " (error ", error, ")");
        return false;
    }

    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code    = EXIT_FAILURE;
    const bool success = GetExitCodeProcess(process.hProcess, &exit_code) && exit_code == EXIT_SUCCESS;
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    if (!success)
        errorln("Python plotting: ", executable, " exited with code ", exit_code);
    return success;
}
#else
bool run_python(const char* executable, const std::string& filename, bool& not_found)
{
    pid_t process;
    char* const arguments[] = { const_cast<char*>(executable), const_cast<char*>(filename.c_str()), nullptr };
    const int error         = posix_spawnp(&process, executable, nullptr, nullptr, arguments, environ);
    if (error != 0)
    {
        not_found = error == ENOENT;
        if (!not_found)
            report_system_error(executable, error);
        return false;
    }

    int status;
    while (waitpid(process, &status, 0) == -1)
    {
        if (errno != EINTR)
        {
            report_system_error("wait for Python", errno);
            return false;
        }
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS)
        return true;
    errorln("Python plotting: ", executable, " failed");
    return false;
}
#endif
#endif
} // namespace

bool python(const std::string& name, const std::string& code)
{
#if defined(KFR_OS_MOBILE) || defined(__EMSCRIPTEN__)
    errorln("Python plotting is unavailable on this platform");
    return false;
#else
    char directory[4096];
#ifdef KFR_OS_WIN
    if (!_getcwd(directory, sizeof(directory)))
#else
    if (!getcwd(directory, sizeof(directory)))
#endif
    {
        report_system_error("get the current directory", errno);
        return false;
    }

    const std::string filename_string = std::string(directory) + "/" + name + ".py";
    FILE* file                        = std::fopen(filename_string.c_str(), "wb");
    if (!file)
    {
        report_system_error("write " + filename_string, errno);
        return false;
    }
    const bool written = std::fwrite(code.data(), 1, code.size(), file) == code.size();
    const bool closed  = std::fclose(file) == 0;
    if (!written || !closed)
    {
        errorln("Python plotting: unable to write ", filename_string);
        return false;
    }

    for (const char* executable : { "python", "python3" })
    {
        bool not_found = false;
        if (run_python(executable, filename_string, not_found))
            return true;
        if (!not_found)
            return false;
    }
    errorln("Python plotting: neither python nor python3 was found on PATH");
    return false;
#endif
}
} // namespace kfr::internal_generic
