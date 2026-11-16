
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

# Wait long enough for the 3-second timeout
sleep 4

# Check that timeout was detected
if ! grep -q "WARNING: Engine ECU communication timeout!" "$output"; then
    echo "FAIL: Engine timeout not detected"
    cat "$output"
    exit 1
fi

echo "PASS: Engine communication timeout detected"

# Send a valid engine message: 1000 RPM = 0x03E8
cansend vcan0 100#03E8

sleep 1

# Check that communication was restored
if grep -q "Engine ECU communication restored!" "$output" &&
   grep -q "Dashboard: Engine RPM = 1000" "$output"; then
    echo "PASS: Engine communication recovery"
else
    echo "FAIL: Engine communication recovery"
    cat "$output"
    exit 1
fi
