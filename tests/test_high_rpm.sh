
#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

# Check that the virtual CAN interface exists
if ! ip link show vcan0 >/dev/null 2>&1; then
    echo "FAIL: vcan0 is not available"
    exit 1
fi

# Compile the Dashboard
gcc -Wall -Wextra dashboard_ecu.c -o dashboard_ecu

# Capture Dashboard output in a temporary file
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

# CAN ID 0x100, two bytes: 0x1194 = 4500 RPM
cansend vcan0 100#1194

sleep 1

# Verify both expected messages
if grep -q "Dashboard: Engine RPM = 4500" "$output" &&
   grep -q "WARNING: High engine RPM!" "$output"; then
    echo "PASS: High RPM warning test"
else
    echo "FAIL: High RPM warning test"
    cat "$output"
    exit 1
fi
