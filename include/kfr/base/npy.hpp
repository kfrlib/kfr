/** @addtogroup tensor
 *  @{
 */
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

#include "tensor.hpp"
#include <complex>
#include <kfr/base/endianness.hpp>
#include <kfr/simd/complex.hpp>

namespace kfr
{

namespace internal_generic
{
template <typename T>
std::string_view npy_format()
{
    using namespace cometa;
    static_assert(is_poweroftwo(sizeof(T)) && is_between(sizeof(T), 1, 16));
    if constexpr (std::is_floating_point_v<T>)
    {
        const std::string_view fmts[] = { "|f1", "<f2", "<f4", "<f8" };
        static_assert(sizeof(T) <= 8);
        return fmts[ilog2(sizeof(T))];
    }
    else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>)
    {
        const std::string_view fmts[] = { "|u1", "<u2", "<u4", "<u8" };
        static_assert(sizeof(T) <= 8);
        return fmts[ilog2(sizeof(T))];
    }
    else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>)
    {
        const std::string_view fmts[] = { "|i1", "<i2", "<i4", "<i8" };
        static_assert(sizeof(T) <= 8);
        return fmts[ilog2(sizeof(T))];
    }
    else if constexpr (is_complex<T>)
    {
        const std::string_view fmts[] = { "|c1", "<c2", "<c4", "<c8", "<c16" };
        static_assert(sizeof(T) <= 16);
        return fmts[ilog2(sizeof(T))];
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        return "|b1";
    }
    else
    {
        static_assert("Cannot save type to python");
        return "";
    }
}
template <index_t Dim>
std::string npy_shape(shape<Dim> sh)
{
    std::string result = "(";
    for (index_t i : sh)
    {
        if (result.size() > 1)
            result += ", ";
        result += std::to_string(i);
    }
    if (Dim == 1)
        result += ",";
    return result + ")";
}

template <typename Fn>
struct npy_writer
{
    Fn&& fn;
    size_t written = 0;

    void operator()(const void* d, size_t size)
    {
        fn(d, size);
        written += size;
    }
    void operator()(std::string_view s) { operator()(s.data(), s.size()); }
    void operator()(std::initializer_list<uint8_t> b) { operator()(&*b.begin(), b.size()); }

    template <typename Num>
    void operator()(cometa::ctype_t<Num>, cometa::identity<Num> n)
    {
        operator()(&n, sizeof(Num));
    }
    template <typename Num>
    void operator()(cometa::ctype_t<Num>, const Num* n, size_t count)
    {
        operator()(n, sizeof(Num) * count);
    }
};

template <typename Fn>
struct npy_reader
{
    Fn&& fn;

    bool operator()(void* d, size_t size) { return fn(d, size); }

    template <typename Num>
    Num operator()(cometa::ctype_t<Num>)
    {
        Num result;
        if (!operator()(&result, sizeof(Num)))
            return {};
        return result;
    }
    template <typename Num>
    bool operator()(cometa::ctype_t<Num>, Num& n)
    {
        return operator()(&n, sizeof(Num));
    }
    template <typename Num>
    bool operator()(cometa::ctype_t<Num>, Num* n, size_t count)
    {
        return operator()(n, sizeof(Num) * count);
    }

    std::string operator()(cometa::ctype_t<char>, size_t count)
    {
        std::string result(count, ' ');
        if (!operator()(cometa::ctype<char>, result.data(), count))
            return {};
        return result;
    }
};

struct npy_header
{
    std::string descr;
    bool fortran_order = false;
    std::vector<index_t> shape;
};

inline bool npy_skip_whitespace(std::string_view& data)
{
    while (!data.empty() && uint8_t(data[0]) <= uint8_t(' '))
        data = data.substr(1);
    return !data.empty();
}
inline bool npy_parse_string(std::string_view& data, std::string& content, char q = '\'')
{
    if (data.empty() || data[0] != q)
        return false;
    data = data.substr(1);
    while (!data.empty() && data[0] != q)
    {
        if (data[0] == '\\')
        {
            data = data.substr(1);
            if (data.empty())
                return false;
        }
        content += data[0];
        data = data.substr(1);
    }
    if (data.empty())
        return false;
    data = data.substr(1);
    return true;
}

