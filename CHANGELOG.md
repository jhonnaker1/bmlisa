# Changelog

## 0.1.2 – 2026-11-10

- LisaEm's hard disks: a `.dc42` ProFile image (Disk Copy 4.2, its blocks
  interleaved as LisaEm keeps them) can be the hard disk, read and written
  as it is, with its checksums written again for LisaEm when the Lisa
  switches off, and when BMLisa reboots or shuts down.
  `disks/profile.dc42` is used when there is no `disks/profile.raw`; the
  menu's *Hard Disk* and the web UI take them too. `tools/hostsim/dc42.py`
  makes one from a raw image
- Larger hard disks: LisaEm's sizes, 16 MB to 512 MB, as well as 5 and
  10 MB; the ProFile's spare table gives the Lisa their size as LisaEm's
  does ("PROFILE 40M")
- A hard disk image of any other size isn't used, and BMLisa says why;
  IDLE took it, and the Lisa showed only its error 84
- Restarting the Lisa (the menu's *Restart the Lisa*, the web UI, another
  hard disk) switches all its devices off and on, as a real Lisa's power
  does: only its processor and MMU were reset, so the boot ROM then found
  them part way through what they were doing (its error 52, or the
  processor halted). Another hard disk is opened, with its own size, on a
  ProFile just switched on
- The Lisa's clock: its days count from 1, and IDLE gave it the day of the
  year counted from 0, so every date was a day early, and on 1 January
  (the Pi's date with no network to set its clock) the date was day 0: the
  Workshop switched the Lisa off as it started
- Ejecting in the menu: with a disk in, *Floppy* and *Hard Disk* offer
  *Insert Another...* or *Eject*. A floppy is pulled out (the menu also
  sees the Lisa eject them itself, and shows the drive empty); the hard
  disk unplugs the ProFile, and the Lisa starts without one until another
  is chosen (kept as `harddisk=none`). IDLE's ProFile is switched off when
  there's no image (it rewound a file that wasn't open)
- Fast: if the Lisa's processor stops, BMLisa says so, as at normal speed,
  rather than leaving the picture still
- Interrupt (NMI) in the menu works more than once
- The menu's Lisa Power Button does nothing while the Lisa is off (it was
  then held over, and shut the Lisa down as soon as it started)
- Documents: the display stays at 60 Hz on the Pi 4 and 5 with the Lisa
  locked to it (0.8% fast; they said it ran at 59.5 Hz); the Pi 4 and 5
  pages had BMApple2's sound sections, and the keyboard's power key is the
  Lisa's power button (Shut Down only while the menu is open); leftover
  KEGS and IIgs names in comments and tools

## 0.1.1 – 2026-11-09

- The Pi: 0.1.0 stopped before the Lisa started (a black screen, F12 did
  nothing; the log said "Emulator has not moved for 3 s"). IDLE is compiled
  as C++, and seven of its functions returning a value fell off their end
  without one; GCC, which builds the Pi kernels, then leaves out code it
  takes as unreachable, so a check for a missing debug.lst was gone and the
  start looped forever (and a bus error on reading memory could go wrong
  too). They return 0 now. The PC simulator, built with clang, didn't show
  it; GitHub's build now compiles the simulator with GCC, with a missing
  return value as an error
- Tested on a Raspberry Pi 5: the Lisa Office System 3.1 from the hard
  disk, to the desktop, and shut down with the power button

## 0.1.0 – 2026-10-22

- BMLisa starts, from BMApple2 0.4.2: IDLE 0.10 (the Lisa emulator by
  Gilles Fétis) in place of KEGS, running in the PC simulator. The Lisa 2
  (or Lisa 1) from the ROMs in roms; the ProFile hard disk (a blank one made
  if there is none) and Disk Copy 4.2 floppies; the keyboard and mouse; the
  Lisa's power button and switching itself off; the F12 menu; the frame lock
  at the Lisa's 59.5 Hz. Boots the Lisa Office System 3.1, the Workshop
  3.0, MacWorks XL 3.0 and SCO Xenix 3.0 (from its boot floppy,
  pf(0,0)xenix)
