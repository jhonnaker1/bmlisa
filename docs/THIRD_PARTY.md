# Third-Party Software

BMLisa incorporates software and other materials from third-party projects. Third-party components remain subject to their respective copyright and license terms.

The original copyright, license, and attribution notices supplied with these components are retained with their source where applicable. The notices and licences for code BMLisa adapts from other projects are in [`sdcard/licenses/`](../sdcard/licenses/), which becomes the `licenses` folder of the SD card.

## IDLE

BMLisa incorporates [IDLE](https://sourceforge.net/projects/idle-lisa-emu/), the Apple Lisa emulator by Gilles Fétis, pre-release alpha 0.10 (its Subversion repository, `svn.code.sf.net/p/idle-lisa-emu/code`, revision 26), under a BSD licence. Its licence is `third_party/idle/doc/licence.txt`; its notice is in [`sdcard/licenses/idle.txt`](../sdcard/licenses/idle.txt).

`third_party/idle/` is that revision unmodified, but without the files of it whose licences don't allow BMLisa, which are left out:

- **Its 68000 processor**, `cpu/m68k*`: Musashi 3.3 by Karl Stenerud (as IDLE changed it), which may only be used for non-commercial purposes, other uses needing the author's agreement. That isn't compatible with the GNU General Public License. BMLisa uses today's Musashi instead (below), with IDLE's changes to it made in BMLisa's own `src/port/lisa_cpu.c` (the 68000's bus errors as IDLE did them, the instruction before last) and `src/port/bmlisa_m68kconf.h`.
- **Its serial chip**, `io/8530scc.c`: from Mini vMac (SCCEMDEV.c, by Philip Cummins, Weston Pawlowski and Paul C. Pratt), under version 2 of the GNU General Public License only, which can't be combined with code under version 3 (Circle). BMLisa's own serial chip, `src/port/scc.cpp`, takes its place.
- The video state PROM it had (`bios/vidstate.rom`), which is Apple's.

The rest of IDLE is IDLE's, but for the disassembler its trace uses, `cpu/debug.c`, from the Castaway Atari ST emulator, under the GNU General Public License version 2 or later, and its SDL front end (not built by BMLisa), partly from Hatari, under the same.

## Musashi

BMLisa's 68000 is [Musashi](https://github.com/kstenerud/Musashi) by Karl Stenerud, version 4.10 (`third_party/musashi/`, a git submodule), under the MIT licence: see its `readme.txt`. Its notice is in [`sdcard/licenses/musashi.txt`](../sdcard/licenses/musashi.txt).

Musashi comes with SoftFloat 2b by John R. Hauser (`third_party/musashi/softfloat/`), for its FPU. SoftFloat's licence makes those who use it indemnify its author, which the GNU General Public License doesn't allow, so BMLisa doesn't build it: the Lisa's 68000 has no FPU, and `src/port/lisa_nofpu.c` stands in for the functions Musashi's FPU code calls, which the 68000 never runs.

## BMApple2

