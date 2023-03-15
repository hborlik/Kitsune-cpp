/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/bpf.h>
#include <linux/in.h>
#include <bpf/bpf_helpers.h>
#include <math.h>
#include <stdbool.h>

#include "bpf_endian.h"
#include "parsing_helpers.h"

/* LLVM maps __sync_fetch_and_add() as a built-in function to the BPF atomic add
 * instruction (that is BPF_STX | BPF_XADD | BPF_W for word sizes)
 */
#ifndef lock_xadd
#define lock_xadd(ptr, val)	((void) __sync_fetch_and_add(ptr, val))
#endif

#include <xdp_stats_kern_user.h>
#include <xdp_stats_kern.h>


SEC("xdp")
int  xdp_stats_func(struct xdp_md *ctx)
{
	void *data_end = (void *)(long)ctx->data_end;
	void *data     = (void *)(long)ctx->data;
	int action = XDP_PASS;

	__u64 timestamp = bpf_ktime_get_ns();
	
	struct hash ph;
	if (parse_and_hash(data_end, data, &ph) < 0) {
		action = XDP_ABORTED;
		goto out;
	}

	int32_t fx_data = (ctx->data_end - ctx->data) << 16;
	fx_data = div_s15p16(fx_data, 1000 << 16);
	fx_data = div_s15p16(fx_data, 1000 << 16);
	if (inc_stat_insert(ph.full, fx_data, timestamp) < 0) {
		action = XDP_ABORTED;
		goto out;
	}

out:

	return xdp_stats_record_action(ctx, action);
}

char _license[] SEC("license") = "GPL";

/* Copied from: $KERNEL/include/uapi/linux/bpf.h
 *
 * User return codes for XDP prog type.
 * A valid XDP program must return one of these defined values. All other
 * return codes are reserved for future use. Unknown return codes will
 * result in packet drops and a warning via bpf_warn_invalid_xdp_action().
 *
enum xdp_action {
	XDP_ABORTED = 0,
	XDP_DROP,
	XDP_PASS,
	XDP_TX,
	XDP_REDIRECT,
};

 * user accessible metadata for XDP packet hook
 * new fields must be added to the end of this structure
 *
struct xdp_md {
	// (Note: type __u32 is NOT the real-type)
	__u32 data;
	__u32 data_end;
	__u32 data_meta;
	// Below access go through struct xdp_rxq_info
	__u32 ingress_ifindex; // rxq->dev->ifindex
	__u32 rx_queue_index;  // rxq->queue_index
};
*/
