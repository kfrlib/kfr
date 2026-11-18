/*
 * C utilities
 * 
 * Copyright (c) 2017 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef _CUTILS_H
#define _CUTILS_H

#include <inttypes.h>
#include <string.h>

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#define likely(x)       (!!(x))
#define unlikely(x)     (!!(x))
#define force_inline __forceinline
#define no_inline __declspec(noinline)
#define __maybe_unused
#define printf_format(fmt, args)
#else
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#define force_inline inline __attribute__((always_inline))
#define no_inline __attribute__((noinline))
#define __maybe_unused __attribute__((unused))
#define printf_format(fmt, args) __attribute__((format(printf, fmt, args)))
#endif

#define xglue(x, y) x ## y
#define glue(x, y) xglue(x, y)
#define stringify(s)    tostring(s)
#define tostring(s)     #s

#ifndef offsetof
#define offsetof(type, field) ((size_t) &((type *)0)->field)
#endif
#define countof(x) (sizeof(x) / sizeof(x[0]))

/* return the pointer of type 'type *' containing 'ptr' as field 'member' */
#define container_of(ptr, type, member) ((type *)((uint8_t *)(ptr) - offsetof(type, member)))

typedef int BOOL;

#ifndef FALSE
enum {
    FALSE = 0,
    TRUE = 1,
};
#endif

void pstrcpy(char *buf, int buf_size, const char *str);
char *pstrcat(char *buf, int buf_size, const char *s);
int strstart(const char *str, const char *val, const char **ptr);

static inline int max_int(int a, int b)
{
    if (a > b)
        return a;
    else
        return b;
}

static inline int min_int(int a, int b)
{
    if (a < b)
        return a;
    else
        return b;
}

static inline int clamp_int(int val, int min_val, int max_val)
{
    if (val < min_val)
        return min_val;
    else if (val > max_val)
        return max_val;
    else
        return val;
}

/* WARNING: undefined if a = 0 */
static inline int clz32(unsigned int a)
{
#if defined(_MSC_VER) && !defined(__clang__)
    unsigned long index = 0;
    _BitScanReverse(&index, a);
    return 31 - (int)index;
#else
    return __builtin_clz(a);
#endif
}

/* WARNING: undefined if a = 0 */
static inline int clz64(uint64_t a)
{
    uint32_t hi = (uint32_t)(a >> 32);
    return hi ? clz32(hi) : 32 + clz32((uint32_t)a);
}

/* WARNING: undefined if a = 0 */
static inline int ctz32(unsigned int a)
{
#if defined(_MSC_VER) && !defined(__clang__)
    unsigned long index = 0;
    _BitScanForward(&index, a);
    return (int)index;
#else
    return __builtin_ctz(a);
#endif
}

/* WARNING: undefined if a = 0 */
static inline int ctz64(uint64_t a)
{
    uint32_t lo = (uint32_t)a;
    return lo ? ctz32(lo) : 32 + ctz32((uint32_t)(a >> 32));
}

static inline uint32_t get_u32(const uint8_t *tab)
{
    uint32_t val;
    memcpy(&val, tab, sizeof(val));
    return val;
}

static inline void put_u32(uint8_t *tab, uint32_t val)
{
    memcpy(tab, &val, sizeof(val));
}

static inline uint32_t get_u16(const uint8_t *tab)
{
    uint16_t val;
    memcpy(&val, tab, sizeof(val));
    return val;
}

static inline void put_u16(uint8_t *tab, uint16_t val)
{
    memcpy(tab, &val, sizeof(val));
}

typedef void *DynBufReallocFunc(void *opaque, void *ptr, size_t size);

typedef struct {
    uint8_t *buf;
    size_t size;
    size_t allocated_size;
    BOOL error; /* true if a memory allocation error occurred */
    DynBufReallocFunc *realloc_func;
    void *opaque; /* for realloc_func */
} DynBuf;

void dbuf_init(DynBuf *s);
void dbuf_init2(DynBuf *s, void *opaque, DynBufReallocFunc *realloc_func);
int dbuf_realloc(DynBuf *s, size_t new_size);
int dbuf_write(DynBuf *s, size_t offset, const uint8_t *data, size_t len);
int dbuf_put(DynBuf *s, const uint8_t *data, size_t len);
int dbuf_putc(DynBuf *s, uint8_t c);
int dbuf_putstr(DynBuf *s, const char *str);
static inline int dbuf_put_u32(DynBuf *s, uint32_t val)
{
    return dbuf_put(s, (uint8_t *)&val, 4);
}
static inline int dbuf_put_u16(DynBuf *s, uint16_t val)
{
    return dbuf_put(s, (uint8_t *)&val, 2);
}
int printf_format(2, 3) dbuf_printf(DynBuf *s, const char *fmt, ...);
void dbuf_free(DynBuf *s);
static inline BOOL dbuf_error(DynBuf *s) {
    return s->error;
}


#define UTF8_CHAR_LEN_MAX 4

int unicode_to_utf8(uint8_t *buf, unsigned int c);
int unicode_from_utf8(const uint8_t *p, int max_len, const uint8_t **pp);

#endif