- The 68000 is Musashi 4.10 (MIT licence), with IDLE's 68000 bus errors in
  BMLisa's lisa_cpu.c, and the serial chip BMLisa's own (scc.cpp): IDLE's
  Musashi 3.3 (non-commercial use only) and Mini vMac's SCC (GPL version 2
  only) are left out, and Musashi's SoftFloat isn't built (its FPU, which the
  68000 doesn't have, calls lisa_nofpu.c's stand-ins)
- The Lisa's parameter memory (its battery-backed settings, which IDLE
  cleared at every start) is kept in bmlisa.pram: MacWorks XL had no
  double-click speed, so double-clicking never opened anything; set once
  in its Control Panel, it is now kept
- The Raspberry Pi kernels: kernel.img, kernel7.img, kernel8-32.img,
  kernel8-rpi4.img and kernel_2712.img, built with IDLE and Musashi in
  place of KEGS; the web UI uses floppy and hard disk images; the guides
  are the Lisa's

BMApple2's history, from which BMLisa comes, follows.

## 0.4.2 – 2026-10-20

- Fixed: the mouse's right button froze the IIgs, and its wheel's click
  changed the emulation's speed. KEGS takes its desktop versions' middle
  button as a request to stop in its debugger and the right one as a speed
  change, and the USB mouse's buttons came in another order; the IIgs mouse
  has one button, so only the left one is passed on now
- Documentation: the installation guide lists all five kernels (and a Pi
  4 or 5's "not found" screen when they're in a folder), the User Guide's
  Video menu has Resolution and CRT Curvature, and the SD card's README
  mentions the modem

## 0.4.1 – 2026-10-19

- The virtual modem works on the Pi: from a terminal program on the modem
  port (slot 2), `ATDTbbs.example.com:23` calls a telnet BBS. KEGS only
  built its network code for Linux, macOS and Windows, so on the Pi the
  modem did nothing; BMLisa now has its own copy of it, on Circle's
  sockets, dialling in the background so that the Apple keeps running
  while it connects (`CONNECT`, or `NO CARRIER`). *Use Remote IP* works
  too; incoming connections aren't supported. See "Calling BBSes" in the
  User Guide
- Tests: dialling a stand-in BBS through the modem, and a call nobody
  answers

## 0.4.0 – 2026-10-19

- Pi Zero, 2, 3 and 4: run the ARM at its full clock (from BMCoco 0.5.3).
  Without an operating system asking for more the firmware left it at its
  lowest (600 MHz on a Pi 3 B+, not 1.4 GHz). The log shows the clock
- Fix the sound being cut off now and then (from BMCoco 0.5.2): a IIgs or
  IIe (59.92 Hz) locked to a 60 Hz display runs 0.13% fast, more than
  dropping one sample now and then made up for, so the sound buffer filled
  and what didn't fit was cut off. The sound is now squeezed or stretched
  smoothly, by up to 2%. On the Pi Zero to 3 the buffer was also measured
  in 48 kHz samples although they play at 44.1 kHz
- Disk activity (from BMCoco 0.5.3): while a 5.25" or 3.5" drive turns, a
  red light (lit while the disk is read) and its slot, drive and track
  (`S6 D1 T17`) at the bottom left of the picture, and "no disk" in red
  when the drive is empty. Video, Show Disk Activity (`disk_activity=0`)
  turns it off
- Fix a notification (Reset, Restart...) staying on the picture after its
  3 seconds when that part of the screen didn't change
- Video, Screen Size (100% down to 80%), for a TV that cuts off the edges
  (from BMCoco 0.5.3)
