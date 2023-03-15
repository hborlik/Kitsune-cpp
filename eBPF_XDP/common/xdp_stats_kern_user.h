/* SPDX-License-Identifier: GPL-2.0 */

/* Used by BPF-prog kernel side BPF-progs and userspace programs,
 * for sharing xdp_stats common struct and DEFINEs.
 */
#ifndef __XDP_STATS_KERN_USER_H
#define __XDP_STATS_KERN_USER_H

#include <asm-generic/int-ll64.h>

#include <linux/in.h>
#include <linux/in6.h>
#include <stdbool.h>
#include <stdint.h>

#include "fixed_point.h"
#include "fasthash.h"
#include "bpf_endian.h"
#include "parsing_helpers.h"

#define INC_STAT_MAX_ENTRIES 5000
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

#define FH_SEED (0x2d31e867)
#define L3_SEED (0x6ad611c3)

enum address_gen {
	ADDRESS_IP       = 0, // /32 or /128
	ADDRESS_NET      = 1, // /24 or /48
	// ADDRESS_WILDCARD = 2, // /0
};

struct address_hash {
	__u64 vals[1]; // [ADDRESS_WILDCARD]
};

struct hash {
	struct address_hash src;
	struct address_hash dst;
	__u64 src_port;
	__u64 dst_port;
	__u64 src_mac;

	__u32 srcMAC_IP;
	__u32 srcIP;
	__u32 srcIP_dstIP; // Channel in paper
	__u32 full; // packet’s source and destination TCP/UDP Socket + IP
};

static __always_inline
void ipv6_hash(const struct in6_addr *ip, struct address_hash *a)//, struct address_hash *b)
{
	a->vals[ADDRESS_IP]  = fasthash64(ip, sizeof(*ip), FH_SEED);
	// b->vals[ADDRESS_IP]  = hashlittle(ip, sizeof(*ip), L3_SEED);
	// a->vals[ADDRESS_NET] = fasthash64(ip, 48 / 8, FH_SEED);
	// b->vals[ADDRESS_NET] = hashlittle(ip, 48 / 8, L3_SEED);
}

static __always_inline
void ipv4_hash(struct in_addr ip, struct address_hash *a)//, struct address_hash *b)
{
	a->vals[ADDRESS_IP] = fasthash64(&ip, sizeof(ip), FH_SEED);
	// b->vals[ADDRESS_IP] = hashlittle(&ip, sizeof(ip), L3_SEED);
	// ip.s_addr &= 0xffffff00;
	// a->vals[ADDRESS_NET] = fasthash64(&ip, sizeof(ip), FH_SEED);
	// b->vals[ADDRESS_NET] = hashlittle(&ip, sizeof(ip), L3_SEED);
}

static __always_inline
__u64 hash_mix(__u64 a, __u64 b)
{
	// Adapted from https://stackoverflow.com/a/27952689. The constant below
	// is derived from the golden ratio.
	a ^= b + 0x9e3779b97f4a7c15 + (a << 6) + (a >> 2);
	return a;
}

static __always_inline
int parse_and_hash(void *data_end, void *data, struct hash *ph) {
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

	/* Packet parsing in steps: Get each header one at a time, aborting if
	 * parsing fails. Each helper function does sanity checking (is the
	 * header type in the packet correct?), and bounds checking.
	 */
	eth_type = parse_ethhdr(&nh, data_end, &eth);
	if (eth_type < 0) {
		return -1;
	}


	if (eth_type == bpf_htons(ETH_P_IP)) {
		if ((ip_type = parse_iphdr(&nh, data_end, &iphdr)) < 0) {
			return -1;
		}
		ipv4.s_addr = iphdr->saddr;
		ipv4_hash(ipv4, &ph->src);
		ipv4.s_addr = iphdr->daddr;
		ipv4_hash(ipv4, &ph->dst);
	} else if (eth_type == bpf_htons(ETH_P_IPV6)) {
		if ((ip_type = parse_ip6hdr(&nh, data_end, &ipv6hdr)) < 0) {
			return -1;
		}
		ipv6 = ipv6hdr->addrs.saddr;
		ipv6_hash(&ipv6, &ph->src);
		ipv6 = ipv6hdr->addrs.daddr;
		ipv6_hash(&ipv6, &ph->dst);
	} else { // dont know what it is
		return -1;
	}


	if (ip_type == IPPROTO_UDP) {
		
		if (parse_udphdr(&nh, data_end, &udphdr) < 0) {
			return -1;
		}
		// rewrite destination port
		// udphdr->dest = bpf_htons(bpf_ntohs(udphdr->dest) - 1);
		src_port = bpf_ntohs(udphdr->source);
		dst_port = bpf_ntohs(udphdr->dest);
	} else if (ip_type == IPPROTO_TCP) {
		if (parse_tcphdr(&nh, data_end, &tcphdr) < 0) {
			return -1;
		}
		// rewrite destination port
		// tcphdr->dest = bpf_htons(bpf_ntohs(tcphdr->dest) - 1);
		src_port = bpf_ntohs(tcphdr->source);
		dst_port = bpf_ntohs(tcphdr->dest);
	} else if (ip_type == IPPROTO_ICMPV6) {

	} else if (ip_type == IPPROTO_ICMP) {
		
	}

	ph->src_mac = fasthash64(eth->h_source, sizeof(eth->h_source), FH_SEED);
	ph->src_port = fasthash64(&src_port, sizeof(src_port), FH_SEED);
	ph->dst_port = fasthash64(&dst_port, sizeof(dst_port), FH_SEED);

	// make a hashes
	__u64 tmp = 0;

	tmp = hash_mix(tmp, ph->src.vals[0]); // source IP
	ph->srcIP = tmp - (tmp >> 32);

	__u64 tmp2 = hash_mix(tmp, ph->src_mac); // source IP, source MAC
	ph->srcMAC_IP = tmp2 - (tmp2 >> 32);

	tmp = hash_mix(tmp, ph->dst.vals[0]); // source IP, destination IP
	ph->srcIP_dstIP = tmp - (tmp >> 32);

	tmp = hash_mix(tmp, ph->src_port); // source IP, destination IP, src port
	tmp = hash_mix(tmp, ph->dst_port); // source IP, destination IP, src port, dst port
	ph->full = tmp - (tmp >> 32);

	return 0;
}

#endif /* __XDP_STATS_KERN_USER_H */
