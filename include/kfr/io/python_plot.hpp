/*
  Copyright (C) 2016-2026 Dan Casarin (https://www.kfrlib.com)
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
#include "../meta/array.hpp"
#include "../meta/string.hpp"

namespace kfr
{
namespace internal_generic
{
/// @brief Write and execute a generated Python plotting script.
/// @return @c true when the script completed successfully.
bool python(const std::string& name, const std::string& code);

inline std::string python_string(const std::string& value)
{
    std::string result = "'";
    for (char ch : value)
    {
        switch (ch)
        {
        case '\\':
            result += "\\\\";
            break;
        case '\'':
            result += "\\'";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            result += ch;
            break;
        }
    }
    return result + "'";
}

template <typename T>
inline T flush_to_zero(T value)
{
    if constexpr (std::is_floating_point_v<T>)
        return std::isfinite(value) ? value : 0;
    else
        return static_cast<double>(value);
}
} // namespace internal_generic

inline std::string concat_args() { return {}; }

template <typename... Ts>
std::string concat_args(const std::string& left, const Ts&... rest)
{
    const std::string right = concat_args(rest...);
    return left.empty() ? right : right.empty() ? left : left + ", " + right;
}

inline std::string python_prologue()
{
    return "#!/usr/bin/env python\n"
           "import sys\n"
           "import os\n"
           "script_dir = os.path.dirname(os.path.abspath(__file__))\n"
           "for level in range(4):\n"
           "    path = os.path.join(script_dir, *(['..'] * level), 'dspplot', 'dspplot')\n"
           "    if os.path.isdir(path):\n"
           "        sys.path.append(path)\n"
           "import dspplotting as dspplot\n\n";
}

template <int = 0>
void plot_show(const std::string& name, const std::string& wavfile, const std::string& options = "")
{
    print(name, "...");
    std::string ss;
    ss += python_prologue() + "dspplot.plot(" +
          concat_args(internal_generic::python_string(wavfile), options) + ")\n";

    print(internal_generic::python(name, ss) ? "done\n" : "failed\n");
}

template <int = 0>
void plot_show(const std::string& name, const char* x, const std::string& options = "")
{
    plot_show(name, std::string(x), options);
}

/// @brief Plot data using python
template <typename T>
void plot_show(const std::string& name, const T& x, const std::string& options = "")
{
    print(name, "...");
    auto array = make_array_ref(x);
    std::string ss;
    ss += python_prologue() + "data = [\n";
    for (size_t i = 0; i < array.size(); i++)
        ss += as_string(kfr::fmt<'g', 20, 17>(internal_generic::flush_to_zero(array[i])), ",\n");
    ss += "]\n";

    ss += "dspplot.plot(" + concat_args("data", options) + ")\n";

    print(internal_generic::python(name, ss) ? "done\n" : "failed\n");
}

/// @brief Plot data using python and save to file
template <typename T>
void plot_save(const std::string& name, const T& x, const std::string& options = "")
{
    plot_show(name, x,
              concat_args(options, "file=" + internal_generic::python_string("../svg/" + name + ".svg")));
}

template <typename T1, typename T2>
void perfplot_show(const std::string& name, T1&& data, T2&& labels, const std::string& options = "")
{
    print(name, "...");
    auto array        = make_array_ref(std::forward<T1>(data));
    auto labels_array = make_array_ref(std::forward<T2>(labels));
    std::string ss;
    ss += python_prologue();
    ss += "data = [\n";
    for (size_t i = 0; i < array.size(); i++)
    {
        auto subarray = make_array_ref(array[i]);
        ss += "[\n";
        for (size_t i = 0; i < subarray.size(); i++)
            ss += as_string("    ", kfr::fmt<'g', 20, 17>(subarray[i]), ",\n");
        ss += "],";
    }
    ss += "]\n";

    ss += "labels = [\n";
    for (size_t i = 0; i < labels_array.size(); i++)
    {
        const std::string label = labels_array[i];
        ss += "    " + internal_generic::python_string(label) + ",";
    }
    ss += "]\n";

    ss += "dspplot.perfplot(" + concat_args("data, labels", options) + ")\n";

    print(internal_generic::python(name, ss) ? "done\n" : "failed\n");
}

template <typename T1, typename T2>
void perfplot_save(const std::string& name, T1&& data, T2&& labels, const std::string& options = "")
{
    perfplot_show(
        name, std::forward<T1>(data), std::forward<T2>(labels),
        concat_args(options, "file=" + internal_generic::python_string("../perf/" + name + ".svg")));
}
} // namespace kfr
