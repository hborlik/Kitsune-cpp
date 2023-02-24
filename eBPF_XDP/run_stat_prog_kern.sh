#!/bin/bash

sudo ../build/eBPF_XDP/eBPF_loader --filename ../build/eBPF_XDP/basic03-map-counter/xdp_prog_kern --progsec xdp_stats_func --dev wlp4s0 --skb-mode