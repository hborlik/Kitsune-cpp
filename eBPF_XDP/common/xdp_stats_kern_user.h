/* SPDX-License-Identifier: GPL-2.0 */

/* Used by BPF-prog kernel side BPF-progs and userspace programs,
 * for sharing xdp_stats common struct and DEFINEs.
 */
#ifndef __XDP_STATS_KERN_USER_H
#define __XDP_STATS_KERN_USER_H

#include <stdbool.h>
#include <stdint.h>

#define INC_STAT_SIZE 20

/* This is the data record stored in the map */
struct datarec {
	__u64 rx_packets;
	__u64 rx_bytes;
};

/* IncStat */
struct inc_stat {
    __u64   last_t;     // timestamp for last inserted measurement in milliseconds
	int32_t CF1		    [INC_STAT_SIZE];
    int32_t CF2		    [INC_STAT_SIZE];
    int32_t w		    [INC_STAT_SIZE];
    int32_t cur_std	    [INC_STAT_SIZE];
    int32_t cur_mean	[INC_STAT_SIZE];
    int32_t cur_var	    [INC_STAT_SIZE];
    bool  isTypeDiff;
    struct bpf_spin_lock lock;
};

#ifndef XDP_ACTION_MAX
#define XDP_ACTION_MAX (XDP_REDIRECT + 1)
#endif

#endif /* __XDP_STATS_KERN_USER_H */
