/*
 * Copyright (c) 2015-2020, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of Intel Corporation nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/** \file
 * \brief SIMD types and primitive operations.
 */

#ifndef ARCH_ARM_SIMD_UTILS_H
#define ARCH_ARM_SIMD_UTILS_H

#include <stdio.h>

#include "ue2common.h"
#include "util/simd_types.h"
#include "util/unaligned.h"
#include "util/intrinsics.h"

#include <string.h> // for memcpy

static really_inline m128 ones128(void) {
    return (m128) vdupq_n_s8(0xFF);
}

static really_inline m128 zeroes128(void) {
    return (m128) vdupq_n_s32(0);
}

/** \brief Bitwise not for m128*/
static really_inline m128 not128(m128 a) {
    return (m128) vmvnq_s32(a);
}

/** \brief Return 1 if a and b are different otherwise 0 */
static really_inline int diff128(m128 a, m128 b) {
    #ifdef __aarch64__
    int res = vaddvq_s8((int8x16_t) vceqq_s32(a, b));
    #else
    const int16x8_t x2 = vpaddlq_s8((int8x16_t) vceqq_s32(a, b));
    const int32x4_t x4 = vpaddlq_s16(x2);
    const int64x2_t x8 = vpaddlq_s32(x4);
    int res = vgetq_lane_s64(x8, 0) + vgetq_lane_s64(x8, 1);
    #endif
    return (-16 != res);
}

static really_inline int isnonzero128(m128 a) {
    return !!diff128(a, zeroes128());
}

/**
 * "Rich" version of diff128(). Takes two vectors a and b and returns a 4-bit
 * mask indicating which 32-bit words contain differences.
 */
static really_inline u32 diffrich128(m128 a, m128 b) {
    static const uint32x4_t movemask = { 1, 2, 4, 8 };
    #ifdef __aarch64__
    return vaddvq_u32(vandq_u32(vmvnq_s32(vceqq_s32((int32x4_t)a, (int32x4_t)b)), movemask));
    #else
    const uint64x2_t x2 = vpaddlq_u32(vandq_u32(vmvnq_s32(vceqq_s32((int32x4_t)a, (int32x4_t)b)), movemask));
    return vgetq_lane_u64(x2, 0) + vgetq_lane_u64(x2, 1);
    #endif
}

/**
 * "Rich" version of diff128(), 64-bit variant. Takes two vectors a and b and
 * returns a 4-bit mask indicating which 64-bit words contain differences.
 */
static really_inline u32 diffrich64_128(m128 a, m128 b) {
    static const uint64x2_t movemask = { 1, 4 };
    #ifdef __aarch64__
    return vaddvq_u64(vandq_u64(vmvnq_s32(vceqq_s64((int64x2_t)a, (int64x2_t)b)), movemask));
    #else
    uint32x4_t cmp = vceqq_u32(vreinterpretq_u32_s64(a), vreinterpretq_u32_s64(b));
    uint32x4_t swapped = vrev64q_u32(cmp);
    uint32x4_t cmpeq_epi64 = vreinterpretq_s64_u32(vandq_u32(cmp, swapped));
    return vgetq_lane_u64(vandq_u64(vmvnq_s32(cmpeq_epi64), movemask), 0) + vgetq_lane_u64(vandq_u64(vmvnq_s32(cmpeq_epi64), movemask), 1);
    #endif
}

static really_really_inline
m128 add_2x64(m128 a, m128 b) {
    return (m128) vaddq_u64((int64x2_t)a, (int64x2_t)b);
}

static really_really_inline
m128 sub_2x64(m128 a, m128 b) {
    return (m128) vsubq_u64((int64x2_t)a, (int64x2_t)b);
}

static really_really_inline
m128 lshift_m128(m128 a, unsigned b) {
    return (m128) vshlq_n_s32((int64x2_t)a, b);
}

static really_really_inline
m128 rshift_m128(m128 a, unsigned b) {
    return (m128) vshrq_n_s32((int64x2_t)a, b);
}

