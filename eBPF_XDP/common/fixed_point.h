// fixed point 

#ifndef FIXED_POINT_H
#define FIXED_POINT_H

// from https://stackoverflow.com/questions/64941736/efficient-implementation-of-fixed-point-power-pow-function-with-argument-const

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// signed division from https://stackoverflow.com/questions/74227051/is-there-a-way-to-perform-signed-division-in-ebpf
static __always_inline
int64_t sdiv(int64_t a, int64_t b) {
	bool aneg = a < 0;
	bool bneg = b < 0;
	// get the absolute positive value of both
	uint64_t adiv = aneg ? -a : a;
	uint64_t bdiv = bneg ? -b : b;
	// Do udiv
	uint64_t out = adiv / bdiv;
	// Make output negative if one or the other is negative, not both
	return aneg != bneg ? -out : out;
}

/* s15.16 division without rounding */
static __always_inline
int32_t div_s15p16 (int32_t x, int32_t y)
{
	return sdiv(((int64_t)x * 65536), y);
}

/* s15.16 multiplication with rounding */
static __always_inline
int32_t mul_s15p16 (int32_t x, int32_t y)
{
	int32_t r;
	int64_t t = (int64_t)x * (int64_t)y;
	r = (int32_t)(uint32_t)(((uint64_t)t + (1 << 15)) >> 16);
	return r;
}

/* log2(2**8), ..., log2(2**1), log2(1+2**(-1), ..., log2(1+2**(-16)) */
static const uint32_t tab[20] = {
	0x80000, 0x40000, 0x20000, 0x10000,
	0x095c1, 0x0526a, 0x02b80, 0x01663,
	0x00b5d, 0x005b9, 0x002e0, 0x00170,
	0x000b8, 0x0005c, 0x0002e, 0x00017,
	0x0000b, 0x00006, 0x00003, 0x00001};
static const int32_t one_s15p16 = 1 * (1 << 16);
static const int32_t neg_fifteen_s15p16 = (-15) * (1 << 16);

static __always_inline
int32_t log2_s15p16 (int32_t a)
{
	uint32_t x, y;
	int32_t t, r;

	x = (a > one_s15p16) ? div_s15p16 (one_s15p16, a) : a;
	y = 0;
	/* process integer bits */
	if ((t = x << 8) < one_s15p16) { x = t; y += tab [0]; }
	if ((t = x << 4) < one_s15p16) { x = t; y += tab [1]; }
	if ((t = x << 2) < one_s15p16) { x = t; y += tab [2]; }
	if ((t = x << 1) < one_s15p16) { x = t; y += tab [3]; }
	/* process fraction bits */
#pragma clang loop unroll(full)
	for (int shift = 1; shift <= 16; shift++) {
		if ((t = x + (x >> shift)) < one_s15p16) { x = t; y += tab[3 + shift]; }
	}
	r = (a > one_s15p16) ? y : (0 - y);
	return r;
}

static __always_inline
int32_t exp2_s15p16 (int32_t a) 
{
	uint32_t x, y;
	int32_t t, r;

	if (a <= neg_fifteen_s15p16) return 0; // underflow

	x = (a < 0) ? (-a) : (a);
	y = one_s15p16;
	/* process integer bits */
	if ((t = x - tab [0]) >= 0) { x = t; y = y << 8; }
	if ((t = x - tab [1]) >= 0) { x = t; y = y << 4; }
	if ((t = x - tab [2]) >= 0) { x = t; y = y << 2; }
	if ((t = x - tab [3]) >= 0) { x = t; y = y << 1; }
	/* process fractional bits */
#pragma clang loop unroll(full)
	for (int shift = 1; shift <= 16; shift++) {
		if ((t = x - tab [3 + shift]) >= 0) { x = t; y = y + (y >> shift); }
	}
	r = (a < 0) ? div_s15p16 (one_s15p16, y) : y;
	return r;
}

// following is from https://stackoverflow.com/questions/54661131/log2-approximation-in-fixed-point

#define FRAC_BITS_OUT (16)
#define INT_BITS_OUT  (15)
#define FRAC_BITS_IN  (16)
#define INT_BITS_IN   (15)

/* count leading zeros: intrinsic or machine instruction on many architectures */
static __always_inline
int32_t clz (uint32_t x)
{
    uint32_t n, y;

    n = 31 + (!x);
    if ((y = (x & 0xffff0000U))) { n -= 16;  x = y; }
    if ((y = (x & 0xff00ff00U))) { n -=  8;  x = y; }
    if ((y = (x & 0xf0f0f0f0U))) { n -=  4;  x = y; }
    if ((y = (x & 0xccccccccU))) { n -=  2;  x = y; }
    if ((    (x & 0xaaaaaaaaU))) { n -=  1;         }
    return n;
}

#define LOG2_TBL_SIZE (6)
#define TBL_SIZE      ((1 << LOG2_TBL_SIZE) + 2)

