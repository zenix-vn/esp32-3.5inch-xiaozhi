#!/bin/sh
# Kindle Desk client. Copy to /mnt/us/desk/desk.sh, chmod +x, run from KUAL or SSH.
#   desk.sh start   : desk mode (hides the Kindle UI, redraws every minute)
#   desk.sh stop    : back to the normal Kindle UI
#   desk.sh draw    : fetch + draw once
SERVER="${SERVER:-http://10.44.1.43:8080}"   # <-- IP of the PC running server.py
FBINK="${FBINK:-/mnt/us/koreader/fbink}"       # any fbink binary (KOReader, MRPI, libkh)
BTN_DEV="${BTN_DEV:-/dev/input/event1}"        # page-turn keys; find yours with: evtest
FULL_EVERY=10                                  # full flashing refresh every N draws (anti-ghosting)
IMG=/tmp/desk.png
CNT=/tmp/desk.cnt

draw() {
  wget -q -T 15 -O "$IMG.new" "$SERVER/screen.png" && mv "$IMG.new" "$IMG" || return 1
  n=$(( $(cat $CNT 2>/dev/null || echo 0) + 1 )); echo $n > $CNT
  if [ $((n % FULL_EVERY)) -eq 1 ]; then F="-f"; else F=""; fi
  "$FBINK" -q $F -g file="$IMG",w=0,h=0,halign=center,valign=center
}

keys() {  # page-turn buttons: down = next card, up = flip (needs evtest)
  command -v evtest >/dev/null || return
  evtest "$BTN_DEV" 2>/dev/null | while read -r l; do
    case "$l" in
      *"KEY_PAGEDOWN"*"value 1"*|*"code 109"*"value 1"*) wget -q -O /dev/null "$SERVER/next"; draw ;;
      *"KEY_PAGEUP"*"value 1"*|*"code 104"*"value 1"*)   wget -q -O /dev/null "$SERVER/flip"; draw ;;
    esac
  done
}

case "$1" in
  draw) draw ;;
  start)
    initctl stop framework 2>/dev/null || stop framework
    lipc-set-prop com.lab126.powerd preventScreenSaver 1 2>/dev/null
    lipc-set-prop com.lab126.cmd wirelessEnable 1 2>/dev/null
    "$FBINK" -q -c
    keys &
    echo $! > /tmp/desk.keys.pid
    while true; do
      draw
      sleep $((60 - $(date +%S)))     # wake on the minute so the clock is exact
    done ;;
  stop)
    kill "$(cat /tmp/desk.keys.pid 2>/dev/null)" 2>/dev/null
    lipc-set-prop com.lab126.powerd preventScreenSaver 0 2>/dev/null
    initctl start framework 2>/dev/null || start framework ;;
  *) echo "usage: $0 start|stop|draw" ;;
esac