static really_really_inline
m128 lshift64_m128(m128 a, unsigned b) {
    #ifdef __aarch64__
    return (m128) vshlq_n_s64((int64x2_t)a, b);
    #else
    return (m128) vshlq_s64((int64x2_t)(a), vdupq_n_s64(b));
    #endif
}

static really_really_inline
m128 rshift64_m128(m128 a, unsigned b) {
    return (m128) vshrq_n_s64((int64x2_t)a, b);
}

static really_inline m128 eq128(m128 a, m128 b) {
    return (m128) vceqq_s8((int8x16_t)a, (int8x16_t)b);
}

static really_inline m128 eq64_m128(m128 a, m128 b) {
    #ifdef __aarch64__
    return (m128) vceqq_u64((int64x2_t)a, (int64x2_t)b);
    #else
    uint32x4_t cmp = vceqq_u32(vreinterpretq_u32_s64(a), vreinterpretq_u32_s64(b));
    uint32x4_t swapped = vrev64q_u32(cmp);
    return vreinterpretq_s64_u32(vandq_u32(cmp, swapped));
    #endif
}


static really_inline u32 movemask128(m128 a) {
    static const uint8x16_t powers = { 1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128 };

    // Compute the mask from the input
    uint64x2_t mask  = vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(vandq_u8((uint8x16_t)a, powers))));
    uint64x2_t mask1 = (m128)vextq_s8(mask, (int8x16_t) zeroes128(), 7);
    mask = vorrq_u8((uint8x16_t) mask, (uint8x16_t) mask1);

    // Get the resulting bytes
    uint16_t output;
    vst1q_lane_u16((uint16_t*)&output, (uint16x8_t)mask, 0);
    return output;
}

static really_inline m128 set1_16x8(u8 c) {
    return (m128) vdupq_n_u8(c);
}

static really_inline m128 set1_4x32(u32 c) {
    return (m128) vdupq_n_u32(c);
}

static really_inline m128 set1_2x64(u64a c) {
    return (m128) vdupq_n_u64(c);
}

static really_inline u32 movd(const m128 in) {
    return vgetq_lane_u32((uint32x4_t) in, 0);
}

static really_inline u64a movq(const m128 in) {
    return vgetq_lane_u64((uint64x2_t) in, 0);
}

/* another form of movq */
static really_inline
m128 load_m128_from_u64a(const u64a *p) {
    return (m128) vsetq_lane_u64(*p, (uint64x2_t) zeroes128(), 0);
}

static really_inline u32 extract32from128(const m128 in, unsigned imm) {
#if defined(HS_OPTIMIZE)
    return vgetq_lane_u32((uint32x4_t) in, imm);
#else
    switch (imm) {
    case 0:
        return vgetq_lane_u32((uint32x4_t) in, 0);
	break;
    case 1:
        return vgetq_lane_u32((uint32x4_t) in, 1);
	break;
    case 2:
        return vgetq_lane_u32((uint32x4_t) in, 2);
	break;
    case 3:
        return vgetq_lane_u32((uint32x4_t) in, 3);
	break;
    default:
	return 0;
	break;
    }
#endif
}

static really_inline u64a extract64from128(const m128 in, unsigned imm) {
#if defined(HS_OPTIMIZE)
    return vgetq_lane_u64((uint64x2_t) in, imm);
#else
    switch (imm) {
    case 0:
        return vgetq_lane_u64((uint64x2_t) in, 0);
        break;
    case 1:
        return vgetq_lane_u64((uint64x2_t) in, 1);
        break;
    default:
        return 0;
        break;
    }
#endif
}

static really_inline m128 low64from128(const m128 in) {
    return vcombine_u64(vget_low_u64(in), vdup_n_u64(0));
}

static really_inline m128 high64from128(const m128 in) {
    return vcombine_u64(vget_high_u64(in), vdup_n_u64(0));
}

static really_inline m128 add128(m128 a, m128 b) {
    return (m128) vaddq_u64((uint64x2_t)a, (uint64x2_t)b);
}

