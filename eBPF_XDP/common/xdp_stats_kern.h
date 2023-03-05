/* SPDX-License-Identifier: GPL-2.0 */

/* Used *ONLY* by BPF-prog running kernel side. */
#ifndef __XDP_STATS_KERN_H
#define __XDP_STATS_KERN_H

/* Data record type 'struct datarec' is defined in common/xdp_stats_kern_user.h,
 * programs using this header must first include that file.
 */
#ifndef __XDP_STATS_KERN_USER_H
#warning "You forgot to #include <../common/xdp_stats_kern_user.h>"
#include <../common/xdp_stats_kern_user.h>
#endif

#include <asm-generic/int-ll64.h>

#include <fixed_point.h>

#define BPF_STATS_MAP_TYPE BPF_MAP_TYPE_ARRAY

/* Keeps stats per (enum) xdp_action */
struct bpf_map_def {
	__uint(type, BPF_STATS_MAP_TYPE);
	__uint(max_entries, XDP_ACTION_MAX);
	__type(key, __u32);
	__type(value, struct datarec);
} xdp_stats_map SEC(".maps");

static __always_inline
__u32 xdp_stats_record_action(struct xdp_md *ctx, __u32 action)
{
	if (action >= XDP_ACTION_MAX)
		return XDP_ABORTED;

	/* Lookup in kernel BPF-side return pointer to actual data record */
	struct datarec *rec = bpf_map_lookup_elem(&xdp_stats_map, &action);
	if (!rec)
		return XDP_ABORTED;

	if (BPF_STATS_MAP_TYPE == BPF_MAP_TYPE_PERCPU_ARRAY) {
		/* BPF_MAP_TYPE_PERCPU_ARRAY returns a data record specific to current
		* CPU and XDP hooks runs under Softirq, which makes it safe to update
		* without atomic operations.
		*/
		rec->rx_packets++;
		rec->rx_bytes += (ctx->data_end - ctx->data);
	} else {
		lock_xadd(&rec->rx_packets, 1);
		lock_xadd(&rec->rx_bytes, (ctx->data_end - ctx->data));
	}

	return action;
}

/* Keeps stats per session (IP, DEST, IP-DEST, and port based) */
/*
* BPF_MAP_TYPE_HASH
	Hash-table maps have the following characteristics:

	*  Maps are created and destroyed by user-space programs.
		Both user-space and eBPF programs can perform lookup,
		update, and delete operations.

	*  The kernel takes care of allocating and freeing
		key/value pairs.

	*  The map_update_elem() helper will fail to insert new
		element when the max_entries limit is reached.  (This
		ensures that eBPF programs cannot exhaust memory.)

	*  map_update_elem() replaces existing elements
		atomically.

	Hash-table maps are optimized for speed of lookup.
*/
struct bpf_map_inc_stat {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, INC_STAT_MAX_ENTRIES);
	__type(key, __u32);
	__type(value, struct inc_stat);
} xdp_inc_stat_map SEC(".maps");


static const int32_t fx_w_init = 1 << 0; // smallest fx value
static const int32_t fx_two = 2 << 16;
static const struct inc_stat default_inc_stat = {
	.w = {[0 ... N_INC_STATS-1] = fx_w_init}
};


/* key, fixed point value, and timestamp in milliseconds */
static __always_inline
int inc_stat_insert(__u32 key, int32_t fx_value, __u64 timestamp) {
	int rv = 0;
	/* Lookup in kernel BPF-side return pointer to actual data record */
	struct inc_stat *stat = bpf_map_lookup_elem(&xdp_inc_stat_map, &key);
	bool did_create = false;
	if (!stat) {
		bpf_map_update_elem(&xdp_inc_stat_map, &key, &default_inc_stat, BPF_NOEXIST); // return value not important
		stat = bpf_map_lookup_elem(&xdp_inc_stat_map, &key);
		did_create = true;
		if (!stat) {
			return -1; // The key should exist at this point
		}
	}

	bpf_spin_lock(&stat->lock);
	if (did_create) {
		stat->last_t = timestamp;
		for (__u32 i = 0; i < N_INC_STATS; ++i) {
			stat->CF1[i] = fx_value;
		}
	}

	// timestamp in microseconds
	__u64 diff = (timestamp - stat->last_t) / 1000 / 1000 / 10; // time difference in centiseconds
	// smallest value we can represent with s15p16 is 0.00001 (10E-5)
	__u64 diff_raw = (timestamp - stat->last_t) / 1000; // microseconds 10E-6
	__u64 diff_s = diff_raw / 1000000;
	__u64 diff_remain = (diff_raw % 1000000) / 100; // microseconds to 10E-4 [0, 10000)
	int32_t fx_diff_s = int_to_s15p16(diff_s);
	fx_diff_s += div_s15p16(int_to_s15p16(diff_remain), 10000 << 16);
	// If isTypeDiff is set, use the time difference as statistics
	if (stat->isTypeDiff) {
		if (diff > 0)
			fx_value = diff;
		else
			fx_value = 0;
	}

	// Decay first
    if (diff > 0) {
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

// 	// update with v
// #pragma clang loop unroll(full)
// 	for (__u32 i = 0; i < N_INC_STATS; ++i) stat->CF1[i] += fx_value;

#pragma clang loop unroll(full)
	for (__u32 i = 0; i < N_INC_STATS; ++i) stat->CF2[i] += mul_s15p16(fx_value, fx_value);

#pragma clang loop unroll(full)
	for (__u32 i = 0; i < N_INC_STATS; ++i) ++(stat->w[i]);
	
	// all exec paths required to unlock
	bpf_spin_unlock(&stat->lock);

	return rv;
}



#endif /* __XDP_STATS_KERN_H */
