#!/bin/bash
#
# Runs the emulator port on the PC (see host_sim.c) through a set of
# scripted tests and checks the results.
#
#   tools/hostsim/run_tests.sh
#   BMLISA_ROMS=/path/to/roms tools/hostsim/run_tests.sh
#   BMLISA_ROMS=... BMLISA_LOS=los.zip BMLISA_WORKSHOP=ws.zip \
#       BMLISA_MACWORKS=mw.zip BMLISA_XENIX=xenix.zip BMLISA_XENIX_BOOT=boot.dc42 \
#       BMLISA_FLOPPY=disk.image tools/hostsim/run_tests.sh
#
# The Lisa's ROMs and software are Apple's, so none are shipped and the
# tests need yours: BMLISA_ROMS is a folder with 341-0175-h, 341-0176-h and
# vidstate.rom (as the SD card's roms folder); BMLISA_LOS, BMLISA_WORKSHOP
# and BMLISA_MACWORKS are ProFile hard disk images with the Lisa Office
# System, the Workshop, MacWorks XL and Xenix installed (BMLISA_XENIX_BOOT
# is Xenix's boot floppy) (profile.raw, or a zip holding one: each test
# gets a copy, so they are never written to); BMLISA_FLOPPY is a Disk Copy
# 4.2 floppy image. Tests whose files aren't given are skipped. Output,
# including screenshots (.ppm), goes to tools/hostsim/out/.
#

set -u

HERE=$(cd "$(dirname "$0")" && pwd)
OUT=$HERE/out
SIM=$HERE/build/bmlisa_sim
ROMS=${BMLISA_ROMS:-}
LOS=${BMLISA_LOS:-}
WORKSHOP=${BMLISA_WORKSHOP:-}
MACWORKS=${BMLISA_MACWORKS:-}
XENIX=${BMLISA_XENIX:-}
XENIX_BOOT=${BMLISA_XENIX_BOOT:-}
FLOPPY=${BMLISA_FLOPPY:-}

make -s -C "$HERE" -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)" || exit 1

# Everything from the last run, but live.sh's SD card (out/live).
mkdir -p "$OUT"
find "$OUT" -mindepth 1 -maxdepth 1 ! -name live -exec rm -rf {} +

FAILED=0
SKIPPED=0
pass() { echo "PASS  $1"; }
fail() { echo "FAIL  $1: $2"; FAILED=$((FAILED + 1)); }
skip() { echo "SKIP  $1: $2"; SKIPPED=$((SKIPPED + 1)); }

# card NAME [HARD_DISK]: out/NAME, a stand-in SD card with copies of the
# ROMs and a copy of HARD_DISK (profile.raw, or a zip holding one) as
# disks/profile.raw.
card() {
    local name=$1 disk=${2:-} f
    mkdir -p "$OUT/$name/roms" "$OUT/$name/disks"
    if [ -n "$ROMS" ]; then
        for f in 341-0175-h 341-0176-h L175REVC.bin L176REVC.bin vidstate.rom; do
            [ -f "$ROMS/$f" ] && cp "$ROMS/$f" "$OUT/$name/roms/"
        done
    fi
    case "$disk" in
    "") ;;
    *.zip) unzip -q -o -j "$disk" '*profile.raw' -d "$OUT/$name/disks" ;;
    *) cp "$disk" "$OUT/$name/disks/profile.raw" ;;
    esac
}

# run NAME REFRESH CMDLINE SCRIPT: runs the simulator in out/NAME.
run() {
    local name=$1 refresh=$2 cmdline=$3 script=$4
    (cd "$OUT/$name" && printf "$script" | BMLISA_REFRESH=$refresh \
        BMLISA_CMDLINE="root=. $cmdline" perl -e 'alarm 900; exec @ARGV' "$SIM" \
        > log.txt 2>&1)
}

