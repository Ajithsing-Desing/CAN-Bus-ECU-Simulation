
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

./dashboard_ecu > "$output" 2>&1 &
dashboard_pid=$!

sleep 1

# Send 11.50 V (11500 mV = 0x2CEC)
cansend vcan0 300#2CEC

sleep 1

# Verify low voltage and warning
if ! grep -q "Dashboard: Battery Voltage = 11.50 V" "$output" ||
   ! grep -q "WARNING: Low battery voltage!" "$output"; then
    echo "FAIL: Low battery warning test"
    cat "$output"
    exit 1
fi

echo "PASS: Low battery warning test"

