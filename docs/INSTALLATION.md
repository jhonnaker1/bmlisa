# Installation

## What you need

- A Raspberry Pi 4 Model B or Pi 400, a Raspberry Pi 5, Pi 500 or Pi 500+,
  a Raspberry Pi 3 Model B, B+ or A+, a Raspberry Pi Zero 2 W, or a
  Raspberry Pi 2 Model B (a Pi Zero or Zero W starts too, but may be too
  slow)
- A microSD card (BMLisa itself needs only a few MB; a ProFile hard disk
  image is 5 or 10 MB, a floppy image 400 or 800K)
- An HDMI display, ideally one that accepts 720p at 60 Hz
- Your own Lisa ROMs (see below)
- A USB keyboard and a USB mouse

## Prepare the SD card

1. Download `bmlisa-v<version>.zip` from the releases page, or build it
   yourself (see [BUILDING.md](BUILDING.md)) and use `build/sdcard/`.
2. Format the SD card as FAT32.
3. Extract the zip to the root of the card, so that the kernels
   (`kernel_2712.img`, `kernel8-rpi4.img`, `kernel8-32.img`, `kernel7.img`,
   `kernel.img`) and `config.txt` are at the top level, not in a folder (a
   Pi 4 or 5 with them in a folder shows its firmware's "not found" screen).
4. Copy your ROMs into the `roms` folder (see below).
5. Copy your hard disk image to `disks/profile.raw` (a LisaEm hard disk,
   `.dc42`, to `disks/profile.dc42`), and floppy images into `disks` (or
   anywhere).
6. Put the card in the Pi, connect the display, keyboard and mouse, and
   switch on.

There is no image to write and nothing to install on the card beyond
copying the files.

## ROM files

The Lisa's ROMs are Apple's, so BMLisa can't include them. Copy images of
your own into the `roms` folder:

| File | ROM |
| --- | --- |
| `341-0175-h`, `341-0176-h` | The Lisa 2's boot ROM (two chips, 8K each) |
| `vidstate.rom` | The video state PROM (256 bytes) |
| `L175REVC.bin`, `L176REVC.bin` | The Lisa 1's boot ROM, for a Lisa 1 |

MAME's `lisa2.zip` has the Lisa 2's under these names. See
[ROMS.txt](ROMS.txt).

## First boot

The Lisa tests itself, then starts from the hard disk. With no hard disk
image, BMLisa makes a blank one, and the boot ROM shows the hard disk
crossed out: insert the Lisa Office System's first floppy with **F12**,
*Floppy*, press Apple+3 (*Startup From...*), then Apple+2, and install it
on the hard disk. Or copy a hard disk image that has it already to
`disks/profile.raw`.

Shut the Lisa down with **F10** (its power button) before switching off;
see [Switching off](USER_GUIDE.md#switching-off) in the User Guide.

USB keyboards and mice can be plugged in and out while it runs.

The display is set to 720p at 60 Hz. If you get no picture, see
[Troubleshooting](USER_GUIDE.md#troubleshooting) in the User Guide.

## Networking (optional)

To manage the card's files from a browser, and use disk images from it,
turn on Ethernet or Wi-Fi under *Network* in the F12 menu, then reboot; the
web UI's address is shown at the bottom of the screen. For Wi-Fi, *Scan
for Wi-Fi Networks* lists the networks in range and asks for the password.
See [Networking and the web UI](USER_GUIDE.md#networking-and-the-web-ui).

## Upgrading

Copy the new release's files over the old ones, except for files you have
changed: `config.txt` and `cmdline.txt` if you edited them. Your settings
(`bmlisa.cfg`, `wpa_supplicant.conf`), ROMs and disk images are not in the
release zip and stay as they are.

With networking on, a new kernel (`kernel_2712.img` for a Pi 5, 500 or
500+, `kernel8-rpi4.img` for a Pi 4 or 400, `kernel8-32.img` for a Pi 3 or
Zero 2 W, `kernel7.img` for a Pi 2, `kernel.img` for a Pi Zero or Zero W)
can also be uploaded in the web UI's file manager and started with
**Reboot**. Updates that change other files (the Pi firmware in the root or
in `firmware/`) need the card in a computer.