# check_log NAME REGEX MESSAGE: the log must contain REGEX
check_log() {
    if grep -Eq "$2" "$OUT/$1/log.txt"; then
        pass "$1: $3"
    else
        fail "$1" "$3 (see out/$1/log.txt)"
    fi
}

# no_log NAME REGEX MESSAGE: the log must not contain REGEX
no_log() {
    if grep -Eq "$2" "$OUT/$1/log.txt"; then
        fail "$1" "$3 (see out/$1/log.txt)"
    else
        pass "$1: $3"
    fi
}

# same NAME A B / differ NAME A B: compare two screenshots of out/NAME.
same() {
    if [ -f "$OUT/$1/$2.ppm" ] && [ -f "$OUT/$1/$3.ppm" ] && cmp -s "$OUT/$1/$2.ppm" "$OUT/$1/$3.ppm"; then
        pass "$1: $2 and $3 are the same"
    else
        fail "$1" "$2 and $3 differ"
    fi
}
differ() {
    if [ -f "$OUT/$1/$2.ppm" ] && [ -f "$OUT/$1/$3.ppm" ] && ! cmp -s "$OUT/$1/$2.ppm" "$OUT/$1/$3.ppm"; then
        pass "$1: $2 and $3 differ"
    else
        fail "$1" "$2 and $3 should differ"
    fi
}

# has_colour NAME FILE R G B MESSAGE: the screenshot has a pixel of that colour
has_colour() {
    if python3 - "$OUT/$1/$2.ppm" "$3" "$4" "$5" <<'EOF'
import sys
data = open(sys.argv[1], 'rb').read()
fields, pos = [], 0
while len(fields) < 4:
    while data[pos:pos + 1].isspace():
        pos += 1
    end = pos
    while not data[end:end + 1].isspace():
        end += 1
    fields.append(data[pos:end])
    pos = end
pixels = data[pos + 1:]
want = bytes(int(v) for v in sys.argv[2:5])
sys.exit(0 if any(pixels[i:i + 3] == want for i in range(0, len(pixels), 3)) else 1)
EOF
    then
        pass "$1: $6"
    else
        fail "$1" "$6 (no $3,$4,$5 pixel in $2.ppm)"
    fi
}

# ---- No ROMs ------------------------------------------------------------------

ROMS_GIVEN=$ROMS
ROMS=
card noroms
run noroms 60 "" "120 shot screen.ppm\n130 quit\n"
check_log noroms "no Lisa ROMs in roms/" "says the ROMs are missing"
ROMS=$ROMS_GIVEN

if [ -z "$ROMS" ] || [ ! -f "$ROMS/341-0175-h" ]; then
    skip "lisa" "set BMLISA_ROMS to a folder with the Lisa 2's ROMs to test the Lisa"
    echo
    echo "$FAILED failed, $SKIPPED skipped"
    [ $FAILED -eq 0 ]
    exit
fi

# ---- A new card: a blank hard disk is made, the boot ROM runs -----------------

card blank
run blank 60 "" "30 shot start.ppm\n600 shot rom.ppm\n610 quit\n"
check_log blank "made a blank hard disk" "makes a blank hard disk"
if [ "$(wc -c < "$OUT/blank/disks/profile.raw" | tr -d ' ')" = "10350592" ]; then
    pass "blank: the blank hard disk is 10 MB (19456 blocks of 532 bytes)"
else
    fail "blank" "the blank hard disk is the wrong size"
fi
check_log blank "Lisa 2, ROMs 341-0175-h" "starts the Lisa 2"
differ blank start rom
has_colour blank rom 255 255 255 "the boot ROM draws its screen"
no_log blank "stopped" "the Lisa runs"

# No hard disk (ejected in the menu: harddisk=none): no blank one is made,
# and the boot ROM finds no ProFile.
card nohd
run nohd 60 "harddisk=none" "1800 shot rom.ppm\n1810 quit\n"
check_log nohd "no hard disk" "starts with no hard disk"
no_log nohd "stopped" "the Lisa runs without one"
if [ ! -e "$OUT/nohd/disks/profile.raw" ]; then
    pass "nohd: no blank hard disk made"