inline bool npy_skip_value(std::string_view& data, char close = '}', std::string context = {})
{
    std::string dummy;
    while (!data.empty())
    {
        switch (data[0])
        {
        case '\'':
        case '"':
            if (!npy_parse_string(data, dummy, data[0]))
                return false;
            break;
        case '(':
        case '[':
        case '{':
            context += data[0];
            break;
        case ')':
        case ']':
        case '}':
            if (context.empty())
                return data[0] == close;
            if (context.back() == data[0])
                context.pop_back();
            break;
        case ',':
            if (context.empty())
                return true;
            else
                data = data.substr(1);
            break;
        default:
            data = data.substr(1);
            break;
        }
    }
    return false;
}

inline bool npy_parse_bool(std::string_view& data, bool& content)
{
    if (data.size() >= 5 && data.substr(0, 5) == "False")
    {
        content = false;
        data    = data.substr(5);
        return true;
    }
    if (data.size() >= 4 && data.substr(0, 4) == "True")
    {
        content = true;
        data    = data.substr(4);
        return true;
    }
    return false;
}
inline bool npy_parse_integer(std::string_view& data, index_t& content)
{
    const char* orig = data.data();
    if (data[0] < '0' || data[0] > '9')
        return false;

    while (data[0] >= '0' && data[0] <= '9')
        data = data.substr(1);
    content = std::stoll(std::string(orig, data.data() - orig));
    return true;
}
inline bool npy_parse_shape(std::string_view& data, std::vector<index_t>& content)
{
    if (data[0] != '(')
        return false;
    data = data.substr(1);
    if (!npy_skip_whitespace(data))
        return false;
    for (;;)
    {
        index_t num;
        if (!npy_parse_integer(data, num))
            return false;
        content.push_back(num);
        if (!npy_skip_whitespace(data))
            return false;
        if (data[0] == ')')
        {
            data = data.substr(1);
            return true;
        }
        if (data[0] != ',')
            return false;
        data = data.substr(1);
        if (!npy_skip_whitespace(data))
            return false;
        if (data[0] == ')')
        {
            data = data.substr(1);
            return true;
        }
    }
}

inline bool npy_decode_field(std::string_view& data, npy_header& hdr)
{
    if (!npy_skip_whitespace(data))
        return false;
    std::string name;
    if (!npy_parse_string(data, name))
        return false;
    if (!npy_skip_whitespace(data))
        return false;
    if (data.empty() || data[0] != ':')
        return false;
    data = data.substr(1);
    if (!npy_skip_whitespace(data))
        return false;
    if (data.empty())
        return false;

    if (name == "descr")
    {
        return npy_parse_string(data, hdr.descr);
    }
    else if (name == "fortran_order")
    {
        return npy_parse_bool(data, hdr.fortran_order);
    }
    else if (name == "shape")
    {
        return npy_parse_shape(data, hdr.shape);
    }
    else
    {
        return npy_skip_value(data);
    }

    if (!npy_skip_whitespace(data))
        return false;
    if (data.empty() || data[0] != ',')
        return false;
    data = data.substr(1);
    return true;
}

inline bool npy_decode_dict(std::string_view data, npy_header& hdr)
{
    if (data.size() < 10 || data[0] != '{')
        return false;
    data = data.substr(1);
    while (!data.empty() && data[0] != '}')
    {
        if (!npy_decode_field(data, hdr))
        {
            return false;
        }
        if (data[0] == '}')
            return true;
        if (data.empty() || data[0] != ',')
            return false;
        data = data.substr(1);
        if (!npy_skip_whitespace(data))
            return false;
    }
    return !data.empty() && data[0] == '}';
}

inline std::string_view npy_magic = "\x93NUMPY";

} // namespace internal_generic