BMLisa is based on [BMApple2](https://github.com/jhonnaker1/bmapple2), the bare metal Apple IIgs emulator for the Raspberry Pi, under the GNU General Public License version 3. Its kernel, display, network, web UI, menu and test harness code is used here.

## BMCoco

BMApple2, and so BMLisa, is based on [BMCoco](https://github.com/jhonnaker1/bmcoco), the bare metal Tandy Color Computer emulator for the Raspberry Pi, under the GNU General Public License version 3.

## The menu's font

The menu's 8x8 font (`src/port/osd_font.h`) is the IBM VGA font from Marcel Sondaar's font8_8.asm, as SDL3 and Amiberry ship it, which is in the public domain.

## BMA800XL

BMCoco, and so BMLisa, is a fork of [BMA800XL](https://github.com/aminch/bma800xl), the bare metal Atari 800XL emulator for the Raspberry Pi, under the GNU General Public License version 3. Its kernel, display, sound, network and web UI code is used here, including its Raspberry Pi 4 and Pi 5 display, sound and GPU drivers, merged up to BMA800XL 1.4.2.

## circle-stdlib

BMLisa uses the [circle-stdlib project](https://github.com/smuehlst/circle-stdlib), v21, which includes [Circle](https://github.com/rsta2/circle) by Rene Stange and the newlib C library.

See [`third_party/circle-stdlib/`](https://github.com/smuehlst/circle-stdlib/tree/v21) for the version used by BMLisa and its accompanying copyright and license information. newlib's notices are in [`sdcard/licenses/newlib.txt`](../sdcard/licenses/newlib.txt) and [`sdcard/licenses/libgloss.txt`](../sdcard/licenses/libgloss.txt).

## GCC Runtime

BMLisa links runtime code from the [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) 15.2.rel1 (GCC), under the GNU General Public License version 3 with the GCC Runtime Library Exception 3.1. The toolchain's GCC licence section is in [`sdcard/licenses/GCC-Runtime.txt`](../sdcard/licenses/GCC-Runtime.txt).

## BMC64

BMLisa incorporates source code from the [BMC64 project](https://github.com/randyrossi/bmc64) by Randy Rossi and contributors.

See [BMC64](https://github.com/randyrossi/bmc64) for its source and its accompanying copyright and license information.

BMC64-derived files retain their original license and attribution notices. For files licensed under Apache-2.0, see [the included license](../sdcard/licenses/Apache-2.0.txt); modifications are identified in their headers.

## Raspberry Pi Firmware

BMLisa includes the Raspberry Pi boot firmware (`bootcode.bin`, `start.elf`, `fixup.dat`, and for the Raspberry Pi 4 `start4.elf` and `fixup4.dat`) and the Raspberry Pi 4 and Pi 5 device trees from the [Raspberry Pi firmware repository](https://github.com/raspberrypi/firmware).

Their licences are next to them, in [`sdcard/firmware/`](../sdcard/firmware/) and on the SD card: `LICENCE.broadcom` for the firmware and `COPYING.linux` for the device trees. See the [Raspberry Pi firmware repository](https://github.com/raspberrypi/firmware/tree/master/boot) for more.

## crt-pi

BMLisa uses the crt-pi CRT shader by davej, as included in BMC64, under the GNU General Public License version 2 or later: on the ARM, and compiled for the Raspberry Pi 4 and Pi 5's 3D GPU from [`tools/v3dshader/crt.frag`](../tools/v3dshader/crt.frag). See [`sdcard/licenses/crt-pi.txt`](../sdcard/licenses/crt-pi.txt) and [`sdcard/licenses/GPL-2.0.txt`](../sdcard/licenses/GPL-2.0.txt).

## Mesa

BMLisa uses the [Mesa 3D graphics library](https://mesa3d.org/)'s Broadcom V3D drivers in three ways. The adapted files, with their copyright notices, and Mesa's MIT licence are in [`sdcard/licenses/Mesa-MIT.txt`](../sdcard/licenses/Mesa-MIT.txt).

- **Adapted into the Raspberry Pi 4 and Pi 5 kernels** ([`src/v3d/`](../src/v3d/)): control list packet layouts and packing rules, the rendering job, texture and tile layouts, and the GPU's device facts. The V3D 4.2 (Pi 4) material is from Mesa 26.2.4, the V3D 7.1 (Pi 5) material from Mesa 25.2.8.
- **Adapted into the shader compiler tool** ([`tools/v3dshader/`](../tools/v3dshader/)): the shader preparation that Mesa's drivers do before compiling. The tool is built on a PC against Mesa 25.2.8 and is not part of the SD card.
- **As the shader compiler:** the compiled shaders in `src/v3d/` (`shader_*.h`, `shader71_*.h`) are output of Mesa 25.2.8's V3D compiler. Compiling with Mesa doesn't make them Mesa code: the crt-pi shaders are under crt-pi's licence (above), the others are BMLisa's.

## Linux

For the 3D GPU of the Raspberry Pi 4 and Pi 5, BMLisa adapts parts of the Linux kernel's V3D driver and BCM2835 power domain driver ([`src/v3d/`](../src/v3d/)), from the [Raspberry Pi kernel](https://github.com/raspberrypi/linux/tree/rpi-6.18.y) (`rpi-6.18.y`), under the GNU General Public License version 2 or later. See [`sdcard/licenses/Linux-V3D.txt`](../sdcard/licenses/Linux-V3D.txt) and [`sdcard/licenses/GPL-2.0.txt`](../sdcard/licenses/GPL-2.0.txt).

The Raspberry Pi 4 and Pi 5 display implementation ([`src/kms/pi4/`](../src/kms/pi4/), [`src/kms/pi5/`](../src/kms/pi5/) and the HDMI sound in [`src/av/`](../src/av/)) was independently written using hardware information documented in the Linux vc4 driver (`rpi-6.18.y`). No code from that driver was copied or incorporated.

## Circle ARM stub

For the Raspberry Pi 4, BMLisa includes `armstub8-rpi4.bin`, built from [Circle's `boot/armstub`](https://github.com/rsta2/circle/tree/master/boot/armstub) source, under Circle's [GNU General Public License version 3](https://github.com/rsta2/circle/blob/master/COPYING).

## Wi-Fi Firmware

BMLisa includes the Wi-Fi firmware for the Raspberry Pi's wireless chip from the [RPi-Distro firmware-nonfree repository](https://github.com/RPi-Distro/firmware-nonfree), downloaded when BMLisa is built.

See the `firmware/` folder on the SD card for the Broadcom, Cypress and Synaptics licenses that accompany it.

The Pi 5's files (`brcmfmac43455-sdio.raspberrypi,5-model-b.*`) are copies of the `brcmfmac43455-sdio.*` files under the name Circle looks for on the Pi 5, under the same Cypress license.

## Other Third-Party Components

Additional third-party components and supporting materials may be present in the BMLisa source tree. Where such material is accompanied by its own copyright, license, or attribution notices, those notices and terms continue to apply.
