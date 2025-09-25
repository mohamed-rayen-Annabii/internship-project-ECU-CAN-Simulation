#!/bin/bash
set -e

# Cleanup previous namespaces
sudo ip netns del ecu1_ns 2>/dev/null || true
sudo ip netns del ecu2_ns 2>/dev/null || true
sudo ip netns del fault_ns 2>/dev/null || true

# Create namespaces
sudo ip netns add ecu1_ns
sudo ip netns add ecu2_ns
sudo ip netns add fault_ns

# Create vxcan pairs
sudo ip link add vcan_tunnel_a type vxcan peer name vcan_fault1
sudo ip link add vcan_tunnel_b type vxcan peer name vcan_fault2

# Assign interfaces to namespaces
sudo ip link set vcan_tunnel_a netns ecu1_ns
sudo ip link set vcan_tunnel_b netns ecu2_ns
sudo ip link set vcan_fault1 netns fault_ns
sudo ip link set vcan_fault2 netns fault_ns

# Bring interfaces up
sudo ip netns exec ecu1_ns ip link set vcan_tunnel_a up
sudo ip netns exec ecu2_ns ip link set vcan_tunnel_b up
sudo ip netns exec fault_ns ip link set vcan_fault1 up
sudo ip netns exec fault_ns ip link set vcan_fault2 up
sudo modprobe can-gw

# Start cangw forwarding inside fault_ns (bidirectional)
sudo ip netns exec fault_ns cangw -A -s vcan_fault1 -d vcan_fault2 -e &
sudo ip netns exec fault_ns cangw -A -s vcan_fault2 -d vcan_fault1 -e &

echo "Setup complete with cangw forwarding!"
echo "Try sending CAN frames from ecu1_ns and receiving on ecu2_ns."

