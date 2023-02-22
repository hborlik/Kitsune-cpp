#!/bin/bash

# Before proceeding, it is important to use Linux veth (Virtual Ethernet Device) for testing

# this loads the xdp program on the interface
# this method does not support type maps
sudo ip link set veth1 xdpgeneric obj xdp_drop.o sec xdp_drop


# load the object on a veth interface with xdp-loader. 
# The -m sbk flag is used for generic XDP loading, which does not require a compliant hardware device.
sudo xdp-loader load -m skb -s xdp_drop veth1 xdp_drop.o


# show the running bpf program status
sudo bpftool prog show

# or show it as part of the interface status
sudo ip link show veth1

# or use the xdp-loader status command
sudo xdp-loader status