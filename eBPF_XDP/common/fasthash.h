/* The MIT License

   Copyright (C) 2012 Zilong Tan (eric.zltan@gmail.com)

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use, copy,
   modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#pragma once

#include <linux/types.h>

// clang-format off

// Compression function for Merkle-Damgard construction.
// This function is generated using the framework provided.
static __attribute__((always_inline)) inline __u64 fasthash_mix(__u64 h) {
	h ^= h >> 23;
	h *= 0x2127599bf4325c37ULL;
	h ^= h >> 47;
	return h;
}

static __attribute__((always_inline)) inline __u64 fasthash64(const void *buf, __u64 len, __u64 seed)
{
	const __u64 m = 0x880355f21e6d1965ULL;
	const __u64 *pos = (const __u64 *)buf;
	const __u64 *end = pos + (len / 8);
	__u64 h = seed ^ (len * m);
	__u64 v;

#pragma clang loop unroll(full)
	while (pos != end) {
		v  = *pos++;
		h ^= fasthash_mix(v);
		h *= m;
	}

	if (len & 7) {
		v = 0;
		__builtin_memcpy(&v, pos, len & 7);
		h ^= fasthash_mix(v);
		h *= m;
	}

	return fasthash_mix(h);
}

static __attribute__((always_inline)) inline __u32 fasthash32(const void *buf, __u64 len, __u32 seed)
{
	// the following trick converts the 64-bit hashcode to Fermat
	// residue, which shall retain information from both the higher
	// and lower parts of hashcode.
        __u64 h = fasthash64(buf, len, seed);
	return h - (h >> 32);
}