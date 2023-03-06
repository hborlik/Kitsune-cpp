/* SPDX-License-Identifier: GPL-2.0 */

/* Used by BPF-prog kernel side BPF-progs and userspace programs,
 * for sharing xdp_stats common struct and DEFINEs.
 */
#ifndef __XDP_STATS_KERN_USER_H
#define __XDP_STATS_KERN_USER_H

#include <stdbool.h>
#include <stdint.h>
#include "fixed_point.h"

#define N_INC_STATS 5

// 100ms, 500ms, 1.5sec, 10sec, and 1min
// 100ms, 500ms, 1500ms, 10000ms, 60000ms
// λ = 5, 3, 1, 0.1, 0.01
static const int32_t fx_lambdas[N_INC_STATS] = {327680, 196608, 65536, 6554, 655}; // values converted using double_to_s15p16

/* This is the data record stored in the map */
struct datarec {
	__u64 rx_packets;
	__u64 rx_bytes;
};

#ifndef XDP_ACTION_MAX
#define XDP_ACTION_MAX (XDP_REDIRECT + 1)
#endif

/* IncStat */
struct inc_stat {
    __u64   last_t;     // timestamp  for last inserted measurement nanoseconds
	int32_t CF1		    [N_INC_STATS];
    int32_t CF2		    [N_INC_STATS];
    int32_t w		    [N_INC_STATS];
    bool  isTypeDiff;
    struct bpf_spin_lock lock;
};

struct inc_stat_features {
    float cur_std	    [N_INC_STATS];
    float cur_mean	    [N_INC_STATS];
    float cur_var	    [N_INC_STATS];
};

static __always_inline
int32_t update_and_process_decay(__u64 timestamp, int32_t fx_value, struct inc_stat *stat) {
    // timestamp in microseconds
	// __u64 diff = (timestamp - stat->last_t) / 1000 / 1000 / 10; // time difference in centiseconds
	// smallest value we can represent with s15p16 is 0.00001 (10E-5)
	__u64 diff_raw = (timestamp - stat->last_t) / 1000; // microseconds 10E-6
	__u64 diff_s = diff_raw / 1000000;
	__u64 diff_remain = (diff_raw % 1000000) / 100; // microseconds to 10E-4 [0, 10000)
	int32_t fx_diff_s = int_to_s15p16(diff_s);
	fx_diff_s += div_s15p16(int_to_s15p16(diff_remain), 10000 << 16);
	// If isTypeDiff is set, use the time difference as statistics
	if (stat->isTypeDiff) {
		if (fx_diff_s > 0)
			fx_value = fx_diff_s;
		else
			fx_value = 0;
	}

	// Process Decay first
    if (fx_diff_s > 0) { //
#pragma clang loop unroll(full)
        for (__u32 i = 0; i < N_INC_STATS; ++i) {
            // Calculate the decay factor
			int32_t fx_t 		= mul_s15p16(fx_diff_s, -fx_lambdas[i]);
            int32_t fx_factor 	= fixed_exp2(fx_t);
            stat->CF1[i] 	= mul_s15p16(fx_factor, stat->CF1[i]);
            stat->CF2[i] 	= mul_s15p16(fx_factor, stat->CF2[i]);
            stat->w[i] 		= mul_s15p16(fx_factor, stat->w[i]);
        }
        stat->last_t = timestamp;
    }

	// update with v
#pragma clang loop unroll(full)
	for (__u32 i = 0; i < N_INC_STATS; ++i) stat->CF1[i] += fx_value;

#pragma clang loop unroll(full)
	for (__u32 i = 0; i < N_INC_STATS; ++i) stat->CF2[i] += mul_s15p16(fx_value, fx_value);

#pragma clang loop unroll(full)
	for (__u32 i = 0; i < N_INC_STATS; ++i) stat->w[i] += (1 << 16); // add one to weight

    return 0;
}

#define INC_STAT_MAX_ENTRIES 5000

#endif /* __XDP_STATS_KERN_USER_H */
