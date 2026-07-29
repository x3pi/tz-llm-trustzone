#!/bin/bash
# uart_cmd.sh "<command>" [seconds]
# Sends a command to the board console and captures the reply.
DEV=/dev/ttyUSB0
CMD="$1"
SECS="${2:-5}"
stty -F $DEV 1500000 raw -echo -echoe -echok
OUT=$(mktemp)
timeout "$SECS" cat $DEV > "$OUT" &
CATPID=$!
sleep 0.3
printf '%s\r\n' "$CMD" > $DEV
wait $CATPID
cat -v "$OUT" | sed 's/\^M$//'
rm -f "$OUT"
