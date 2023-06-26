/*
  Copyright (C) 2016-2023 Dan Cazarin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  KFR is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with KFR.

  If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
  Buying a commercial license is mandatory as soon as you develop commercial activities without
  disclosing the source code of your own applications.
  See https://www.kfrlib.com for details.
 */
#pragma once
#include "cident.h"
#include "cometa/string.hpp"
#include <exception>

namespace kfr
{

class exception : public std::exception
{
public:
    using std::exception::exception;
    exception(std::string str) : m_what(std::move(str)) {}

    const char* what() const noexcept final { return m_what.c_str(); }

private:
    std::string m_what;
};
class logic_error : public exception
{
public:
    using exception::exception;
};
class runtime_error : public exception
{
public:
    using exception::exception;
};

#ifndef KFR_THROW_EXCEPTION
#define KFR_THROW_EXCEPTION(kind, ...)                                                                       \
    do                                                                                                       \
    {                                                                                                        \
        throw ::kfr::CMT_CONCAT(kind, _error)(kfr::as_string(__VA_ARGS__));                                  \
    } while (0)
#endif

#define KFR_PRINT_AND_ABORT(kind, ...)                                                                       \
    do                                                                                                       \
    {                                                                                                        \
        std::string s = kfr::as_string(__VA_ARGS__);                                                         \
        std::fprintf(stderr, "KFR " CMT_STRINGIFY(kind) " error: %s\n", s.c_str());                          \
        std::fflush(stderr);                                                                                 \
        std::abort();                                                                                        \
    } while (0)

#if defined __cpp_exceptions || defined _HAS_EXCEPTIONS || defined _EXCEPTIONS
#define KFR_REPORT_ERROR KFR_THROW_EXCEPTION
#else
#define KFR_REPORT_ERROR KFR_PRINT_AND_ABORT
#endif

#define KFR_CHECK_IMPL(cond, kind, ...)                                                                      \
    do                                                                                                       \
    {                                                                                                        \
        if (CMT_UNLIKELY(!(cond)))                                                                           \
            KFR_REPORT_ERROR(kind, __VA_ARGS__);                                                             \
    } while (0)

#define KFR_REPORT_RUNTIME_ERROR(...) KFR_REPORT_ERROR(runtime, __VA_ARGS__)

#define KFR_REPORT_LOGIC_ERROR(...) KFR_REPORT_ERROR(logic, __VA_ARGS__)

#if !defined(KFR_DISABLE_CHECKS)

#define KFR_RUNTIME_CHECK(cond, ...) KFR_CHECK_IMPL(cond, runtime, __VA_ARGS__)

#define KFR_LOGIC_CHECK(cond, ...) KFR_CHECK_IMPL(cond, logic, __VA_ARGS__)

#else
#define KFR_RUNTIME_CHECK(cond, ...) CMT_NOOP
#define KFR_LOGIC_CHECK(cond, ...) CMT_NOOP

#endif

} // namespace kfr
