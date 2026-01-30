#!/bin/bash

set -e

FREEZE_DURATION_MS=500
THRESHOLD_MS=400
TEST_DURATION_SEC=10

echo "Testing system_freeze kernel module..."

sudo rmmod system_freeze
sudo insmod system_freeze.ko

if [[ $(lsmod | grep -q system_freeze) ]]; then
	echo "FAIL: System Freeze module not found"
	exit 1
fi
echo "System freeze module reloaded."

# Test 1: Verify duration option
echo "Test 1: Verifying -d option (should run for ${TEST_DURATION_SEC} seconds)..."
START_TIME=$(date +%s)
./system_freeze_monitor -t $THRESHOLD_MS -d $TEST_DURATION_SEC -c 0x3 > /dev/null 2>&1
END_TIME=$(date +%s)
ACTUAL_DURATION=$((END_TIME - START_TIME))

if [ $ACTUAL_DURATION -lt $TEST_DURATION_SEC ] || [ $ACTUAL_DURATION -gt $((TEST_DURATION_SEC + 2)) ]; then
    echo "FAIL: Duration test failed. Expected ~${TEST_DURATION_SEC}s, got ${ACTUAL_DURATION}s"
    exit 1
fi
echo "PASS: Duration option works correctly (${ACTUAL_DURATION}s)"

# Test 2: Verify freeze detection
echo "Test 2: Verifying freeze detection..."
./system_freeze_monitor -t $THRESHOLD_MS -d $TEST_DURATION_SEC -c all > /tmp/monitor_output.txt &
MONITOR_PID=$!

sleep 2

echo "Triggering ${FREEZE_DURATION_MS} ms freeze..."
echo $FREEZE_DURATION_MS | sudo tee /sys/kernel/debug/system_freeze/duration_ms > /dev/null
echo 1 | sudo tee /sys/kernel/debug/system_freeze/start > /dev/null

wait $MONITOR_PID

# Check if any events were detected
if ! grep -q "Jump:" /tmp/monitor_output.txt; then
    echo "FAIL: No latency events detected"
    rm -f /tmp/monitor_output.txt
    exit 1
fi

# Extract latency values and verify they're close to expected duration
EXPECTED_US=$((FREEZE_DURATION_MS * 1000))
MIN_US=$((EXPECTED_US - 100000))  # Allow 10ms tolerance
MAX_US=$((EXPECTED_US + 100000))

cat /tmp/monitor_output.txt

FAIL=0
while read -r line; do
    if [[ $line =~ Jump:\ ([0-9]+)\ us ]]; then
        LATENCY_US=${BASH_REMATCH[1]}
        if [ $LATENCY_US -lt $MIN_US ] || [ $LATENCY_US -gt $MAX_US ]; then
            echo "FAIL: Detected latency ${LATENCY_US}us outside expected range ${MIN_US}-${MAX_US}us"
            FAIL=1
        fi
    fi
done < /tmp/monitor_output.txt

rm -f /tmp/monitor_output.txt

if [ $FAIL -eq 0 ]; then
    echo "PASS: Freeze detection works correctly"
    echo ""
    echo "All tests passed successfully"
else
    exit 1
fi


