/* SPDX-License-Identifier: GPL-2.0 */

/* Used by BPF-prog kernel side BPF-progs and userspace programs,
 * for sharing xdp_stats common struct and DEFINEs.
 */
#ifndef __XDP_STATS_KERN_USER_H
#define __XDP_STATS_KERN_USER_H

#include <stdbool.h>

#define INC_STAT_SIZE 20

/* This is the data record stored in the map */
struct datarec {
	__u64 rx_packets;
	__u64 rx_bytes;
};

/* IncStat */
struct inc_stat {
    float last_t;
	float CF1		[INC_STAT_SIZE];
    float CF2		[INC_STAT_SIZE];
    float w		    [INC_STAT_SIZE];
    float cur_std	[INC_STAT_SIZE];
    float cur_mean	[INC_STAT_SIZE];
    float cur_var	[INC_STAT_SIZE];
    bool  isTypeDiff;
    struct bpf_spin_lock lock;
};

#ifndef XDP_ACTION_MAX
#define XDP_ACTION_MAX (XDP_REDIRECT + 1)
#endif

#endif /* __XDP_STATS_KERN_USER_H */