- The menus can be used with GPIO joysticks: either one moves, fire chooses
- From BMA800XL 1.4.1 and 1.4.2:
  - The SD card's `licenses` folder: the notices and licences of the code
    from other projects in the kernels (KEGS, Circle and newlib, BMC64's
    Apache 2.0 code, crt-pi, Mesa and Linux's V3D code, the GCC runtime);
    docs/THIRD_PARTY.md says where each is used
  - `./build_sdcard.sh pi4 pi5` builds several boards at once, and runs the
    PC tests first (`--skip-tests` leaves them out)
  - `cmdline.txt` leaves `usbpowerdelay` to Circle (300 ms, 510 ms behind a
    hub) rather than 1000 ms, for a quicker start
  - The Pi 4 and 5's display and GPU code is in `src/av/`, `src/v3d/` and
    `src/kms/`, with the scaled picture centred exactly
  - Troubleshooting: an 8BitDo M30 2.4G whose receiver is in Switch mode
- docs/SIMULATOR.md: testing and debugging BMLisa on a PC (with gdb, lldb
  or strace), and getting a log from the Pi
- Tests: the machine-switching test no longer overwrites APPLE2E.ROM in
  BMLISA_ROMS; sound tests at 44.1 and 48 kHz

## 0.3.2 – 2026-10-16

- The web UI shows the machine running ("Apple IIgs, ROM 3", "Apple IIe"),
  not always "Apple IIgs"
- About shows the machine's frame rate as "Frames", not "IIgs", and the
  SD card's README lists the Pi 5 and Pi Zero
- Pi 4 and 5: a rate just outside what HDMI allows gets the standard 50 or
  60 Hz mode (as in BMCoco), instead of keeping whatever mode the display had
- The web UI no longer carries BMC64's BASIC listing editor, which BMLisa
  never used

## 0.3.1 – 2026-10-16

- The web UI's welcome line and footer name both machines: "Bare metal
  Apple IIgs and IIe"

## 0.3.0 – 2026-10-15

- From BMA800XL 1.2.0 to 1.4.0 (as in BMCoco 0.4.0):
  - Raspberry Pi 5, Pi 500 and Pi 500+ (`kernel_2712.img`): BMA800XL's
    display driver, run at the IIgs's exact rate; Resolution in the Video
    menu (the Pi 5's firmware ignores `hdmi_mode`); HDMI sound; the power
    button shuts down (docs/PI5.md)
  - Raspberry Pi Zero and Zero W (`kernel.img`), on their one core (likely
    too slow for the IIgs)
  - The Pi 4 and 5's 3D GPU draws the CRT effect and smooth scaling, at the
    full 1080 lines (`crt_gpu=0` for the ARM, as before)
  - CRT Curvature in the Video menu
  - Fix the Pi 2 and 3 starting at 1080p instead of 720p
  - Logging is off unless `enable_logging` is set; File logs to the serial
    port too, and the log file is complete after a crash, a hang or
    switching off

## 0.2.0 – 2026-10-15

- Machine choice in the F10 menu (and `machine=`): Apple IIgs ROM 3, ROM
  01, or Apple IIe, of those there are ROMs for
- Monitors (Video, Monitor, and `monitor=`): colour, or black and white,
  green or amber monochrome, with Hi-Res drawn as sharp dots
- Volume (Sound, Volume, and `volume=`), for the IIe too
- The web UI starts 5.25" disks on a IIgs: its startup slot is set to 6 for
  that start (a IIgs set to Scan never goes on to slot 6)
- APPLE2E.ROM can also be made from the unenhanced IIe's MAME chips
- The PC simulator's live mode has the mouse, and plays stereo sound right

## 0.1.0 – 2026-10-15

- First version: BMLisa, based on BMCoco 0.2.0 (itself based on BMA800XL
  1.1.0 and BMC64), with XRoar replaced by KEGS 1.38
- Apple IIgs, with ROM 01 or ROM 3, and Apple IIe, with a IIe ROM
- ROM 3 and the IIe ROM made from MAME's ROM chips (Ample's ROM set on the
  Mac too) the first time BMLisa starts
- Raspberry Pi 2, 3, Zero 2 W, 4 B and 400
- Frames locked to the display's refresh (59.92 Hz; on a Pi 4 the display
  is set to that exactly), with the sound kept in step
- USB keyboards (as an ADB keyboard), USB mice, gamepads as the joystick
  (analog stick or D-pad), GPIO joysticks
- KEGS's own settings (F4) for disks, ROM, speed and the rest; BMLisa's
  menu (F10), in the Apple II's font, for the Pi: video, sound, network,
  restart
- The machine's own speed by default: 2.8 MHz for a IIgs, 1 MHz for a IIe
- Web UI, in Apple II colours: manage the SD card, edit config.kegs, and
  start a disk image (put in the drive that boots it, and the machine
  restarted)
- PC test harness with a stand-in 6502 ROM, and a live mode in a web page
