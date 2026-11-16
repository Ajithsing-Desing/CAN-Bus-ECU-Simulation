
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

# 12.60 V = 12600 mV = 0x3138
cansend vcan0 300#3138

sleep 1

if ! grep -q "Dashboard: Battery Voltage = 12.60 V" "$output"; then
    echo "FAIL: Normal battery voltage not received"
    cat "$output"
    exit 1
fi

if grep -q "WARNING: Low battery voltage!" "$output"; then
    echo "FAIL: Unexpected low battery warning"
    cat "$output"
    exit 1
fi

echo "PASS: Normal battery voltage test"
