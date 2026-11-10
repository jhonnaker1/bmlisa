# BMLisa User Guide

BMLisa turns a Raspberry Pi into an Apple Lisa. It runs the IDLE emulator
by Gilles Fétis on the bare hardware, so it starts in a couple of seconds
and can be switched off at any time (but see Switching off).

## The SD card

| File or folder   | What it is                                                  |
|------------------|-------------------------------------------------------------|
| `kernel8-32.img`, `kernel7.img`, `kernel.img`, `kernel8-rpi4.img`, `kernel_2712.img` | BMLisa for the Pi 3 and Zero 2 W, the Pi 2, the Pi Zero and Zero W, the Pi 4 and Pi 400, and the Pi 5, 500 and 500+ |
| `bootcode.bin`, `start.elf`, `fixup.dat`, `start4.elf`, `fixup4.dat`, `armstub8-rpi4.bin`, `*.dtb`, `overlays/` | Raspberry Pi firmware |
| `config.txt`     | Raspberry Pi firmware settings: the display mode            |
| `cmdline.txt`    | BMLisa's options (see Options)                              |
| `bmlisa.cfg`     | Settings changed in BMLisa's menu (written by BMLisa)       |
| `bmlisa.pram`    | The Lisa's parameter memory, its battery-backed settings (written by BMLisa) |
| `roms/`          | The Lisa's ROMs: you supply them (see ROMs)                 |
| `disks/`         | `profile.raw` (or LisaEm's `profile.dc42`), the hard disk, and your floppy images |
| `firmware/`      | Wi-Fi firmware                                              |

## ROMs

The Lisa's ROMs are Apple's, so BMLisa doesn't include them. Copy your own
into `roms`:

| Lisa     | ROM files                                                   |
|----------|-------------------------------------------------------------|
| Lisa 2 (2/5, 2/10, Macintosh XL) | `341-0175-h` and `341-0176-h` (the boot ROM's two chips, 8K each), and `vidstate.rom` (the video PROM, 256 bytes) |
| Lisa 1   | `L175REVC.bin` and `L176REVC.bin`, and `vidstate.rom`       |

MAME's `lisa2.zip` has the Lisa 2's under these names: copy the three files
out of the zip into `roms`. BMLisa starts the Lisa 2 if its ROMs are there,
otherwise the Lisa 1; *Machine* in the **F12** menu (or `machine=` in
`cmdline.txt`) chooses, and BMLisa reboots into it.

## Disks

**The hard disk** is a ProFile: an image on the card, either raw, in
IDLE's form (blocks of 532 bytes: 20 bytes of tags, then 512 of data), or
LisaEm's (`.dc42`, a Disk Copy 4.2 image), which BMLisa reads and writes as
it is. It can be 5 or 10 MB, or one of the larger sizes LisaEm makes (16,
20, 32, 40, 64, 128, 256 or 512 MB); an image of any other size isn't used,
and BMLisa says so. It is `disks/profile.raw` (or `disks/profile.dc42` if
there is only that), or another chosen with *Hard Disk* in the menu (or
`harddisk=` in `cmdline.txt`); the choice is kept, and the Lisa starts
again from it, as if switched off and on. If there is none,
BMLisa makes a blank 10 MB raw one, so that an operating system can be
installed on it from floppies.

A LisaEm disk's checksums (Disk Copy's) are written again when the Lisa
switches itself off, and when BMLisa reboots or shuts down; that reads the
whole image, a few seconds on the Pi. LisaEm doesn't check them, so a Pi
switched off without shutting down does no harm.

**Starting from a floppy:** press a key now and then while the Lisa tests
itself (*TESTING...*); its *Startup From* menu then opens, and Apple+2
starts from the floppy drive (Apple+3 from the ProFile).

MacWorks XL 3.0 runs from a hard disk with it installed, such as IDLE's
10 MB one, or from its boot floppy (with Apple+2) with such a disk
attached. A 40 MB LisaEm disk with MacWorks XL doesn't start: from the
disk it stops at the hourglass, and from the floppy MacWorks stops with a
Sad Mac (0F0002); why isn't known yet.

**Floppies** are Disk Copy 4.2 images (`.dc42`, `.image`, `.img`). A Lisa 2
has one 3.5" drive, a Lisa 1 two Twiggy drives. Choose one with *Floppy* in
the menu, or start BMLisa with one in the drive with `floppy=` in
`cmdline.txt` (a path with no spaces in it). The Lisa ejects disks itself,
from its software. With a disk in the drive, *Floppy* in the menu offers
*Insert Another...* or *Eject*, which pulls it out as a paper clip in a
real Lisa's drive does: let the Lisa eject it when you can.

**Ejecting the hard disk:** with one attached, *Hard Disk* in the menu
offers *Insert Another...* or *Eject*, which unplugs the ProFile: shut the
Lisa down first. The Lisa then starts with no hard disk (from a floppy)
until another is chosen; the choice is kept (`harddisk=none`).

From the web UI, clicking a disk image uses it: a hard disk image becomes
the hard disk and the Lisa starts again from it; a floppy goes in the
drive.

What runs (tested in the PC simulator, see [SIMULATOR.md](SIMULATOR.md)):

- **The Lisa Office System** (3.1) and **the Workshop** (3.0), from the
  hard disk.
- **MacWorks XL** (3.0), the Macintosh on the Lisa, from the hard disk.
  The first time, open the *Control Panel* (in the Apple menu) and choose a
  *Double-Click Speed*: with blank parameter memory (see below) it has
  none, and double-clicking doesn't open anything until it has.
- **SCO Xenix** (3.0), from its boot floppy (see below).

### Starting from another disk

The Lisa starts from the hard disk. If that doesn't start (or to start from
a floppy), its boot ROM shows a box with **Apple+2** (*Continue*) and
**Apple+3** (*Startup From...*). Apple+3 lists the drives: **Apple+2** is
the floppy, **Apple+3** the hard disk.

