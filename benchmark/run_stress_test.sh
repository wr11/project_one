#!/bin/bash
# Stress test runner script

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

STRESS_CLIENT="./ge/build/bin/stress_client"
if [ ! -f "$STRESS_CLIENT" ]; then
    STRESS_CLIENT="./ge/build/bin/stress_client.exe"
fi

if [ ! -f "$STRESS_CLIENT" ]; then
    echo "Error: stress_client not found. Please build the project first."
    echo "Run: cd ge && ./build-msys2.sh"
    exit 1
fi

echo "=== GearEngine Stress Test Runner ==="
echo ""

# Test 1: Basic connection test
echo "Test 1: Basic Connection Test (100 clients, 10 messages each)"
echo "------------------------------------------------------------"
"$STRESS_CLIENT" 100 10 100 localhost 8080
echo ""

# Test 2: High concurrency
echo "Test 2: High Concurrency Test (500 clients, 5 messages each)"
echo "------------------------------------------------------------"
sleep 2
"$STRESS_CLIENT" 500 5 200 localhost 8080
echo ""

# Test 3: Message throughput
echo "Test 3: Message Throughput Test (100 clients, 100 messages, fast)"
echo "-------------------------------------------------------------------"
sleep 2
"$STRESS_CLIENT" 100 100 10 localhost 8080
echo ""

echo "All tests completed!"
