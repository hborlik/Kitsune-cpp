/* SPDX-License-Identifier: GPL-2.0 */

/* Used by BPF-prog kernel side BPF-progs and userspace programs,
 * for sharing xdp_stats common struct and DEFINEs.
 */
#ifndef __XDP_STATS_KERN_USER_H
#define __XDP_STATS_KERN_USER_H

#include <stdbool.h>
#include <stdint.h>

#define N_INC_STATS 5

// 100ms, 500ms, 1.5sec, 10sec, and 1min
// 100ms, 500ms, 1500ms, 10000ms, 60000ms
// λ = 5, 3, 1, 0.1, 0.01
static const int32_t fx_lambdas[N_INC_STATS] = {20 << 16, 33 << 16, 100 << 16, 1000 << 16, 10000 << 16}; // centiseconds

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
    __u64   last_t;     // timestamp for last inserted measurement in microseconds
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

#define INC_STAT_MAX_ENTRIES 5000

#endif /* __XDP_STATS_KERN_USER_H */
