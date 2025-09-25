#!/bin/bash
set -e

IFACE_OPT=${1:-both}
IFACE_FAULT1="vcan_fault1"
IFACE_FAULT2="vcan_fault2"

CORRUPT="70%"

apply_corrupt() {
    local iface=$1
    sudo ip netns exec fault_ns tc qdisc del dev "$iface" root 2>/dev/null || true
    sudo ip netns exec fault_ns tc qdisc add dev "$iface" root netem corrupt $CORRUPT
    echo "[*] Corruption $CORRUPT applied on $iface"
}

case "$IFACE_OPT" in
    fault1)
        apply_corrupt "$IFACE_FAULT1"
        ;;
    fault2)
        apply_corrupt "$IFACE_FAULT2"
        ;;
    both)
        apply_corrupt "$IFACE_FAULT1"
        apply_corrupt "$IFACE_FAULT2"
        ;;
    *)
        echo "[!] Invalid interface option: $IFACE_OPT"
        exit 1
        ;;
esac

