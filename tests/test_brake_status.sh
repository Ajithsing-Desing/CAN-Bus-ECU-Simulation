
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

# Start Dashboard
./dashboard_ecu > "$output" 2>&1 &
dashboard_pid=$!

sleep 1

# Send brake RELEASED (CAN ID 0x200, value 00)
cansend vcan0 200#00

sleep 1

# Send brake PRESSED (CAN ID 0x200, value 01)
cansend vcan0 200#01

sleep 1

# Verify both states
if grep -q "Dashboard: Brake RELEASED" "$output" &&
   grep -q "Dashboard: Brake PRESSED" "$output"; then
    echo "PASS: Brake status test"
else
    echo "FAIL: Brake status test"
    cat "$output"
    exit 1
fi
