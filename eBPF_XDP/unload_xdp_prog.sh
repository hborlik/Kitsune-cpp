#!/bin/bash

# If you loaded the program using ip command, unload it like this.
# Note you should use the same flags you used when loading the program.
sudo ip link set veth1 xdpgeneric off

# or, if loaded using xdp-loader, use this
sudo xdp-loader unload -a veth1