**Xenix** on a hard disk is started from its boot floppy, as on a real
Lisa: the boot ROM can't start it from the hard disk (it says error 75).
Insert `Xenix 3.0 Boot.dc42`, press Apple+3, then Apple+2, and at Xenix's
`:` prompt type `pf(0,0)xenix` and Return. It misses the first key typed
there: if it says *Unknown device*, type it again. The Apple key is Xenix's
Control key: at *Type CONTROL-d to proceed with normal startup*, press
Apple+D. Log in as `root`.

### Parameter memory

The Lisa keeps its settings in a little memory that a battery keeps while
it is switched off: the startup device, the Lisa Office System's
preferences, and, under MacWorks, the Macintosh's (such as its
double-click speed). BMLisa keeps it in
`bmlisa.pram` in the root of the card, written as the Lisa changes it.
Deleting the file gives the Lisa blank parameter memory, as a flat
battery would.

## Switching off

The Lisa has a soft power button: its software shuts down, writing what it
has to its disks, and then switches the Lisa off. **F10** is that button
(also in the menu, and a keyboard's power key). Once the Lisa is off, its
screen goes dark; a key or a click switches it on again.

BMLisa writes everything the Lisa writes to its disk images out to the card
every second, so the Pi can be switched off at any time; but like a real
Lisa switched off without shutting down, the Lisa Office System then checks
its disk at the next start, and Xenix asks to clean its file system. Use
F10 first.

## The menu

**F12** (or a gamepad's **Home** button, or **Select+Start**) opens
BMLisa's menu; **Esc** goes back a page and F12 closes it. The arrow keys
or D-pad move, **Return** or **A** chooses. The Lisa stops while it is
open.

| Item           | What it does |
|----------------|--------------|
| Floppy (Upper and Lower Floppy on a Lisa 1) | Puts a floppy image in the drive |
| Hard Disk      | Chooses the hard disk image; the Lisa starts again from it |
| Lisa Power Button | The Lisa's power button (F10): its software shuts down |
| Restart the Lisa | Starts it again at once, as a reset does; what isn't saved is lost |
| Interrupt (NMI) | The Lisa's NMI key: Lisa software's debugger, where there is one |
| Fast           | Runs the Lisa as fast as the Pi can, rather than at its own 5 MHz |
| Machine        | Lisa 2 or Lisa 1, those there are ROMs for; BMLisa reboots into it |
| Video          | The monitor (white, green or amber on black), the screen size (smaller for a TV that cuts off the edges), the frame lock, the resolution (Pi 5), the CRT effect and its curvature, smooth scaling |
| Network        | Ethernet or Wi-Fi, the web UI (see below) |
| About          | Versions, the machine, hard disk, display and network |
| Reboot, Shut Down | Restart BMLisa, or halt the Pi. Shut Down also from a keyboard's power key when the menu is open: Fn+F10 on a Pi 400, which switches off; Fn+F10 starts it again |

Settings changed in the menu are kept in `bmlisa.cfg` and used at the next
boot.

## Keyboard and mouse

The Lisa's keyboard has no Control, Escape, function or arrow keys.

| PC key                 | Lisa                                          |
|------------------------|-----------------------------------------------|
| F12                    | BMLisa's menu                                 |
| F10, power key         | The Lisa's power button                       |
| Windows (Command on a Mac keyboard), Ctrl | Apple (Command)            |
| Left Alt, right Alt    | Left and right Option                         |
| Menu (beside right Ctrl) | Enter (beside the Lisa's space bar)         |
| Caps Lock              | Caps Lock, which stays down until pressed again, as the Lisa's does |
| Num Lock               | The keypad's Clear                            |

The other keys are where they are on a US Lisa keyboard; the keypad is the
Lisa's keypad.

A **USB mouse** is the Lisa's mouse; any of its buttons is the Lisa's one
button.

## Display and timing

The Lisa's screen is 720 x 364 pixels, black and white, on a 4:3 screen
(its pixels are taller than they are wide). IDLE's Lisa shows 59.5 frames a
second, and `config.txt` sets the display to 720p at 60 Hz. When the
display's refresh rate is within 2% of the Lisa's, each frame is shown on
exactly one display refresh, so the mouse moves smoothly; otherwise the
Lisa keeps exact time and frames are shown as they come. At 60 Hz the Lisa
is locked to the display, so it runs 0.8% fast, which isn't noticeable (the
Pi 4 and 5 set a display's rate only when it is within 0.5% of the
emulator's, and 59.5 Hz isn't). `vsync=on` or
`vsync=off` in `cmdline.txt` (or *Frame Lock* in the menu) forces one or
the other.

**Monitor** (in *Video*): white on black, as the Lisa's, or green or amber.

The Lisa has no sound in BMLisa (IDLE emulates none).

## Networking and the web UI

BMLisa can join your network over Ethernet or Wi-Fi and serve a web page
for managing the SD card from any browser on the same network, so files can
be copied to and from the Pi without taking the card out. It is the web UI
from BMC64.

Turn it on in the menu's *Network*. For Ethernet, set *Network* to Ethernet
and reboot. For Wi-Fi, choose *Scan for Wi-Fi Networks*: the first time,
it offers to switch Wi-Fi on and reboot; after that it lists the networks
it can hear, strongest first. Pick yours: it fills in *Wi-Fi Network*, and
if the network needs a password, *Wi-Fi Password* is next. Then reboot.
Set *Wi-Fi Country* to your two letter country code too; it decides which
channels are allowed. A hidden network can be typed in under *Wi-Fi
Network* instead. Or, in `cmdline.txt` (on the same single line as the
other options):

- Ethernet: `network=ethernet`
- Wi-Fi: `network=wifi`, and copy `wpa_supplicant.conf.example` to
  `wpa_supplicant.conf` in the root of the card with your network's name,
  password and country.

Wi-Fi needs the files in the `firmware` folder, which `build_sdcard.sh` puts
there.

Once the Pi has an address, it is shown at the bottom of the screen for a
few seconds (`Web UI: http://192.168.1.50/`) and written to the log.
Open that address in a browser.

The web UI has:

- **Dashboard**: the Pi's model, address, uptime, temperature, power and
  throttling warnings and free space on the card. Buttons to **Restart**
  the Lisa, **Reboot** BMLisa, **Shut Down** (the Pi halts; switch it off,
  then on again to restart; a Pi 400 switches itself off, and its power key
  starts it again), and **Disable Web UI** until the next reboot.
- **Files**: browse the SD card; upload (or drag and drop) files into a
  folder, download, rename, delete, and create folders. Clicking a disk
  image **uses it** (see Disks). `cmdline.txt`, `config.txt`, `bmlisa.cfg`
  and `wpa_supplicant.conf` can be **edited** in the browser; changes take
  effect after a reboot, and the previous version is kept as `.bak`.

Uploads can't overwrite `cmdline.txt`, `config.txt` or `wpa_supplicant.conf`
(edit them instead), or anything in `firmware`. Everything else, including
the kernels, can be replaced, so a new build of BMLisa can be uploaded and
started with **Reboot**. A mistake in `cmdline.txt` or `config.txt` can stop
the Pi starting, and then needs the card in a computer (the `.bak` copy
makes that easy).

The web UI is open to anyone on your network. To protect it with a PIN, set
`webui_pin=1234` in `cmdline.txt`; the browser then asks for a user name
(anything) and the PIN. To network without the web UI, set `webui=0`.

The network also sets the Pi's clock, which the Lisa's clock starts from,
and so files saved on the card get the right date and time. Set `timezone`
to your offset from UTC in minutes, for example `timezone=600` for UTC+10.

## Options

`cmdline.txt` is a single line of `name=value` options separated by spaces.
BMLisa's menu saves the options it changes to `bmlisa.cfg`, one
`name=value` per line, and those win over `cmdline.txt`. Either file can be
edited from the web UI.

| Option              | Values                       | Default  | What it does |
|---------------------|------------------------------|----------|--------------|
| `machine`           | `lisa2`, `lisa1`             | first with ROMs | The Lisa to emulate (see ROMs) |
| `harddisk`          | a path on the card           | `disks/profile.raw` | The ProFile hard disk image (see Disks) |
| `floppy`            | a path on the card           | none     | A floppy image to put in the drive at start |
| `monitor`           | `white`, `green`, `amber`    | `white`  | The screen's colour |
| `aspect`            | number                       | `1.0`    | Multiplies the width of the picture |
| `scale`             | 0.1 to 1.0                   | `1.0`    | Height of the picture as a fraction of the screen (Video, Screen Size; less for a TV that cuts off the edges) |
| `smooth`            | `0`, `1`                     | `0`      | Bilinear filtering instead of sharp pixels |
| `crt_shader`        | `0`, `1`                     | `0`      | CRT look (scanlines) from BMC64 |
| `crt_curvature`     | `0`, `1`                     | `0`      | With `crt_shader`, a curved screen |
| `crt_gpu`           | `0`, `1`                     | `1`      | Pi 4 and 5: the 3D GPU draws the smooth and CRT pictures; `0` has the ARM draw them (see [PI4.md](PI4.md)) |
| `vsync`             | `auto`, `on`, `off`          | `auto`   | Lock frames to the display (see Display and timing) |
| `resolution`        | `auto`, `1920x1080`, `1280x720` | `auto` | Pi 5 only, with `video_driver=kms`: the display's resolution; `auto` keeps the one the firmware chose (the Pi 5's firmware ignores `hdmi_mode`) |
| `video_driver`      | `kms`, `hvs`, `firmware`     | `kms`    | Pi 4 and 5: `kms` runs the display at the Lisa's exact frame rate; `hvs` leaves the mode set in `config.txt`; `firmware` uses the firmware's framebuffer, if the others give trouble (see [PI4.md](PI4.md) and [PI5.md](PI5.md)) |
| `gpio_joysticks`    | `0`, `1`                     | `0`      | Joysticks on the GPIO header, which move the menu's cursor (BMC64's "GPIO Config 1" wiring) |
| `enable_logging`    | `off`, `uart`, `file`        | `off`    | Where the log goes (see Logging) |
| `network`           | `off`, `ethernet`, `wifi`    | `off`    | Networking (see Networking and the web UI) |
| `webui`             | `0`, `1`                     | `1`      | Web UI when networking is on |
| `webui_pin`         | digits                       | none     | PIN for the web UI |
| `timezone`          | minutes east of UTC          | `0`      | Time zone for the network clock |
| `usbpowerdelay`     | milliseconds                 | Circle's | Circle: wait for USB devices to power up (by default 300 ms, 510 ms behind a hub) |

