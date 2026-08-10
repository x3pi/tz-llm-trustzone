#!/bin/bash
# Re-establish hdc + runtime tuning + restart the fake daemon after a
# board reboot (manual power-cycle or the new softlockup_panic auto-reboot).
# Board must already be reachable on the LAN (wifi up) before running this.
#
# Usage: BOARD_IP=192.168.1.58 ./scripts/kick-the-tires/resume-daemon.sh
set -e
BOARD_IP=${BOARD_IP:-192.168.1.58}
UART_DEV=${UART_DEV:-/dev/ttyUSB0}

echo "=== enabling hdc over TCP via UART ==="
stty -F "$UART_DEV" 1500000 raw -echo -echoe -echok
printf 'param set persist.hdc.port 8710 && param set ohos.ctl.stop hdcd && /system/bin/hdcd -t &\r\n' > "$UART_DEV"
sleep 3

echo "=== connecting hdc ==="
hdc tconn "$BOARD_IP:8710"
sleep 1

echo "=== mounting /data/ssd + applying runtime tuning ==="
hdc shell "mount -t ext4 /dev/block/nvme0n1p1 /data/ssd 2>&1; \
echo 60 > /proc/sys/kernel/watchdog_thresh; \
echo performance > /sys/devices/system/cpu/cpufreq/policy4/scaling_governor; \
echo performance > /sys/devices/system/cpu/cpufreq/policy6/scaling_governor; \
echo 100000 > /sys/devices/system/cpu/cpufreq/policy0/schedutil/rate_limit_us; \
echo 1 > /proc/sys/kernel/softlockup_panic; \
echo 5 > /proc/sys/kernel/panic; \
echo 1 > /proc/sys/kernel/panic_on_rcu_stall"

echo "=== starting daemon ==="
hdc shell "cd /data/ssd/rknpu && export LD_LIBRARY_PATH=/data/ssd/rknpu:\$LD_LIBRARY_PATH && \
nohup ./ld-linux-aarch64.so.1 ./fake -m tinyllama -s 0 > /data/ssd/fake_run_resume.log 2>&1 & echo STARTED_PID=\$!"

echo "=== polling for HTTP API to become reachable (network drops fast after warmup, so retry tight) ==="
PROMPT=${1:-"What is 2+2?"}
for i in $(seq 1 120); do
    RESP=$(timeout 2 curl -s -m 1.5 -X POST "http://$BOARD_IP:8080/completion" \
        -H "Content-Type: application/json" \
        -d "{\"prompt\":\"$PROMPT\"}" 2>/dev/null)
    if [ -n "$RESP" ]; then
        echo "=== SUCCESS after $i attempts ==="
        echo "$RESP"
        exit 0
    fi
    sleep 0.5
done
echo "=== gave up after 120 attempts (~90s) -- network/daemon not reachable in time ==="
exit 1
