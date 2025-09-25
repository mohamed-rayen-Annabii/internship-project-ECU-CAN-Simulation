#!/bin/bash
set -e

IFACE_OPT=$1
shift

FLAG_DIR="test"
LOG_DIR="test"
mkdir -p "$FLAG_DIR" "$LOG_DIR"

IFACE_FAULT1="vcan_fault1"
IFACE_FAULT2="vcan_fault2"

TC_CMD_FAULT1="sudo ip netns exec fault_ns tc qdisc add dev $IFACE_FAULT1 root netem"
TC_CMD_FAULT2="sudo ip netns exec fault_ns tc qdisc add dev $IFACE_FAULT2 root netem"
BLOCK_CMD=""
COMBO=$(IFS=+; echo "$*")
details=()
tc_needed=false

for arg in "$@"; do
    case "$arg" in
        delay)
            TC_CMD_FAULT1="$TC_CMD_FAULT1 delay 300ms"
            TC_CMD_FAULT2="$TC_CMD_FAULT2 delay 300ms"
            details+=("300ms delay injected")
            tc_needed=true
            ;;
        loss)
            TC_CMD_FAULT1="$TC_CMD_FAULT1 loss 36%"
            TC_CMD_FAULT2="$TC_CMD_FAULT2 loss 36%"
            details+=("10% packet loss injected")
            tc_needed=true
            ;;
        corrupt|corruption)
            TC_CMD_FAULT1="$TC_CMD_FAULT1 corrupt 75%"
            TC_CMD_FAULT2="$TC_CMD_FAULT2 corrupt 75%"
            details+=("70% corruption injected")
            tc_needed=true
            ;;
        block)
            BLOCK_CMD="sudo ip netns exec fault_ns iptables -A OUTPUT -o $IFACE_FAULT1 -j DROP"
            details+=("Block applied on $IFACE_FAULT1")
            ;;
        *)
            echo "[!] Unknown fault type: $arg"
            exit 1
            ;;
    esac
done

if $tc_needed; then
    case "$IFACE_OPT" in
        fault1)
            sudo ip netns exec fault_ns tc qdisc del dev $IFACE_FAULT1 root 2>/dev/null || true
            echo "[*] Applying netem: $TC_CMD_FAULT1 on $IFACE_FAULT1"
            eval "$TC_CMD_FAULT1"
            ;;
        fault2)
            sudo ip netns exec fault_ns tc qdisc del dev $IFACE_FAULT2 root 2>/dev/null || true
            echo "[*] Applying netem: $TC_CMD_FAULT2 on $IFACE_FAULT2"
            eval "$TC_CMD_FAULT2"
            ;;
        both)
            sudo ip netns exec fault_ns tc qdisc del dev $IFACE_FAULT1 root 2>/dev/null || true
            sudo ip netns exec fault_ns tc qdisc del dev $IFACE_FAULT2 root 2>/dev/null || true
            echo "[*] Applying netem: $TC_CMD_FAULT1 on $IFACE_FAULT1"
            eval "$TC_CMD_FAULT1"
            echo "[*] Applying netem: $TC_CMD_FAULT2 on $IFACE_FAULT2"
            eval "$TC_CMD_FAULT2"
            ;;
        *)
            echo "[!] Invalid interface option: $IFACE_OPT. Use fault1, fault2, or both."
            exit 1
            ;;
    esac
fi

if [[ -n "$BLOCK_CMD" ]]; then
    echo "[*] Applying block: $BLOCK_CMD"
    eval "$BLOCK_CMD"
fi

FLAG_FILE="$FLAG_DIR/$COMBO.flag"
touch "$FLAG_FILE"

LOG_FILE="$LOG_DIR/$COMBO.log"
detail=$(IFS=", "; echo "${details[*]}")
echo "PASSED: $detail on $IFACE_OPT" > "$LOG_FILE"

