/** @addtogroup memory
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
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

#include "../simd/read_write.hpp"
#include "../simd/types.hpp"
#include <algorithm>
#include <atomic>
#include <memory>

namespace kfr
{

namespace internal_generic
{

struct memory_statistics
{
    std::atomic_uintptr_t allocation_count   = ATOMIC_VAR_INIT(0);
    std::atomic_uintptr_t allocation_size    = ATOMIC_VAR_INIT(0);
    std::atomic_uintptr_t deallocation_count = ATOMIC_VAR_INIT(0);
    std::atomic_uintptr_t deallocation_size  = ATOMIC_VAR_INIT(0);
};

inline memory_statistics& get_memory_statistics()
{
    static memory_statistics ms;
    return ms;
}

#pragma pack(push, 1)

struct mem_header
{
    u8 offset;
    u8 alignment;
    u8 reserved1;
    u8 reserved2;
    unsigned int references_uint;
    size_t size;

    KFR_MEM_INTRINSIC std::atomic_uint& references()
    {
        return reinterpret_cast<std::atomic_uint&>(references_uint);
    }
}
#ifdef CMT_GNU_ATTRIBUTES
__attribute__((__packed__))
#endif
;

#pragma pack(pop)

inline mem_header* aligned_header(void* ptr) { return ptr_cast<mem_header>(ptr) - 1; }

inline size_t aligned_size(void* ptr) { return aligned_header(ptr)->size; }

inline void* aligned_malloc(size_t size, size_t alignment)
{
    get_memory_statistics().allocation_count++;
    get_memory_statistics().allocation_size += size;
    void* ptr = malloc(size + (alignment - 1) + sizeof(mem_header));
    if (ptr == nullptr)
        return nullptr;
    void* aligned_ptr                         = advance(ptr, sizeof(mem_header));
    aligned_ptr                               = align_up(aligned_ptr, alignment);
    aligned_header(aligned_ptr)->alignment    = static_cast<u8>(alignment > 255 ? 255 : alignment);
    aligned_header(aligned_ptr)->offset       = static_cast<u8>(distance(aligned_ptr, ptr));
    aligned_header(aligned_ptr)->references() = 1;
    aligned_header(aligned_ptr)->size         = size;
    return aligned_ptr;
}

inline void aligned_force_free(void* ptr)
{
    get_memory_statistics().deallocation_count++;
    get_memory_statistics().deallocation_size += aligned_size(ptr);
    free(advance(ptr, -static_cast<ptrdiff_t>(aligned_header(ptr)->offset)));
}

inline void aligned_add_ref(void* ptr) { aligned_header(ptr)->references()++; }

inline void aligned_free(void* ptr)
{
    if (--aligned_header(ptr)->references() == 0)
        aligned_force_free(ptr);
}

inline void aligned_release(void* ptr) { aligned_free(ptr); }
} // namespace internal_generic

/// @brief Allocates aligned memory
template <typename T = void, size_t alignment = platform<>::native_cache_alignment>
KFR_INTRINSIC T* aligned_allocate(size_t size = 1)
{
    T* ptr = static_cast<T*>(CMT_ASSUME_ALIGNED(
        internal_generic::aligned_malloc(std::max(alignment, size * details::elementsize<T>()), alignment),
        alignment));
    return ptr;
}

/// @brief Deallocates aligned memory
template <typename T = void>
KFR_INTRINSIC void aligned_deallocate(T* ptr)
{
    return internal_generic::aligned_free(ptr);
}

namespace internal_generic
{
template <typename T>
struct aligned_deleter
{
    KFR_MEM_INTRINSIC void operator()(T* ptr) const { aligned_deallocate(ptr); }
};
} // namespace internal_generic

template <typename T>
struct autofree
{
    KFR_MEM_INTRINSIC autofree() {}
    explicit KFR_MEM_INTRINSIC autofree(size_t size) : ptr(aligned_allocate<T>(size)) {}
    autofree(const autofree&) = delete;
    autofree& operator=(const autofree&) = delete;
    autofree(autofree&&) CMT_NOEXCEPT    = default;
    autofree& operator=(autofree&&) CMT_NOEXCEPT = default;
    KFR_MEM_INTRINSIC T& operator[](size_t index) CMT_NOEXCEPT { return ptr[index]; }
    KFR_MEM_INTRINSIC const T& operator[](size_t index) const CMT_NOEXCEPT { return ptr[index]; }

    template <typename U = T>
    KFR_MEM_INTRINSIC U* data() CMT_NOEXCEPT
    {
        return ptr_cast<U>(ptr.get());
    }
    template <typename U = T>
    KFR_MEM_INTRINSIC const U* data() const CMT_NOEXCEPT
    {
        return ptr_cast<U>(ptr.get());
    }

    std::unique_ptr<T[], internal_generic::aligned_deleter<T>> ptr;
};

#ifdef KFR_USE_STD_ALLOCATION

template <typename T>
using allocator = std::allocator<T>;

#else

/// @brief Aligned allocator
template <typename T>
struct allocator
{
    using value_type      = T;
    using pointer         = T*;
    using const_pointer   = const T*;
    using reference       = T&;
    using const_reference = const T&;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    template <typename U>
    struct rebind
    {
        using other = allocator<U>;
    };
    constexpr allocator() CMT_NOEXCEPT                 = default;
    constexpr allocator(const allocator&) CMT_NOEXCEPT = default;
    template <typename U>
    constexpr allocator(const allocator<U>&) CMT_NOEXCEPT
    {
    }
    pointer address(reference x) const CMT_NOEXCEPT { return std::addressof(x); }
    const_pointer address(const_reference x) const CMT_NOEXCEPT { return std::addressof(x); }
    pointer allocate(size_type n, std::allocator<void>::const_pointer = 0) const
    {
        pointer result = aligned_allocate<value_type>(n);
        if (!result)
            CMT_THROW(std::bad_alloc());
        return result;
    }
    void deallocate(pointer p, size_type) { aligned_deallocate(p); }
    size_type max_size() const { return std::numeric_limits<size_type>::max() / sizeof(value_type); }
    template <typename U, typename... Args>
    void construct(U* p, Args&&... args)
    {
        ::new (pvoid(p)) U(std::forward<Args>(args)...);
    }
    template <typename U>
    void destroy(U* p)
    {
        p->~U();
    }
};

template <typename T1, typename T2>
constexpr inline bool operator==(const allocator<T1>&, const allocator<T2>&) CMT_NOEXCEPT
{
    return true;
}
template <typename T1, typename T2>
constexpr inline bool operator!=(const allocator<T1>&, const allocator<T2>&) CMT_NOEXCEPT
{
    return false;
}

#endif

struct aligned_new
{
    inline static void* operator new(size_t size) { return aligned_allocate(size); }
    inline static void operator delete(void* ptr) { return aligned_deallocate(ptr); }
};

#define KFR_CLASS_REFCOUNT(cl)                                                                               \
                                                                                                             \
public:                                                                                                      \
    void addref() const { m_refcount++; }                                                                    \
    void release() const                                                                                     \
    {                                                                                                        \
        if (--m_refcount == 0)                                                                               \
        {                                                                                                    \
            delete this;                                                                                     \
        }                                                                                                    \
    }                                                                                                        \
                                                                                                             \
private:                                                                                                     \
    mutable std::atomic_uintptr_t m_refcount = ATOMIC_VAR_INIT(0);

} // namespace kfr