else
    fail "nohd" "a blank hard disk was made"
fi
has_colour nohd rom 255 255 255 "the boot ROM draws its screen"

# A hard disk image of the wrong size isn't used (the Lisa only showed its
# error 84 for it).
card wrongsize
head -c 100000 /dev/zero > "$OUT/wrongsize/disks/profile.raw"
run wrongsize 60 "" "120 quit\n"
check_log wrongsize "isn't a ProFile image" "a hard disk of the wrong size is refused"

# LisaEm's hard disks (Disk Copy 4.2: data, then tags, the blocks
# interleaved): disks/profile.dc42 is used when there is no profile.raw.
# tools/hostsim/dc42.py makes one from a raw image (blank here).
card lisaem
head -c 10350592 /dev/zero > "$OUT/lisaem/blank.raw"
python3 "$HERE/dc42.py" "$OUT/lisaem/blank.raw" "$OUT/lisaem/disks/profile.dc42"
rm "$OUT/lisaem/blank.raw"
run lisaem 60 "" "600 shot rom.ppm\n610 quit\n"
check_log lisaem "hard disk \\./disks/profile\\.dc42" "uses LisaEm's disks/profile.dc42"
no_log lisaem "isn't a ProFile image|stopped" "the Lisa runs from it"
if [ ! -e "$OUT/lisaem/disks/profile.raw" ]; then
    pass "lisaem: no profile.raw made beside it"
else
    fail "lisaem" "a blank profile.raw was made"
fi

# The parameter memory (the Lisa's battery-backed settings) is read from
# bmlisa.pram at start, and isn't written while it doesn't change.
card pram
printf 'BMLisa test parameter memory: 64 bytes of settings..............' > "$OUT/pram/bmlisa.pram"
cp "$OUT/pram/bmlisa.pram" "$OUT/pram/pram.before"
run pram 60 "" "600 quit\n"
check_log pram "parameter memory from \\./bmlisa\\.pram" "reads the parameter memory"
if cmp -s "$OUT/pram/bmlisa.pram" "$OUT/pram/pram.before"; then
    pass "pram: left as it was"
else
    fail "pram" "bmlisa.pram changed"
fi
# One of the wrong size isn't used, nor written over while the settings
# don't change.
card pram_short
printf 'too short' > "$OUT/pram_short/bmlisa.pram"
run pram_short 60 "" "600 quit\n"
check_log pram_short "isn't 64 bytes: not used" "a parameter memory of the wrong size isn't used"
if [ "$(cat "$OUT/pram_short/bmlisa.pram")" = "too short" ]; then
    pass "pram_short: left as it was"
else
    fail "pram_short" "bmlisa.pram written over"
fi

# The monitor: green.
card green
run green 60 "monitor=green" "600 shot rom.ppm\n610 quit\n"
has_colour green rom 49 255 49 "a green screen with monitor=green"

# The menu opens (F12) and closes (Esc); the Lisa then goes on.
card menu
run menu 60 "" "300 key 69 1\n302 key 69 0\n340 shot menu.ppm\n360 key 41 1\n362 key 41 0\n400 shot after.ppm\n410 quit\n"
has_colour menu menu 255 129 0 "the menu shows its selection bar"
differ menu menu after

# ---- The Lisa Office System ---------------------------------------------------

