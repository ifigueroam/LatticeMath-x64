#!/bin/bash

# LatticeMath-x64 Benchmarking Shield
# Provides laboratory-grade isolation for high-fidelity performance metrics.

if [[ $EUID -ne 0 ]]; then
   echo "Error: This script must be run as root to control CPU governors and priority."
   exit 1
fi

echo "--- LatticeMath-x64 Shield Activated ---"

# 1. Power Management: Lockdown to Max Performance
echo "[*] Setting CPU Scaling Governor to 'performance'..."
for gov in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
    echo "performance" > "$gov"
done

# 2. Disable Turbo Boost (Intel)
if [ -f /sys/devices/system/cpu/intel_pstate/no_turbo ]; then
    echo "[*] Disabling Intel Turbo Boost..."
    echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo
fi

# 3. Disable ASLR (Minimize address-space randomization noise)
echo "[*] Disabling ASLR..."
echo 0 > /proc/sys/kernel/randomize_va_space

# 4. Clear Page Caches
echo "[*] Clearing System Caches..."
sync; echo 3 > /proc/sys/vm/drop_caches

# 5. Execute Shielded Benchmark
echo "[*] Executing Benchmark with High Priority (nice -20, taskset core 0)..."
nice -n -20 taskset -c 0 ./Testing/test_00-benchmark

# 6. Restoration
echo "--- Restoring System State ---"
echo "[*] Enabling ASLR..."
echo 2 > /proc/sys/kernel/randomize_va_space
if [ -f /sys/devices/system/cpu/intel_pstate/no_turbo ]; then
    echo "[*] Re-enabling Intel Turbo Boost..."
    echo 0 > /sys/devices/system/cpu/intel_pstate/no_turbo
fi
echo "[*] Resetting CPU Scaling Governor to 'powersave'..."
for gov in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
    echo "powersave" > "$gov"
done

echo "--- Shield Deactivated ---"