/* for i = [0,65]: log2(1 + i/64) * (1 << 31) */
static const uint32_t log2Tab [TBL_SIZE] =
{
    0x00000000, 0x02dcf2d1, 0x05aeb4dd, 0x08759c50, 
    0x0b31fb7d, 0x0de42120, 0x108c588d, 0x132ae9e2, 
    0x15c01a3a, 0x184c2bd0, 0x1acf5e2e, 0x1d49ee4c, 
    0x1fbc16b9, 0x22260fb6, 0x24880f56, 0x26e2499d, 
    0x2934f098, 0x2b803474, 0x2dc4439b, 0x30014ac6, 
    0x32377512, 0x3466ec15, 0x368fd7ee, 0x38b25f5a, 
    0x3acea7c0, 0x3ce4d544, 0x3ef50ad2, 0x40ff6a2e, 
    0x43041403, 0x450327eb, 0x46fcc47a, 0x48f10751, 
    0x4ae00d1d, 0x4cc9f1ab, 0x4eaecfeb, 0x508ec1fa, 
    0x5269e12f, 0x5440461c, 0x5612089a, 0x57df3fd0, 
    0x59a80239, 0x5b6c65aa, 0x5d2c7f59, 0x5ee863e5, 
    0x60a02757, 0x6253dd2c, 0x64039858, 0x65af6b4b, 
    0x675767f5, 0x68fb9fce, 0x6a9c23d6, 0x6c39049b, 
    0x6dd2523d, 0x6f681c73, 0x70fa728c, 0x72896373, 
    0x7414fdb5, 0x759d4f81, 0x772266ad, 0x78a450b8, 
    0x7a231ace, 0x7b9ed1c7, 0x7d17822f, 0x7e8d3846, 
    0x80000000, 0x816fe50b
};

#define RND_SHIFT     (31 - FRAC_BITS_OUT)
#define RND_CONST     ((1 << RND_SHIFT) / 2)
#define RND_ADJUST    (0x10d) /* established heuristically */

/* 
   compute log2(x) in s15.16 format, where x is in s15.16 format
   maximum absolute error 8.18251e-6 @ 0x20352845 (0.251622232)
*/
static __always_inline
int32_t fixed_log2 (int32_t x)
{
    int32_t f1, f2, dx, a, b, approx, lz, i, idx;
    uint32_t t;

    /* x = 2**i * (1 + f), 0 <= f < 1. Find i */
    lz = clz (x);
    i = INT_BITS_IN - lz;
    /* normalize f */
    t = (uint32_t)x << (lz + 1);
    /* index table of log2 values using LOG2_TBL_SIZE msbs of fraction */
    idx = t >> (32 - LOG2_TBL_SIZE);
    /* difference between argument and smallest sampling point */
    dx = t - (idx << (32 - LOG2_TBL_SIZE));
    /* fit parabola through closest three sampling points; find coeffs a, b */
    f1 = (log2Tab[idx+1] - log2Tab[idx]);
    f2 = (log2Tab[idx+2] - log2Tab[idx]);
    a = f2 - (f1 << 1);
    b = (f1 << 1) - a;
    /* find function value for argument by computing ((a*dx+b)*dx) */
    approx = (int32_t)((((int64_t)a)*dx) >> (32 - LOG2_TBL_SIZE)) + b;
    approx = (int32_t)((((int64_t)approx)*dx) >> (32 - LOG2_TBL_SIZE + 1));
    approx = log2Tab[idx] + approx;
    /* round fractional part of result */
    approx = (((uint32_t)approx) + RND_CONST + RND_ADJUST) >> RND_SHIFT;
    /* combine integer and fractional parts of result */
    return (i << FRAC_BITS_OUT) + approx;
}

// from https://stackoverflow.com/questions/36550388/power-of-2-approximation-in-fixed-point
/* compute exp2(a) in s15.16 fixed-point arithmetic, -16 < a < 15 */
static __always_inline
int32_t fixed_exp2 (int32_t a)
{
    int32_t i, f, r, s;
    /* split a = i + f, such that f in [-0.5, 0.5] */
    i = ((a + 0x8000) & ~0xffff); // add 0.5 and clear decimal bits
    f = a - i;
    s = ((15 << 16) - i) >> 16;
    /* minimax approximation for exp2(f) on [-0.5, 0.5] */
    r = 0x00000e20;                 // 5.5171669058037949e-2
    r = (r * f + 0x3e1cc333) >> 17; // 2.4261112219321804e-1
    r = (r * f + 0x58bd46a6) >> 16; // 6.9326098546062365e-1
    r = r * f + 0x7ffde4a3;         // 9.9992807353939517e-1
    return (uint32_t)r >> s;
}

/* compute a**b for a >= 0 */
// a ** b = 
static __always_inline
int32_t pow_s15p16 (int32_t a, int32_t b)
{
	return exp2_s15p16 (mul_s15p16 (b, log2_s15p16 (a)));
}

/* compute a**b for a >= 0 */
// -16 < b * log2(a) < 15
static __always_inline
int32_t fixed_pow_s15p16 (int32_t a, int32_t b)
{
	return fixed_exp2 (mul_s15p16 (b, fixed_log2 (a)));
}

// s15p16 conversion functions

static __attribute__((unused)) __always_inline
double s15p16_to_double (int32_t a)
{
	return a / 65536.0;
}

static __attribute__((unused)) __always_inline
int32_t double_to_s15p16 (double a)
{
	return ((int32_t)(a * 65536.0 + ((a < 0) ? (-0.5) : 0.5)));
}

static __always_inline
int32_t int_to_s15p16(int a) {
	return (a) * (1 << 16);
}

#endif // FIXED_POINT_H