if [ -n "$LOS" ]; then
    card los "$LOS"
    # Boot to the desktop; the mouse opens the Desk menu; the Lisa's
    # power button (F10) shuts it down and it switches itself off; a key
    # switches it on again.
    script="1200 shot boot.ppm\n4500 shot desk.ppm\n"
    for i in 0 1 2 3 4 5 6 7 8 9; do
        script="$script$((4510 + i * 2)) mouse 4 1 0\n"
    done
    script="${script}4570 mouse 0 0 1\n4620 shot menu.ppm\n4630 mouse 0 0 0\n4700 shot closed.ppm\n"
    script="${script}4800 key 67 1\n4805 key 67 0\n7000 shot off.ppm\n7010 key 4 1\n7015 key 4 0\n7300 shot on.ppm\n7310 quit\n"
    run los 60 "" "$script"
    differ los boot desk
    differ los desk menu
    differ los menu closed
    check_log los "the Lisa switched itself off" "shuts down with the power button"
    has_colour los off 0 0 0 "dark once off"
    differ los off on
    no_log los "processor stopped" "the processor never stops"
else
    skip "los" "set BMLISA_LOS to a hard disk image with the Lisa Office System"
fi

# The Lisa Office System from a LisaEm hard disk (the same disk, made
# LisaEm's): it boots and shuts down, and Disk Copy's checksums are right
# after its writes.
if [ -n "$LOS" ]; then
    card los_dc42
    case "$LOS" in
    *.zip) unzip -p "$LOS" '*profile.raw' > "$OUT/los_dc42/los.raw" ;;
    *) cp "$LOS" "$OUT/los_dc42/los.raw" ;;
    esac
    python3 "$HERE/dc42.py" "$OUT/los_dc42/los.raw" "$OUT/los_dc42/disks/profile.dc42"
    rm "$OUT/los_dc42/los.raw"
    run los_dc42 60 "" "4500 shot desk.ppm\n4510 key 67 1\n4515 key 67 0\n7000 shot off.ppm\n7010 quit\n"
    differ los_dc42 desk off
    check_log los_dc42 "the Lisa switched itself off" "boots and shuts down from a LisaEm disk"
    if python3 "$HERE/dc42.py" --check "$OUT/los_dc42/disks/profile.dc42"; then
        pass "los_dc42: Disk Copy's checksums are right"
    else
        fail "los_dc42" "Disk Copy's checksums are wrong"
    fi
else
    skip "los_dc42" "set BMLISA_LOS to a hard disk image with the Lisa Office System"
fi

# Another hard disk while the Lisa runs (the web UI): the Lisa starts again
# from it, on a ProFile switched off and on (the boot ROM gave its error 52,
# or the processor halted); then a plain restart.
if [ -n "$LOS" ]; then
    card switch "$LOS"
    head -c 10350592 /dev/zero > "$OUT/switch/disks/blank.raw"
    run switch 60 "" "4500 shot desk.ppm\n4510 run ./disks/blank.raw\n5400 shot blank.ppm\n5410 webui coldstart\n6300 shot again.ppm\n6310 quit\n"
    check_log switch "hard disk \\./disks/blank\\.raw" "takes another hard disk while running"
    no_log switch "stopped" "the processor never stops"
    differ switch desk blank
    has_colour switch again 255 255 255 "the boot ROM runs after a restart"
else
    skip "switch" "set BMLISA_LOS to a hard disk image with the Lisa Office System"
fi

if [ -n "$LOS" ] && [ -n "$FLOPPY" ]; then
    card floppy "$LOS"
    cp "$FLOPPY" "$OUT/floppy/disks/floppy.image"
    run floppy 60 "" "4500 shot before.ppm\n4510 run ./disks/floppy.image\n4900 shot after.ppm\n4910 quit\n"
    check_log floppy "floppy ./disks/floppy.image in drive 1" "inserts the floppy (web UI)"
    differ floppy before after
else
    skip "floppy" "set BMLISA_LOS and BMLISA_FLOPPY to test a floppy"
fi

# ---- MacWorks XL: the Macintosh's Finder ---------------------------------------

