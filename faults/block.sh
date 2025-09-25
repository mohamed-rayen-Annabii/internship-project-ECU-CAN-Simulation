#!/bin/bash
set -e

IFACE_OPT=${1:-both}
IFACE_FAULT1="vcan_fault1"
IFACE_FAULT2="vcan_fault2"

apply_block() {
    local iface=$1
    sudo ip netns exec fault_ns iptables -F OUTPUT 2>/dev/null || true
    sudo ip netns exec fault_ns iptables -A OUTPUT -o "$iface" -j DROP
    echo "[*] Block applied on $iface"
}

case "$IFACE_OPT" in
    fault1)
        apply_block "$IFACE_FAULT1"
        ;;
    fault2)
        apply_block "$IFACE_FAULT2"
        ;;
    both)
        apply_block "$IFACE_FAULT1"
        apply_block "$IFACE_FAULT2"
        ;;
    *)
        echo "[!] Invalid interface option: $IFACE_OPT"
        exit 1
        ;;
esac

