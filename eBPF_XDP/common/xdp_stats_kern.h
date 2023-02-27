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
	__uint(max_entries, 100);
	__type(key, __u32);
	__type(value, struct inc_stat);
} xdp_inc_stat_map SEC(".maps");


static float lambdas[] = {5, 3, 1, 0.1, 0.01};
static const __u32 lambdas_size = sizeof(lambdas)/sizeof(lambdas[0]);

static __always_inline
int inc_stat_insert(__u32 key, float value, float timestamp) {
	/* Lookup in kernel BPF-side return pointer to actual data record */
	struct inc_stat *stat = bpf_map_lookup_elem(&xdp_inc_stat_map, &key);
	bool did_create = false;
	if (!stat) {
		struct inc_stat new_inc_stat = {
			.w = {[0 ... INC_STAT_SIZE-1] = 1e-20}
		};

		stat = &new_inc_stat;
		did_create = true;
	} else {
		bpf_spin_lock(&stat->lock);
	}


	// If isTypeDiff is set, use the time difference as statistics
	float diff = timestamp - stat->last_t;
	if (stat->isTypeDiff) {
		if (diff > 0)
			value = diff;
		else
			value = 0;
	}


	// Decay first
    if (diff > 0) {
        for (__u32 i = 0; i < lambdas_size; ++i) {
            // Calculate the decay factor
            float factor = pow(2.0f, -lambdas[i] * diff);
            stat->CF1[i] *= factor;
            stat->CF2[i] *= factor;
            stat->w[i] *= factor;
        }
        stat->last_t = timestamp;
    }

	// update with v
	for (__u32 i = 0; i < lambdas_size; ++i) stat->CF1[i] += value;
	for (__u32 i = 0; i < lambdas_size; ++i) stat->CF2[i] += value * value;
	for (__u32 i = 0; i < lambdas_size; ++i) ++(stat->w[i]);
	
	if (!did_create){
		bpf_spin_unlock(&stat->lock);
	} else if (bpf_map_update_elem(&xdp_inc_stat_map, &key, stat, BPF_NOEXIST) != 0) {
		return -1; // value was probably inserted before we managed to get here
	}

	return 0;
}



#endif /* __XDP_STATS_KERN_H */
