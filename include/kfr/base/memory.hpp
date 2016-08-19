/** @addtogroup utility
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

#include "read_write.hpp"
#include "types.hpp"
#include <atomic>
#include <memory>

namespace kfr
{

namespace internal
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

struct mem_header
{
    u8 offset;
    u8 alignment;
    u8 reserved1;
    u8 reserved2;
    size_t size;
} __attribute__((__packed__));

inline mem_header* aligned_header(void* ptr) { return ptr_cast<mem_header>(ptr) - 1; }

inline size_t aligned_size(void* ptr) { return aligned_header(ptr)->size; }

inline void* aligned_malloc(size_t size, size_t alignment)
{
    get_memory_statistics().allocation_count++;
    get_memory_statistics().allocation_size += size;
    void* ptr = malloc(size + (alignment - 1) + sizeof(mem_header));
    if (ptr == nullptr)
        return nullptr;
    void* aligned_ptr                      = advance(ptr, sizeof(mem_header));
    aligned_ptr                            = align_up(aligned_ptr, alignment);
    aligned_header(aligned_ptr)->alignment = static_cast<u8>(alignment > 255 ? 255 : alignment);
    aligned_header(aligned_ptr)->offset    = static_cast<u8>(distance(aligned_ptr, ptr));
    aligned_header(aligned_ptr)->size      = size;
    return aligned_ptr;
}
inline void aligned_free(void* ptr)
{
    get_memory_statistics().deallocation_count++;
    get_memory_statistics().deallocation_size += aligned_size(ptr);
    free(advance(ptr, -static_cast<ptrdiff_t>(aligned_header(ptr)->offset)));
}
}

template <typename T = void, size_t alignment = native_cache_alignment>
CMT_INLINE T* aligned_allocate(size_t size = 1)
{
    T* ptr = static_cast<T*>(CMT_ASSUME_ALIGNED(
        internal::aligned_malloc(std::max(alignment, size * details::elementsize<T>), alignment), alignment));
    return ptr;
}

template <typename T = void>
CMT_INLINE void aligned_deallocate(T* ptr)
{
    return internal::aligned_free(ptr);
}

namespace internal
{
template <typename T>
struct aligned_deleter
{
    CMT_INLINE void operator()(T* ptr) const { aligned_deallocate(ptr); }
};
}

template <typename T>
struct autofree
{
    CMT_INLINE autofree() {}
    explicit CMT_INLINE autofree(size_t size) : ptr(aligned_allocate<T>(size)) {}
    autofree(const autofree&) = delete;
    autofree& operator=(const autofree&) = delete;
    autofree(autofree&&) noexcept        = default;
    autofree& operator=(autofree&&) noexcept = default;
    CMT_INLINE T& operator[](size_t index) noexcept { return ptr[index]; }
    CMT_INLINE const T& operator[](size_t index) const noexcept { return ptr[index]; }

    template <typename U = T>
    CMT_INLINE U* data() noexcept
    {
        return ptr_cast<U>(ptr.get());
    }
    template <typename U = T>
    CMT_INLINE const U* data() const noexcept
    {
        return ptr_cast<U>(ptr.get());
    }

    std::unique_ptr<T[], internal::aligned_deleter<T>> ptr;
};

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
    constexpr allocator() noexcept                 = default;
    constexpr allocator(const allocator&) noexcept = default;
    template <typename U>
    constexpr allocator(const allocator<U>&) noexcept
    {
    }
    pointer address(reference x) const noexcept { return std::addressof(x); }
    const_pointer address(const_reference x) const noexcept { return std::addressof(x); }
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
constexpr inline bool operator==(const allocator<T1>&, const allocator<T2>&) noexcept
{
    return true;
}
template <typename T1, typename T2>
constexpr inline bool operator!=(const allocator<T1>&, const allocator<T2>&) noexcept
{
    return false;
}

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
}
