# Testing and debugging BMLisa on a PC

BMLisa isn't tested in QEMU. On the Pi it runs bare metal, on the Circle
kernel with no Linux underneath, and QEMU's Raspberry Pi machines don't
emulate enough of the Pi (its VideoCore display, HDMI sound, USB and Wi-Fi
as BMLisa uses them) to run it. There would be nothing to strace either:
strace follows a Linux program's system calls, and BMLisa makes none.

What BMLisa has instead is a **PC simulator** (`tools/hostsim`, from
BMCoco and BMApple2): the same emulator code that runs on the Pi (IDLE and
BMLisa's port of it, `src/port/`), built as an ordinary program for Linux or
macOS, with a simulated display, keyboard, mouse and SD card in place of the
Pi's. Because it's an ordinary program, **gdb, lldb, strace and the rest
work on it** as on any other program.

## What it can and can't show

The simulator runs the Lisa emulation, the menu, the disks (the ProFile
hard disk and floppies), the web UI's requests, and BMLisa's pacing. A problem in any of those usually shows up in it
too.

It does **not** run the Pi side: the Circle kernel, the Pi's display and
sound drivers, USB, Wi-Fi and Circle's network stack, and the SD card
driver. And a PC is much faster than a Pi, so a Pi that can't keep up
won't show in it.

So the simulator is the first place to look, and if a problem doesn't show
up there, the Pi's log (at the end of this page) is the next.

## Setting it up

You need a C compiler, `make`, `git` and Python 3. No Raspberry Pi
toolchain is needed for the simulator.

**Linux** (Debian, Ubuntu, Raspberry Pi OS):

```
sudo apt install build-essential git python3 gdb strace
```

**macOS**: install Xcode's command line tools (`xcode-select --install`),
which include `clang`, `make`, `git` and the `lldb` debugger.

Get the source, with Musashi (a git submodule; IDLE is in the source
itself, in `third_party/idle`):

```
git clone https://github.com/jhonnaker1/bmlisa
cd bmlisa
git submodule update --init third_party/musashi
```

(`git clone --recursive` works too, but also fetches circle-stdlib, which
only the Pi build needs.)

Build and run the tests:

```
tools/hostsim/run_tests.sh
```

The Lisa's ROMs and software can't be distributed, so without them only
the first test runs. Point it at a folder with your ROMs (the same files as
in the SD card's `roms` folder, see docs/ROMS.txt), and hard disk images
with the Lisa Office System, the Workshop, MacWorks XL and Xenix (and
Xenix's boot floppy) (`profile.raw`, or a zip
holding one) and a floppy image; each test gets its own copy:

```
BMLISA_ROMS=/path/to/your/roms BMLISA_LOS=los.zip BMLISA_WORKSHOP=workshop.zip \
    BMLISA_MACWORKS=macworks.zip BMLISA_XENIX=xenix.zip BMLISA_XENIX_BOOT=boot.dc42 \
    BMLISA_FLOPPY=disk.image tools/hostsim/run_tests.sh
```

Each test runs in its own folder under `tools/hostsim/out/`, which stands
in for the SD card, with its log (`log.txt`) and screenshots (`.ppm`).

## Using it live

```
tools/hostsim/live.sh /path/to/your/roms
```

then open <http://127.0.0.1:8064/> in a browser. The Lisa's screen,
keyboard and mouse are in the page, and F12 opens BMLisa's menu as on the
Pi. The simulator's SD card is `tools/hostsim/out/live`: its hard disk is
`disks/profile.raw` (a blank one is made the first time: copy yours over
it), floppy images go in its `disks` folder, and `bmlisa.cfg` into it if
you want the same settings as your Pi's card. Options from `cmdline.txt`
go in `BMLISA_CMDLINE`:

```
BMLISA_CMDLINE="harddisk=disks/workshop.raw" tools/hostsim/live.sh /path/to/your/roms
```

Its messages (the same as the Pi's log) go to the terminal. Ctrl+C stops
it.

## Debugging it

### Where is it stuck? (gdb or lldb)

Start it live as above, get it into the state where it's stuck, and attach
a debugger to it from another terminal.

Linux:

```
sudo gdb -p $(pgrep -f build/bmlisa_sim)
(gdb) thread apply all bt
```

macOS:

```
lldb -p $(pgrep -f build/bmlisa_sim)
(lldb) thread backtrace all
```

The backtrace shows where each thread is, in BMLisa's and IDLE's source
(the simulator is built with debugging information). When the emulation is
running normally, the main thread is somewhere under
`bmlisa_lisa_run_frame()` (the Lisa's CPU) or `host_video_present()`
(waiting for the next frame). Type
`continue` to let it go on, or `detach` and `quit` to leave it running.

To run it under the debugger from the start instead, run it from its SD
card folder:

```
cd tools/hostsim/out/live
BMLISA_LIVE=8064 BMLISA_CMDLINE="root=." gdb ../../build/bmlisa_sim
(gdb) run < /dev/null
```

and press Ctrl+C in gdb when it's stuck.

### What is it doing? (strace, Linux)

```
strace -f -tt -e trace=file,network -p $(pgrep -f build/bmlisa_sim)
```

shows every file it opens (disk images, ROMs, settings) and the web
page's network traffic, with the time.

### Scripts

The simulator can also run a script of key presses, mouse moves and
screenshots on its standard input, with time measured in frames, so that a
problem can be repeated exactly. For example, with your ROMs in `roms` and
the Lisa Office System on `disks/profile.raw`:

```
cd tools/hostsim/out/live
printf '4500 shot desk.ppm\n4510 key 67 1\n4515 key 67 0\n5000 shot off.ppm\n5001 quit\n' |
    BMLISA_CMDLINE="root=." ../../build/bmlisa_sim
```

saves the desktop at frame 4500 (75 seconds in), presses the Lisa's power
button (F10, USB usage 67) and saves the screen as it shuts down. The
commands are listed at the top of `tools/hostsim/host_sim.c`, and
`tools/hostsim/run_tests.sh` has more examples.

Useful settings, as environment variables:

| Variable                     | Does                                                        |
|------------------------------|-------------------------------------------------------------|
| `BMLISA_CMDLINE`           | Options as in `cmdline.txt` (`root=.` is the SD card)       |
| `BMLISA_REFRESH`           | The display's refresh rate (default 60)                     |
| `BMLISA_IP`                | Pretend the network is up with this address (from frame 100) |
| `BMLISA_WIFI`              | Pretend Wi-Fi is running, with a few networks to choose from |
| `BMLISA_REFRESH_LATER`     | Change the display's refresh rate to this at frame 30: a display slightly off its first measurement |

## On the Pi itself

On the Pi there is no debugger or strace, but there is a log. Add
`enable_logging=file` to the line in `cmdline.txt`, and BMLisa writes
`bmlisa.log` in the root of the SD card (and to the serial port: GPIO14 TX
and GPIO15 RX at 115200 baud, for a USB serial adapter, which keeps working
even when the SD card doesn't).

What the log shows:

- The Pi, the display and sound, the machine set up, and (from 0.4.0) the
  CPU clock speed.
- If the emulation stops moving for 3 seconds, `Emulator has not moved for
  3 s; last seen:` and what it was doing (emulating, showing a frame,
  running Circle's tasks such as the network, writing sound...). That line
  is the most useful one for a hang.

When something goes wrong, the most helpful report has:

1. What you did, step by step, and what happened.
2. The BMLisa version, which Pi, and which Lisa (Lisa 2 or Lisa 1).
3. `bmlisa.log` from the card, with `enable_logging=file` (taking the card
   out right after the problem; the log is written as it goes).
4. Your `cmdline.txt` and `bmlisa.cfg`.
5. If it involves a disk or program, which one, or the file if it's free
   to share.
