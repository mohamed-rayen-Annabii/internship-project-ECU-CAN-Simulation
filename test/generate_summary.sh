#!/bin/bash
set -e

LOG_DIR="logs"
FLAG_DIR="test"
RESULTS_FILE="test/results.md"

mkdir -p "$LOG_DIR" "$FLAG_DIR"

echo "# CAN Fault Injection Test Summary" > "$RESULTS_FILE"
echo "" >> "$RESULTS_FILE"
echo "Generated on $(date)" >> "$RESULTS_FILE"
echo "" >> "$RESULTS_FILE"

if compgen -G "$FLAG_DIR/*.flag" > /dev/null; then
    for flagfile in "$FLAG_DIR"/*.flag; do
        fault_name=$(basename "$flagfile" .flag)
        log_file="$LOG_DIR/$fault_name.log"
        echo "## Fault: \`$fault_name\`" >> "$RESULTS_FILE"
        if [[ -f "$log_file" ]]; then
            echo '```' >> "$RESULTS_FILE"
            cat "$log_file" >> "$RESULTS_FILE"
            echo '```' >> "$RESULTS_FILE"
        else
            echo "_No log file found for this fault._" >> "$RESULTS_FILE"
        fi
        echo "" >> "$RESULTS_FILE"
    done
else
    echo "_No fault injections recorded._" >> "$RESULTS_FILE"
fi

echo "[*] Summary generated at $RESULTS_FILE"

