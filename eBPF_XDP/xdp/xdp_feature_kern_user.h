/* SPDX-License-Identifier: GPL-2.0 */


#ifndef __XDP_FEATURE_KERN_USER_H
#define __XDP_FEATURE_KERN_USER_H

/* This is the session record stored in the map */
struct session {
	__u64           rx_bytes;       /* session bytes */
	unsigned char	h_dest[6];	    /* destination ether addr MAC*/
	unsigned char	h_source[6];	/* source ether addr MAC*/
    int             eth_type;       /* ip type either ETH_P_IP, or ETH_P_IPV6 */
    int             proto_type;     /* protocol type IPPROTO_UDP, IPPROTO_TCP, IPPROTO_ICMPV6, or IPPROTO_ICMP */
    __u16           source_port;    /* source port if proto_type is IPPROTO_UDP or IPPROTO_TCP */
	__u16           dest_port;      /* dest port if proto_type is IPPROTO_UDP or IPPROTO_TCP */
};

#endif /* __XDP_FEATURE_KERN_USER_H */