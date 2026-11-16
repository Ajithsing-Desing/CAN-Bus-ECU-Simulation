
#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

if ! ip link show vcan0 >/dev/null 2>&1; then
    echo "FAIL: vcan0 is not available"
    exit 1
fi

gcc -Wall -Wextra dashboard_ecu.c -o dashboard_ecu

output=$(mktemp)
dashboard_pid=""

cleanup() {
    if [[ -n "$dashboard_pid" ]]; then
        kill "$dashboard_pid" 2>/dev/null || true
        wait "$dashboard_pid" 2>/dev/null || true
    fi
    rm -f "$output"
}
trap cleanup EXIT

# Start Dashboard in background
./dashboard_ecu > "$output" 2>&1 &
dashboard_pid=$!

sleep 1

# Invalid Engine frame: only 1 byte instead of 2
cansend vcan0 100#FF

sleep 1

# Check that Dashboard rejects the invalid frame
if grep -q "Invalid Engine ECU frame" "$output"; then
    echo "PASS: Invalid CAN frame rejected"
else
    echo "FAIL: Invalid CAN frame test"
    cat "$output"
    exit 1
fi
