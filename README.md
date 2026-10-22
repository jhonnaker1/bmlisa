# BMLisa

BMLisa is a bare metal Apple Lisa emulator for the Raspberry Pi. It runs
[IDLE](https://sourceforge.net/projects/idle-lisa-emu/), the Lisa emulator
by Gilles Fétis, directly on the hardware, with no Linux underneath, the
same way [BMC64](https://github.com/randyrossi/bmc64) does for the
Commodore 64, [BMA800XL](https://github.com/aminch/bma800xl) for the Atari
800XL, [BMCoco](https://github.com/jhonnaker1/bmcoco) for the Tandy Color
Computer and [BMApple2](https://github.com/jhonnaker1/bmapple2) for the
Apple IIgs.

BMLisa is based on [BMApple2](https://github.com/jhonnaker1/bmapple2),
which is based on [BMCoco](https://github.com/jhonnaker1/bmcoco), which is
based on [BMA800XL](https://github.com/aminch/bma800xl) by Adam Minchinton,
which is based on [BMC64](https://github.com/randyrossi/bmc64) by Randy
Rossi. The emulator is replaced by IDLE; the kernel, display, network and
web UI come from those projects.

**BMLisa is in development.** It runs in the PC simulator
(`tools/hostsim`), and its Raspberry Pi kernels build, for the Pi Zero, 2,
3, 4 and 5; they haven't been tried on a Pi yet. See
[docs/INSTALLATION.md](docs/INSTALLATION.md) and the
[User Guide](docs/USER_GUIDE.md).

## How it works

IDLE's emulation (`third_party/idle`, from its Subversion repository,
unmodified) is built without its SDL front end (`src/port/idle.mk`), on
[Musashi](https://github.com/kstenerud/Musashi) 4 (`third_party/musashi`)
for its 68000: IDLE's own copy of Musashi, and its serial chip, aren't in
BMLisa, as their licences don't allow it (`src/port/lisa_cpu.c` and
`src/port/scc.cpp` do what IDLE's changes to them did; see
[the third-party notes](docs/THIRD_PARTY.md)).
`src/port/lisa.cpp` does what that front end did, and
`src/port/platform.c` runs it on top of the kernel services in
`src/host.h`:

- The Lisa's 720 x 364 screen goes to the display at 4:3, locked to its
  refresh (the Lisa runs 59.5 frames a second), in white, green or amber.
- USB keyboards (Ctrl and the Windows or Command key are the Apple key, Alt
  the Option keys) and mice.
- The ProFile hard disk is an image on the SD card (`disks/profile.raw`
  unless another is chosen; a blank 10 MB one is made if there is none),
  and floppies are Disk Copy 4.2 images.
- F12 opens BMLisa's menu: the floppy, the hard disk, the Lisa's power
  button (also F10), restarting, speed, the model (Lisa 2 or Lisa 1),
  video, network.
- When the Lisa's software switches it off, the screen goes dark until a
  key or a click switches it on again.

It runs the Lisa Office System 3.1, the Workshop 3.0, MacWorks XL 3.0
(the Macintosh's Finder: choose a double-click speed in its Control Panel
once) and SCO Xenix 3.0. The Lisa's parameter memory, its battery-backed
settings, is kept on the card (`bmlisa.pram`).

Xenix on a hard disk is started from its boot floppy, as on a real Lisa:
the boot ROM can't start it (error 75). Insert `Xenix 3.0 Boot.dc42`, press
Apple+3 (Startup From...), then Apple+2 (the floppy), and at Xenix's `:`
prompt type `pf(0,0)xenix` (it misses the first key: type it again if the
`p` is lost). Apple (Ctrl) is Xenix's Control key: Apple+d at its startup
prompt.

## ROMs

The Lisa's ROMs are Apple's, so they are not included. Copy yours into the
SD card's `roms` folder: `341-0175-h`, `341-0176-h` and `vidstate.rom` for
the Lisa 2 (MAME's `lisa2.zip` has them under these names), or
`L175REVC.bin`, `L176REVC.bin` and `vidstate.rom` for the Lisa 1. See
[docs/ROMS.txt](docs/ROMS.txt).

## The PC simulator

```
tools/hostsim/run_tests.sh
BMLISA_ROMS=/path/to/roms BMLISA_LOS=los.zip BMLISA_WORKSHOP=workshop.zip \
    BMLISA_MACWORKS=macworks.zip BMLISA_XENIX=xenix.zip BMLISA_XENIX_BOOT=boot.dc42 \
    BMLISA_FLOPPY=disk.image tools/hostsim/run_tests.sh
tools/hostsim/live.sh /path/to/roms
```

The tests need your ROMs and hard disk images (copies are used, never the
files themselves): they boot the Lisa Office System to its desktop, open a
menu with the mouse, shut it down with the power button and start it
again, insert a floppy, type at the Workshop's command line, open
MacWorks XL's hard disk from the Finder's File menu, and boot Xenix from
its floppy, log in as root and list its files.
`live.sh` runs BMLisa in real time in a web page.

## License

GPL 3 (see LICENSE). IDLE is under a BSD licence and Musashi under the MIT
licence; see [docs/THIRD_PARTY.md](docs/THIRD_PARTY.md).