static really_inline m128 and128(m128 a, m128 b) {
    return (m128) vandq_s8((int8x16_t)a, (int8x16_t)b);
}

static really_inline m128 xor128(m128 a, m128 b) {
    return (m128) veorq_s8((int8x16_t)a, (int8x16_t)b);
}

static really_inline m128 or128(m128 a, m128 b) {
    return (m128) vorrq_s8((int8x16_t)a, (int8x16_t)b);
}

static really_inline m128 andnot128(m128 a, m128 b) {
    return (m128) (m128) vandq_s8( vmvnq_s8(a), b);
}

// aligned load
static really_inline m128 load128(const void *ptr) {
    assert(ISALIGNED_N(ptr, alignof(m128)));
    return (m128) vld1q_s32((const int32_t *)ptr);
}

// aligned store
static really_inline void store128(void *ptr, m128 a) {
    assert(ISALIGNED_N(ptr, alignof(m128)));
    vst1q_s32((int32_t *)ptr, a);
}

// unaligned load
static really_inline m128 loadu128(const void *ptr) {
    return (m128) vld1q_s32((const int32_t *)ptr);
}

// unaligned store
static really_inline void storeu128(void *ptr, m128 a) {
    vst1q_s32((int32_t *)ptr, a);
}

// packed unaligned store of first N bytes
static really_inline
void storebytes128(void *ptr, m128 a, unsigned int n) {
    assert(n <= sizeof(a));
    memcpy(ptr, &a, n);
}

// packed unaligned load of first N bytes, pad with zero
static really_inline
m128 loadbytes128(const void *ptr, unsigned int n) {
    m128 a = zeroes128();
    assert(n <= sizeof(a));
    memcpy(&a, ptr, n);
    return a;
}


#define CASE_ALIGN_VECTORS(a, b, offset)  case offset: return (m128)vextq_s8((int8x16_t)(a), (int8x16_t)(b), (offset)); break;

static really_really_inline
m128 palignr_imm(m128 r, m128 l, int offset) {
    switch (offset) {
    case 0: return l; break;
    CASE_ALIGN_VECTORS(l, r, 1);
    CASE_ALIGN_VECTORS(l, r, 2);
    CASE_ALIGN_VECTORS(l, r, 3);
    CASE_ALIGN_VECTORS(l, r, 4);
    CASE_ALIGN_VECTORS(l, r, 5);
    CASE_ALIGN_VECTORS(l, r, 6);
    CASE_ALIGN_VECTORS(l, r, 7);
    CASE_ALIGN_VECTORS(l, r, 8);
    CASE_ALIGN_VECTORS(l, r, 9);
    CASE_ALIGN_VECTORS(l, r, 10);
    CASE_ALIGN_VECTORS(l, r, 11);
    CASE_ALIGN_VECTORS(l, r, 12);
    CASE_ALIGN_VECTORS(l, r, 13);
    CASE_ALIGN_VECTORS(l, r, 14);
    CASE_ALIGN_VECTORS(l, r, 15);
    case 16: return r; break;
    default:
	return zeroes128();
	break;
    }
}

static really_really_inline
m128 palignr(m128 r, m128 l, int offset) {
#if defined(HS_OPTIMIZE)
    return (m128)vextq_s8((int8x16_t)l, (int8x16_t)r, offset);
#else
    return palignr_imm(r, l, offset);
#endif
}
#undef CASE_ALIGN_VECTORS

static really_really_inline
m128 rshiftbyte_m128(m128 a, unsigned b) {
    return palignr(zeroes128(), a, b);
}

static really_really_inline
m128 lshiftbyte_m128(m128 a, unsigned b) {
    return palignr(a, zeroes128(), 16 - b);
}

