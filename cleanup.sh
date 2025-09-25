#!/bin/bash
set -e

# Delete namespaces if they exist
sudo ip netns del ecu1_ns 2>/dev/null || true
sudo ip netns del ecu2_ns 2>/dev/null || true
sudo ip netns del fault_ns 2>/dev/null || true

# Since vxcan interfaces are inside namespaces, deleting namespaces removes interfaces.
# But just in case, delete any vxcan links in root namespace if any remain:
sudo ip link del vcan_tunnel_a 2>/dev/null || true
sudo ip link del vcan_tunnel_b 2>/dev/null || true
sudo ip link del vcan_fault1 2>/dev/null || true
sudo ip link del vcan_fault2 2>/dev/null || true

# Kill any lingering cangw processes (the CAN gateway forwarding in fault_ns)
sudo pkill -f "cangw -A" 2>/dev/null || true

# Clean logs and flags
rm -rf logs/*.log test/*.flag || true

echo "[*] Cleanup done."

