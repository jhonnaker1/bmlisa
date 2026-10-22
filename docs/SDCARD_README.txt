BMLisa - bare metal Apple Lisa emulator for the Raspberry Pi
============================================================

Copy everything here to the root of a FAT32 SD card and boot a Raspberry
Pi 5, 500, 4, 400, 3, Zero 2 W or 2 from it (a Pi Zero or Zero W starts
too, but may be too slow), with a USB keyboard and mouse connected.

First copy the Lisa's ROMs into the roms folder (see roms/ROMS.txt): for a
Lisa 2, 341-0175-h, 341-0176-h and vidstate.rom. They are Apple's, so they
are not included.

The hard disk is disks/profile.raw: copy a ProFile image with the Lisa
Office System, the Workshop, MacWorks XL or Xenix there, or let BMLisa
make a blank one and install from floppies. Floppies are Disk Copy 4.2
images (.dc42, .image): insert one from the F12 menu.

  F12           BMLisa's menu: floppy, hard disk, restart, speed, video
  F10           The Lisa's power button: shut down before switching off
  Windows/Cmd   Apple            Ctrl     Apple too
  Alt           Option           Menu     Enter

At the boot ROM's screen, Apple+3 chooses the drive to start from (Apple+2
the floppy). Xenix starts from its boot floppy: at its ":" prompt, type
pf(0,0)xenix (again if the first key is lost).

Networking: choose Ethernet or Wi-Fi under Network in the menu (or add
network=ethernet or network=wifi to cmdline.txt), then open the address
shown on screen in a browser to manage the files on this card and use
disk images.

Licences: BMLisa is under the GPL version 3 (LICENSE.txt); the licenses
folder has the notices for other projects' code it includes, and the
firmware folder the Raspberry Pi and Wi-Fi firmware's licences.

Options are in cmdline.txt; see docs/USER_GUIDE.md in BMLisa's source for
all of them.
