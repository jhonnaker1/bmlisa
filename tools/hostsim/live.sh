#!/bin/bash
#
# Runs BMLisa in the host simulator in real time, shown in a web page:
#
#   tools/hostsim/live.sh ROM_DIRECTORY [PORT]
#
# then open http://127.0.0.1:PORT/ (default port 8064). ROM_DIRECTORY holds
# your Lisa ROMs (341-0175-h, 341-0176-h and vidstate.rom; see
# docs/ROMS.txt). The simulator's SD card is tools/hostsim/out/live: its
# hard disk is disks/profile.raw there (a blank one is made if there is
# none: copy yours over it), and floppy images put in its disks folder can
# be inserted from the F12 menu. Choosing Reboot (or Shut Down) in the menu
# starts it again. Ctrl+C stops it.
#
set -u

HERE=$(cd "$(dirname "$0")" && pwd)
ROMS=${1:-}
PORT=${2:-8064}
CARD=$HERE/out/live

if [ -z "$ROMS" ]; then
    echo "usage: $0 ROM_DIRECTORY [PORT]" >&2
    exit 2
fi

make -s -C "$HERE" -j4 || exit 1

mkdir -p "$CARD"/disks
# IDLE only reads the ROMs, so a link to the folder is safe.
[ -L "$CARD/roms" ] && unlink "$CARD/roms"
ln -s "$ROMS" "$CARD/roms"

cd "$CARD" || exit 1
while true; do
    BMLISA_LIVE=$PORT BMLISA_CMDLINE="root=$CARD ${BMLISA_CMDLINE:-}" \
        "$HERE/build/bmlisa_sim" < /dev/null
    status=$?
    [ $status -ne 0 ] && exit $status
    echo "[live] restarting"
    sleep 1
done
