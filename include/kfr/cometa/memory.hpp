/** @addtogroup memory
 *  @{
 */
#pragma once

#include "numeric.hpp"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>

namespace cometa
{

namespace details
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
    u16 offset;
    u16 alignment;
    unsigned int references_uint;
    size_t size;

    CMT_MEM_INTRINSIC std::atomic_uint& references()
    {
        return reinterpret_cast<std::atomic_uint&>(references_uint);
    }
}
#ifdef CMT_GNU_ATTRIBUTES
__attribute__((__packed__))
#endif
;

static_assert(sizeof(mem_header) == sizeof(size_t) + 2 * sizeof(u16) + sizeof(unsigned int),
              "Wrong mem_header layout");

#pragma pack(pop)

inline mem_header* aligned_header(void* ptr) { return ptr_cast<mem_header>(ptr) - 1; }

inline size_t aligned_size(void* ptr) { return aligned_header(ptr)->size; }

inline void* aligned_malloc(size_t size, size_t alignment)
{
    if (alignment == 0 || alignment > 32768)
        return nullptr;
    get_memory_statistics().allocation_count++;
    get_memory_statistics().allocation_size += size;
    void* ptr = malloc(size + (alignment - 1) + sizeof(mem_header));
    if (ptr == nullptr)
        return nullptr;
    void* aligned_ptr                         = advance(ptr, sizeof(mem_header));
    aligned_ptr                               = align_up(aligned_ptr, alignment);
    aligned_header(aligned_ptr)->alignment    = static_cast<u16>(alignment);
    aligned_header(aligned_ptr)->offset       = static_cast<u16>(distance(aligned_ptr, ptr));
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

inline void* aligned_reallocate(void* ptr, size_t new_size, size_t alignment)
{
    if (ptr)
    {
        if (new_size)
        {
            void* new_ptr   = aligned_malloc(new_size, alignment);
            size_t old_size = aligned_size(ptr);
            memcpy(new_ptr, ptr, std::min(old_size, new_size));
            aligned_release(ptr);
            return new_ptr;
        }
        else
        {
            aligned_release(ptr);
            return nullptr;
        }
    }
    else
    {
        if (new_size)
        {
            return details::aligned_malloc(new_size, alignment);
        }
        else
        {
            return nullptr; // do nothing
        }
    }
}
} // namespace details

constexpr inline size_t default_memory_alignment = 64;

/// @brief Allocates aligned memory
template <typename T = void, size_t alignment = default_memory_alignment>
CMT_INTRINSIC T* aligned_allocate(size_t size = 1)
{
    T* ptr = static_cast<T*>(CMT_ASSUME_ALIGNED(
        details::aligned_malloc(std::max(alignment, size * details::elementsize<T>()), alignment),
        alignment));
    return ptr;
}
/// @brief Allocates aligned memory
template <typename T = void>
CMT_INTRINSIC T* aligned_allocate(size_t size, size_t alignment)
{
    T* ptr = static_cast<T*>(CMT_ASSUME_ALIGNED(
        details::aligned_malloc(std::max(alignment, size * details::elementsize<T>()), alignment),
        alignment));
    return ptr;
}

/// @brief Deallocates aligned memory
template <typename T = void>
CMT_INTRINSIC void aligned_deallocate(T* ptr)
{
    return details::aligned_free(ptr);
}

namespace details
{
template <typename T>
struct aligned_deleter
{
    CMT_MEM_INTRINSIC void operator()(T* ptr) const { aligned_deallocate(ptr); }
};
} // namespace details

template <typename T>
struct autofree
{
    CMT_MEM_INTRINSIC autofree() {}
    explicit CMT_MEM_INTRINSIC autofree(size_t size) : ptr(aligned_allocate<T>(size)) {}
    autofree(const autofree&)                    = delete;
    autofree& operator=(const autofree&)         = delete;
    autofree(autofree&&) CMT_NOEXCEPT            = default;
    autofree& operator=(autofree&&) CMT_NOEXCEPT = default;
    CMT_MEM_INTRINSIC T& operator[](size_t index) CMT_NOEXCEPT { return ptr[index]; }
    CMT_MEM_INTRINSIC const T& operator[](size_t index) const CMT_NOEXCEPT { return ptr[index]; }

    template <typename U = T>
    CMT_MEM_INTRINSIC U* data() CMT_NOEXCEPT
    {
        return ptr_cast<U>(ptr.get());
    }
    template <typename U = T>
    CMT_MEM_INTRINSIC const U* data() const CMT_NOEXCEPT
    {
        return ptr_cast<U>(ptr.get());
    }

    std::unique_ptr<T[], details::aligned_deleter<T>> ptr;
};

#ifdef KFR_USE_STD_ALLOCATION

template <typename T>
using data_allocator = std::allocator<T>;

#else

/// @brief Aligned allocator
template <typename T>
struct data_allocator
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
        using other = data_allocator<U>;
    };
    constexpr data_allocator() CMT_NOEXCEPT                      = default;
    constexpr data_allocator(const data_allocator&) CMT_NOEXCEPT = default;
    template <typename U>
    constexpr data_allocator(const data_allocator<U>&) CMT_NOEXCEPT
    {
    }
    pointer allocate(size_type n) const
    {
        pointer result = aligned_allocate<value_type>(n);
        if (!result)
            CMT_THROW(std::bad_alloc());
        return result;
    }
    void deallocate(pointer p, size_type) { aligned_deallocate(p); }
};

template <typename T1, typename T2>
constexpr inline bool operator==(const data_allocator<T1>&, const data_allocator<T2>&) CMT_NOEXCEPT
{
    return true;
}
template <typename T1, typename T2>
constexpr inline bool operator!=(const data_allocator<T1>&, const data_allocator<T2>&) CMT_NOEXCEPT
{
    return false;
}

#endif

struct aligned_new
{
    inline static void* operator new(size_t size) noexcept { return aligned_allocate(size); }
    inline static void operator delete(void* ptr) noexcept { return aligned_deallocate(ptr); }

#ifdef __cpp_aligned_new
    inline static void* operator new(size_t size, std::align_val_t al) noexcept
    {
        return details::aligned_malloc(size,
                                       std::max(size_t(default_memory_alignment), static_cast<size_t>(al)));
    }
    inline static void operator delete(void* ptr, std::align_val_t al) noexcept
    {
        return details::aligned_free(ptr);
    }
#endif
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

namespace details
{

template <typename T, typename Fn>
CMT_ALWAYS_INLINE static void call_with_temp_heap(size_t temp_size, Fn&& fn)
{
    autofree<T> temp(temp_size);
    fn(temp.data());
}

template <size_t stack_size, typename T, typename Fn>
CMT_NOINLINE static void call_with_temp_stack(size_t temp_size, Fn&& fn)
{
    alignas(default_memory_alignment) T temp[stack_size];
    fn(&temp[0]);
}

} // namespace details

template <size_t stack_size = 4096, typename T = u8, typename Fn>
CMT_ALWAYS_INLINE static void call_with_temp(size_t temp_size, Fn&& fn)
{
    if (temp_size <= stack_size)
        return details::call_with_temp_stack<stack_size, T>(temp_size, std::forward<Fn>(fn));
    return details::call_with_temp_heap<T>(temp_size, std::forward<Fn>(fn));
}
} // namespace cometa