template <typename T, index_t Dims, typename Fn>
void save_to_npy(const tensor<T, Dims>& t, Fn&& write_callback)
{
    using namespace internal_generic;

    npy_writer<Fn&> wr{ write_callback };
    wr(internal_generic::npy_magic);
    wr(ctype<uint16_t>, 0x0001);

    std::string header = std::string("{'descr': '") + std::string(npy_format<T>()) +
                         "', 'fortran_order': False, 'shape': " + npy_shape(t.shape()) + ", }";

    std::string_view padding = "                                                               ";

    size_t total_header = cometa::align_up(wr.written + 2 + header.size() + 1, 64);
    header += padding.substr(0, total_header - (wr.written + 2 + header.size() + 1));
    header += '\n';
    uint16_t header_len = header.size();
    wr(cometa::ctype<uint16_t>, header_len);
    wr(header);
    if (t.is_contiguous())
    {
        wr(cometa::ctype<T>, t.data(), t.size());
    }
    else
    {
        tensor<T, Dims> copy = t.copy();
        wr(cometa::ctype<T>, copy.data(), copy.size());
    }
}

enum class npy_decode_result
{
    ok,
    cannot_read,
    invalid_header,
    invalid_type,
    invalid_shape,
};

template <typename T, index_t Dims, typename Fn>
npy_decode_result load_from_npy(tensor<T, Dims>& result, Fn&& read_callback)
{
    using namespace internal_generic;

    npy_reader<Fn&> rd{ read_callback };

    if (std::string s = rd(ctype<char>, internal_generic::npy_magic.size()); s != internal_generic::npy_magic)
        return npy_decode_result::cannot_read;
    uint16_t v;
    if (v = rd(ctype<uint16_t>); v != 1 && v != 2 && v != 3)
        return npy_decode_result::cannot_read;
    uint32_t header_len;
    if (v >= 2)
    {
        if (!rd(ctype<uint32_t>, header_len))
            return npy_decode_result::cannot_read;
    }
    else
    {
        uint16_t header_len16;
        if (!rd(ctype<uint16_t>, header_len16))
            return npy_decode_result::cannot_read;
        header_len = header_len16;
    }
    std::string header = rd(ctype<char>, header_len);

    npy_header hdr;
    if (!npy_decode_dict(header, hdr))
        return npy_decode_result::invalid_header;

    std::string_view tfmt   = npy_format<T>();
    bool convert_endianness = false;
    if (hdr.descr != tfmt)
    {
        if (hdr.descr.size() > 1 && hdr.descr[0] != tfmt[0] && hdr.descr.substr(1) == tfmt.substr(1))
        {
            convert_endianness = hdr.descr[0] == '>';
        }
        else
        {
            return npy_decode_result::invalid_type;
        }
    }
    if (hdr.shape.size() != Dims)
        return npy_decode_result::invalid_shape;

    shape<Dims> sh;
    std::copy(hdr.shape.begin(), hdr.shape.end(), sh.begin());
    if (sh.product() == 0)
        return npy_decode_result::invalid_shape;

    tensor<T, 1> buffer(shape<1>{ sh.product() });

    rd(ctype<T>, buffer.data(), buffer.size());
    if (convert_endianness)
    {
        convert_endianess(buffer.data(), buffer.size());
    }
    if (hdr.fortran_order)
    {
        result = tensor<T, Dims>(buffer.data(), sh, internal_generic::strides_for_shape<Dims, true>(sh),
                                 buffer.finalizer());
    }
    else
    {
        result = tensor<T, Dims>(buffer.data(), sh, internal_generic::strides_for_shape<Dims, false>(sh),
                                 buffer.finalizer());
    }

    return npy_decode_result::ok;
}
} // namespace kfr

namespace cometa
{
template <>
struct representation<kfr::npy_decode_result>
{
    using type = std::string;
    static std::string get(kfr::npy_decode_result value)
    {
        switch (value)
        {
        case kfr::npy_decode_result::ok:
            return "ok";
        case kfr::npy_decode_result::cannot_read:
            return "cannot_read";
        case kfr::npy_decode_result::invalid_header:
            return "invalid_header";
        case kfr::npy_decode_result::invalid_type:
            return "invalid_type";
        case kfr::npy_decode_result::invalid_shape:
            return "invalid_shape";
        default:
            return "(unknown)";
        }
    }
};
} // namespace cometa