if [ -n "$MACWORKS" ]; then
    card macworks "$MACWORKS"
    # To the File menu, hold the button, down to Open (the hard disk is
    # selected), let go: the hard disk's window opens.
    run macworks 60 "" "3600 shot finder.ppm\n3700 mouse 10 2 0\n3702 mouse 10 2 0\n3704 mouse 10 2 0\n3706 mouse 10 2 0\n3708 mouse 10 0 0\n3740 mouse 0 0 1\n3800 shot menu.ppm\n3801 mouse 0 10 1\n3803 mouse 0 10 1\n3805 mouse 0 10 1\n3807 mouse 0 6 1\n3840 mouse 0 0 0\n4400 shot window.ppm\n4410 quit\n"
    differ macworks finder menu
    differ macworks finder window
    no_log macworks "processor stopped" "the processor never stops"
else
    skip "macworks" "set BMLISA_MACWORKS to a hard disk image with MacWorks XL"
fi

# ---- Xenix: from its boot floppy -------------------------------------------------

if [ -n "$XENIX" ] && [ -n "$XENIX_BOOT" ]; then
    card xenix "$XENIX"
    cp "$XENIX_BOOT" "$OUT/xenix/disks/boot.dc42"
    # An installed Xenix hard disk isn't started by the boot ROM (error 75):
    # Apple+3 (Startup From...), Apple+2 (the floppy), and at Xenix's boot
    # loader, pf(0,0)xenix (it misses the first key typed: an x, deleted).
    boot='3600 key 224 1\n3604 key 32 1\n3608 key 32 0\n3612 key 224 0\n3700 key 224 1\n3704 key 31 1\n3708 key 31 0\n3712 key 224 0\n5400 type x\n5420 type x\n5440 key 42 1\n5444 key 42 0\n5460 type pf(0,0)xenix\n5570 key 40 1\n5574 key 40 0\n'
    # The image wasn't shut down: y cleans its file system, and Xenix shuts
    # down.
    run xenix 60 "harddisk=disks/profile.raw floppy=disks/boot.dc42" "${boot}10800 shot clean.ppm\n10800 type y\n10810 key 40 1\n10814 key 40 0\n18000 quit\n"
    cp "$OUT/xenix/log.txt" "$OUT/xenix/log1.txt"
    # Then it starts: Apple+d is Control-d (normal startup), Return keeps
    # the time, root logs in, Return keeps the terminal, ls.
    run xenix 60 "harddisk=disks/profile.raw floppy=disks/boot.dc42" "${boot}10800 shot single.ppm\n10800 key 224 1\n10804 key 7 1\n10808 key 7 0\n10812 key 224 0\n13000 key 40 1\n13004 key 40 0\n16000 shot login.ppm\n16010 type root\n16100 key 40 1\n16104 key 40 0\n17500 key 40 1\n17504 key 40 0\n18000 shot shell.ppm\n18010 type ls /\n18100 key 40 1\n18104 key 40 0\n19000 shot ls.ppm\n19010 quit\n"
    differ xenix clean single
    differ xenix single login
    differ xenix login shell
    differ xenix shell ls
    no_log xenix "processor stopped" "the processor never stops"
else
    skip "xenix" "set BMLISA_XENIX and BMLISA_XENIX_BOOT (its boot floppy) to test Xenix"
fi

# ---- The Workshop: typing ------------------------------------------------------

if [ -n "$WORKSHOP" ]; then
    card workshop "$WORKSHOP"
    # F (File Manager), L (List), Return.
    run workshop 60 "" "5400 shot prompt.ppm\n5402 key 9 1\n5406 key 9 0\n5500 shot filemgr.ppm\n5510 key 15 1\n5514 key 15 0\n5600 key 40 1\n5604 key 40 0\n5800 shot list.ppm\n5810 quit\n"
    differ workshop prompt filemgr
    differ workshop filemgr list
else
    skip "workshop" "set BMLISA_WORKSHOP to a hard disk image with the Workshop"
fi

echo
echo "$FAILED failed, $SKIPPED skipped"
[ $FAILED -eq 0 ]