## Logging

BMLisa logs what it is doing, which helps when something doesn't work:
the version, the display set up, the ROMs and hard disk it found, floppies
inserted, devices coming and going, and network status. Choose where it
goes with `enable_logging` in `cmdline.txt`:

- **Off** (the default, also when `enable_logging` isn't set): no log.
- **UART**: the serial port, GPIO14 TX and GPIO15 RX at 115200 baud
  (all boards, the Pi 5 too). Needs a USB serial adapter.
- **File**: the serial port, and `bmlisa.log` in the root of the SD card.
  The file starts with what was logged since boot, is written to the card
  every few seconds (and is complete after a crash, a hang or switching
  off), and starts again at boot once it is over 1 MB. Download it from
  the web UI's file manager to send with a bug report. Writing it costs a
  little time on each frame, so switch it off once you have what you need.

## Troubleshooting

**"There are no Lisa ROMs in the roms folder"**: see ROMs; the names must be
exactly those, and the sizes right (8192 bytes, and 256 for
`vidstate.rom`).

**The boot ROM shows a crossed out hard disk and a number**: the hard disk
doesn't start. A blank one (as BMLisa makes) has nothing on it yet: install
the Lisa Office System from its floppies, or choose another image with
*Hard Disk*. Xenix's hard disk is started from its floppy (see Disks).

**The Lisa Office System says the disk was in use when the Lisa failed**:
the Lisa was switched off without shutting down. Let it check the disk.
Shut down with F10 next time.

**"The Lisa's processor stopped"**: the emulated 68000 halted (a bus error
while taking another). Restart with the menu's *Reboot*; the log says
where.

**No picture**: uncomment `hdmi_safe=1` in `config.txt`. If that works, your
display doesn't accept 720p at 60 Hz; try another `hdmi_mode`.

**The TV cuts off the top and bottom of the picture or the menu**: many TVs
enlarge the picture and crop its edges (overscan). Make *Video*, *Screen
Size* smaller (90% usually does), or turn overscan off on the TV ("Just
Scan", "Screen Fit" or "1:1").

**Keyboard or mouse not working**: unplug it and plug it in again; devices
can come and go while the emulator runs. Some keyboards need a longer
`usbpowerdelay` (add `usbpowerdelay=1000` to `cmdline.txt`).