static really_inline
m128 variable_byte_shift_m128(m128 tbl, s32 amount) {
    assert(amount >= -16 && amount <= 16);
    static const uint8x16_t vbs_mask = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
    const uint8x16_t outside_mask = set1_16x8(0xf0);

    m128 idx_masked = palignr_imm(vbs_mask, outside_mask, 16 - amount);
    #ifdef __aarch64__
    return (m128)vqtbl1q_s8(tbl, idx_masked);
    #else
    int8x16_t ret;
    // %e and %f represent the even and odd D registers
    // respectively.
    __asm__ __volatile__(
        "vtbl.8  %e[ret], {%e[tbl], %f[tbl]}, %e[idx]\n"
        "vtbl.8  %f[ret], {%e[tbl], %f[tbl]}, %f[idx]\n"
        : [ret] "=&w"(ret)
        : [tbl] "w"(tbl), [idx] "w"(idx_masked));
    return vreinterpretq_s64_s8(ret);
    #endif
}

#ifdef __cplusplus
extern "C" {
#endif
extern const u8 simd_onebit_masks[];
#ifdef __cplusplus
}
#endif

static really_inline
m128 mask1bit128(unsigned int n) {
    assert(n < sizeof(m128) * 8);
    u32 mask_idx = ((n % 8) * 64) + 95;
    mask_idx -= n / 8;
    return loadu128(&simd_onebit_masks[mask_idx]);
}

// switches on bit N in the given vector.
static really_inline
void setbit128(m128 *ptr, unsigned int n) {
    *ptr = or128(mask1bit128(n), *ptr);
}

// switches off bit N in the given vector.
static really_inline
void clearbit128(m128 *ptr, unsigned int n) {
    *ptr = andnot128(mask1bit128(n), *ptr);
}

// tests bit N in the given vector.
static really_inline
char testbit128(m128 val, unsigned int n) {
    const m128 mask = mask1bit128(n);

    return isnonzero128(and128(mask, val));
}

static really_inline
m128 pshufb_m128(m128 tbl, m128 b) {
    /* On Intel, if bit 0x80 is set, then result is zero, otherwise which the lane it is &0xf.
       In NEON, if >=16, then the result is zero, otherwise it is that lane.
       btranslated is the version that is converted from Intel to NEON.  */
    int8x16_t idx_masked = vandq_s8((int8x16_t)b,vdupq_n_s8(0x8f));
    #ifdef __aarch64__
    return (m128)vqtbl1q_s8((int8x16_t)tbl, (uint8x16_t)idx_masked);
    #else
    int8x16_t ret;
    // %e and %f represent the even and odd D registers
    // respectively.
    __asm__ __volatile__(
        "vtbl.8  %e[ret], {%e[tbl], %f[tbl]}, %e[idx]\n"
        "vtbl.8  %f[ret], {%e[tbl], %f[tbl]}, %f[idx]\n"
        : [ret] "=&w"(ret)
        : [tbl] "w"(tbl), [idx] "w"(idx_masked));
    return vreinterpretq_s64_s8(ret);
    #endif
}

static really_inline
m128 max_u8_m128(m128 a, m128 b) {
    return (m128) vmaxq_u8((uint8x16_t)a, (uint8x16_t)b);
}

static really_inline
m128 min_u8_m128(m128 a, m128 b) {
    return (m128) vminq_u8((uint8x16_t)a, (uint8x16_t)b);
}

static really_inline
m128 sadd_u8_m128(m128 a, m128 b) {
    return (m128) vqaddq_u8((uint8x16_t)a, (uint8x16_t)b);
}

static really_inline
m128 sub_u8_m128(m128 a, m128 b) {
    return (m128) vsubq_u8((uint8x16_t)a, (uint8x16_t)b);
}

static really_inline
m128 set4x32(u32 x3, u32 x2, u32 x1, u32 x0) {
    uint32_t ALIGN_ATTR(16) data[4] = { x0, x1, x2, x3 };
    return (m128) vld1q_u32((uint32_t *) data);
}

static really_inline
m128 set2x64(u64a hi, u64a lo) {
    uint64_t ALIGN_ATTR(16) data[2] = { lo, hi };
    return (m128) vld1q_u64((uint64_t *) data);
}

#endif // ARCH_ARM_SIMD_UTILS_H
