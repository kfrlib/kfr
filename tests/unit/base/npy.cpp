/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base.hpp>
#include <kfr/base/npy.hpp>
#include <kfr/io/file.hpp>
#include <kfr/io/tostring.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

template <index_t Dims>
static std::string shape_str(shape<Dims> size)
{
    if constexpr (Dims == 1)
        return std::to_string(size.back());
    else
        return shape_str(size.remove_back()) + "x" + std::to_string(size.back());
}

template <typename T>
static std::string py_type()
{
    if constexpr (std::is_same_v<T, bool>)
        return "bool";
    else if constexpr (std::is_floating_point_v<T>)
        return "float" + std::to_string(sizeof(T) * 8);
    else if constexpr (is_complex<T>)
        return "complex" + std::to_string(sizeof(T) * 8);
    else if constexpr (std::is_unsigned_v<T>)
        return "uint" + std::to_string(sizeof(T) * 8);
    else if constexpr (std::is_signed_v<T>)
        return "int" + std::to_string(sizeof(T) * 8);
    else
        return "?";
}

template <typename T, index_t Dims>
static std::string filename(shape<Dims> size, int ver, char order, char endianness)
{
    return shape_str(size) + "-" + py_type<T>() + "-v" + std::to_string(ver) + "-" + order + "-" +
           endianness + "e.npy";
}

template <typename T, index_t Dims>
static void test_npy_t(shape<Dims> size)
{
    for (int ver : { 1, 2 })
    {
        for (char order : { 'c', 'f' })
        {
            for (char endianness : { 'l', 'b' })
            {
                std::string filepath =
                    KFR_SRC_DIR "/tests/npy/" + filename<T, Dims>(size, ver, order, endianness);
                testo::scope s(filepath);
                std::shared_ptr<file_reader<void>> f = open_file_for_reading(filepath);
                CHECK(f != nullptr);
                if (!f)
                    continue;

                tensor<T, Dims> data;
                tensor<T, Dims> reference(size);
                reference.flatten() = counter<subtype<T>>();
                npy_decode_result result =
                    load_from_npy(data, [f](void* d, size_t size) { return f->read(d, size) == size; });
                CHECK(result == npy_decode_result::ok);
                CHECK(data.shape() == size);
                CHECK(data == reference);

                if (endianness == 'l' && order == 'c' && ver == 1)
                {
                    f->seek(0, seek_origin::end);
                    std::vector<uint8_t> reference_binary;
                    reference_binary.resize(f->tell());
                    f->seek(0, seek_origin::begin);
                    f->read(reference_binary.data(), reference_binary.size());

                    std::vector<uint8_t> binary;
                    save_to_npy(reference,
                                [&](const void* d, size_t sz)
                                {
                                    binary.insert(binary.end(), reinterpret_cast<const uint8_t*>(d),
                                                  reinterpret_cast<const uint8_t*>(d) + sz);
                                });
                    CHECK(binary.size() == reference_binary.size());
                    CHECK(std::equal(binary.begin(), binary.end(), reference_binary.begin(),
                                     reference_binary.end()));
                }
            }
        }
    }
}

template <index_t Dims>
static void test_npy(shape<Dims> size)
{
    test_npy_t<u8>(size);
    test_npy_t<u16>(size);
    test_npy_t<u32>(size);
    test_npy_t<u64>(size);
    test_npy_t<i8>(size);
    test_npy_t<i16>(size);
    test_npy_t<i32>(size);
    test_npy_t<i64>(size);
    test_npy_t<f32>(size);
    test_npy_t<f64>(size);
    test_npy_t<c32>(size);
    test_npy_t<c64>(size);
    // test_npy_t<bool>(size);
}

TEST(npy_all)
{
    test_npy(shape{ 110 });
    test_npy(shape{ 10, 11 });
    test_npy(shape{ 2, 5, 11 });
}

} // namespace CMT_ARCH_NAME
} // namespace kfr
