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
int32_t sdiv(int32_t a, int32_t b) {
	bool aneg = a < 0;
	bool bneg = b < 0;
	// get the absolute positive value of both
	uint32_t adiv = aneg ? -a : a;
	uint32_t bdiv = bneg ? -b : b;
	// Do udiv
	uint32_t out = adiv / bdiv;
	// Make output negative if one or the other is negative, not both
	return aneg != bneg ? -out : out;
}

/* s15.16 division without rounding */
static __always_inline
int32_t fxdiv_s15p16 (int32_t x, int32_t y)
{
	return sdiv(((int64_t)x * 65536), y);
}

/* s15.16 multiplication with rounding */
static __always_inline
int32_t fxmul_s15p16 (int32_t x, int32_t y)
{
	int32_t r;
	int64_t t = (int64_t)x * (int64_t)y;
	r = (int32_t)(uint32_t)(((uint64_t)t + (1 << 15)) >> 16);
	return r;
}

/* log2(2**8), ..., log2(2**1), log2(1+2**(-1), ..., log2(1+2**(-16)) */
static const uint32_t tab [20] = {0x80000, 0x40000, 0x20000, 0x10000,
						   0x095c1, 0x0526a, 0x02b80, 0x01663,
						   0x00b5d, 0x005b9, 0x002e0, 0x00170, 
						   0x000b8, 0x0005c, 0x0002e, 0x00017, 
						   0x0000b, 0x00006, 0x00003, 0x00001};
static const int32_t one_s15p16 = 1 * (1 << 16);
static const int32_t neg_fifteen_s15p16 = (-15) * (1 << 16);

static __always_inline
int32_t fxlog2_s15p16 (int32_t a)
{
	uint32_t x, y;
	int32_t t, r;

	x = (a > one_s15p16) ? fxdiv_s15p16 (one_s15p16, a) : a;
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
int32_t fxexp2_s15p16 (int32_t a) 
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
	r = (a < 0) ? fxdiv_s15p16 (one_s15p16, y) : y;
	return r;
}

/* compute a**b for a >= 0 */
static __always_inline
int32_t fxpow_s15p16 (int32_t a, int32_t b)
{
	return fxexp2_s15p16 (fxmul_s15p16 (b, fxlog2_s15p16 (a)));
}

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