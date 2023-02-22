// from https://www.tigera.io/learn/guides/ebpf/ebpf-xdp/
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <bpf/bpf_helpers.h>
#include <arpa/inet.h>

SEC("xdp")
int xdp_pass_prog(struct xdp_md *ctx) {
    return XDP_PASS;
}

// SEC("xdp/drop_ipv6")
// int xdp_drop_ipv6(struct xdp_md *ctx) {
//     void *data_end = (void*)(long)ctx->data_end;
//     void *data = (void*)(long)ctx->data;
//     struct ethhdr *eth = data;
//     __u16 h_proto;

//     // static analysis requirement to check data bounds
//     if (data + sizeof(struct ethhdr) > data_end)
//         return XDP_DROP;

//     h_proto = eth->h_proto;

//     if (h_proto == htons(ETH_P_IPV6))
//         return XDP_DROP;

//     return XDP_PASS;
// }

char _license[] SEC("license") = "GPL";