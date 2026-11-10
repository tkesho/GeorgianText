#!/usr/bin/env bash
# Compile every example for Uno, Nano and ESP32 and print flash/RAM usage.
set -u
LIB="$(cd "$(dirname "$0")/.." && pwd)"
BOARDS=("arduino:avr:uno" "arduino:avr:nano" "esp32:esp32:esp32")
fail=0
printf "%-20s %-20s %-8s %s\n" EXAMPLE BOARD RESULT USAGE
for ex in "$LIB"/examples/*/; do
  name=$(basename "$ex")
  for b in "${BOARDS[@]}"; do
    out=$(arduino-cli compile --fqbn "$b" --library "$LIB" --warnings all "$ex" 2>&1)
    rc=$?
    if [ $rc -eq 0 ]; then
      fl=$(echo "$out" | grep -oE "Sketch uses [0-9]+ bytes \([0-9]+%\)" | grep -oE "[0-9]+ bytes \([0-9]+%\)")
      ram=$(echo "$out" | grep -oE "Global variables use [0-9]+ bytes \([0-9]+%\)" | grep -oE "[0-9]+ bytes \([0-9]+%\)")
      printf "%-20s %-20s %-8s flash %s | RAM %s\n" "$name" "$b" OK "$fl" "$ram"
      w=$(echo "$out" | grep -E "GeorgianLCD/src|$name.ino" | grep -iE "warning" | head -3)
      [ -n "$w" ] && echo "   warnings: $w"
    else
      printf "%-20s %-20s %-8s\n" "$name" "$b" FAIL; echo "$out" | grep -E "error|Error" | head -8; fail=1
    fi
  done
done
exit $fail
