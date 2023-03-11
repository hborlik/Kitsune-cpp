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
	/**
	 * Parsing structures 
	 */

	struct ethhdr *eth;
	struct in6_addr ipv6;
	struct in_addr ipv4;
	struct iphdr *iphdr;
	struct ipv6hdr *ipv6hdr;
	struct udphdr *udphdr;
	struct tcphdr *tcphdr;
	int eth_type, ip_type;
	__u16 src_port = 0;
	__u16 dst_port = 0;

	struct hdr_cursor nh = {.pos = data};

	struct hash ph;

	/* Packet parsing in steps: Get each header one at a time, aborting if
	 * parsing fails. Each helper function does sanity checking (is the
	 * header type in the packet correct?), and bounds checking.
	 */
	eth_type = parse_ethhdr(&nh, data_end, &eth);
	if (eth_type < 0) {
		action = XDP_ABORTED;
		goto out;
	}


	if (eth_type == bpf_htons(ETH_P_IP)) {
		if ((ip_type = parse_iphdr(&nh, data_end, &iphdr)) < 0) {
			action = XDP_ABORTED;
			goto out;
		}
		ipv4.s_addr = iphdr->saddr;
		ipv4_hash(ipv4, &ph.src);
		ipv4.s_addr = iphdr->daddr;
		ipv4_hash(ipv4, &ph.dst);
	} else if (eth_type == bpf_htons(ETH_P_IPV6)) {
		if ((ip_type = parse_ip6hdr(&nh, data_end, &ipv6hdr)) < 0) {
			action = XDP_ABORTED;
			goto out;
		}
		ipv6 = ipv6hdr->addrs.saddr;
		ipv6_hash(&ipv6, &ph.src);
		ipv6 = ipv6hdr->addrs.daddr;
		ipv6_hash(&ipv6, &ph.dst);
	} else { // dont know what it is
		goto out;
	}


	if (ip_type == IPPROTO_UDP) {
		
		if (parse_udphdr(&nh, data_end, &udphdr) < 0) {
			action = XDP_ABORTED;
			goto out;
		}
		// rewrite destination port
		// udphdr->dest = bpf_htons(bpf_ntohs(udphdr->dest) - 1);
		src_port = bpf_ntohs(udphdr->source);
		dst_port = bpf_ntohs(udphdr->dest);
	} else if (ip_type == IPPROTO_TCP) {
		if (parse_tcphdr(&nh, data_end, &tcphdr) < 0) {
			action = XDP_ABORTED;
			goto out;
		}
		// rewrite destination port
		// tcphdr->dest = bpf_htons(bpf_ntohs(tcphdr->dest) - 1);
		src_port = bpf_ntohs(tcphdr->source);
		dst_port = bpf_ntohs(tcphdr->dest);
	} else if (ip_type == IPPROTO_ICMPV6) {

	} else if (ip_type == IPPROTO_ICMP) {
		
	}

	ph.src_port = fasthash64(&src_port, sizeof(src_port), FH_SEED);
	ph.dst_port = fasthash64(&dst_port, sizeof(dst_port), FH_SEED);

	// make a hash for the src IP:port and dst IP:port
	__u64 tmp = 0;

	tmp = hash_mix(tmp, ph.dst.vals[0]);
	tmp = hash_mix(tmp, ph.src.vals[0]);
	tmp = hash_mix(tmp, ph.dst_port);
	tmp = hash_mix(tmp, ph.src_port);

	__u32 hash = tmp - (tmp >> 32);

	int32_t fx_data = (ctx->data_end - ctx->data) << 16;
	fx_data = div_s15p16(fx_data, 1000 << 16);
	fx_data = div_s15p16(fx_data, 1000 << 16);
	if (inc_stat_insert(hash, fx_data, timestamp) < 0) {
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
