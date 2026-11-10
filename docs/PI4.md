# Raspberry Pi 4 B and Pi 400

BMLisa runs on the Pi 4 B and Pi 400 as a 64-bit kernel,
`kernel8-rpi4.img`. The emulator, menus, USB keyboards, mice and gamepads,
networking and the web UI work as on the Pi 3.

Pi 4 support comes from [BMA800XL](https://github.com/aminch/bma800xl)
1.1.0, by way of [BMCoco](https://github.com/jhonnaker1/bmcoco).
BMA800XL's picture is 8-bit with a palette; BMCoco's and BMLisa's are
RGB565 (BMLisa draws the Lisa's one bit pixels into it), so the
display code (`src/av/video.cpp`, `src/av/scaler.cpp`) takes RGB565
instead. The rest is BMA800XL's.

## How it differs from the Pi 2 and 3

| | Pi 2 and 3 | Pi 4 B and Pi 400 |
|---|---|---|
| Kernel | 32-bit | 64-bit, with Circle's `armstub8-rpi4.bin` |
| Picture | VideoCore dispmanx layer | BMA800XL's display driver, or the firmware's framebuffer |
| Smooth scaling | The GPU | The display hardware's scaler (the HVS) |
| CRT shader | crt-pi on the GPU | crt-pi recalculated on the ARM cores |

The emulator itself (`src/port/`) is the same on every board.

## Picture

BMLisa has BMA800XL's driver for the Pi 4's display hardware
(`src/kms/pi4/`). It takes over the HDMI port in use (HDMI0 if both are),
shows the Lisa's picture through the HVS, the Pi's hardware scaler, and can
set the display's refresh rate for an emulator. It is chosen with
`video_driver` in `cmdline.txt`:

| `video_driver` | What it does |
|---|---|
| `kms` (default) | As `hvs`, and also sets the video mode for the emulator's rate where that is within 0.5% of the display's. The Lisa's 59.5 Hz isn't, so the display keeps its 60 Hz and the Lisa is locked to it (0.8% fast). |
| `hvs` | Keeps the mode set in `config.txt`. Use it for a TV that goes blank for a while when the refresh rate changes. |
| `firmware` | The firmware's framebuffer, with the picture scaled by the ARM. Use it if the others give trouble. |

If something isn't as expected (an unknown mode, a step that doesn't
complete), the driver falls back to the firmware's mode or framebuffer.
The log (serial, or `bmlisa.log`) shows what happened, for example:

```
kms: HDMI0, HVS channel 1, 1280x720
kms: EDID (HDMI0): 2 block(s), HDMI, 9 video modes
BMLisa: display refresh rate 60.000 Hz
BMLisa: display 60.00 Hz, Lisa 59.50 Hz, synchronised to the display
```

The driver was written by examining the Linux `vc4` driver and the BCM2711
device tree, documenting the hardware's behaviour, and writing BMA800XL's
own code from that.

A bare metal kernel can't use the Pi 4's OpenGL ES, so BMLisa drives
the Pi 4's 3D GPU itself (`src/v3d/`, from BMA800XL 1.2.0, by way of
BMCoco) to draw the smooth and CRT pictures (`smooth`, `crt_shader`,
`crt_curvature`): crt-pi, at up to 1080 lines and scaled up to fill larger
screens. If the GPU doesn't start or can't draw a picture, ARM cores 1 to 3
draw it instead (`src/av/scaler.cpp`; up to 1080 lines, 720 with
curvature and smooth both on); `crt_gpu=0` always uses the ARM. The log's
`v3d:` lines and *Drawn by* line say which. `crt_gpu_test=1` also compares
the GPU's pictures with the ARM's at start up.

The CRT shader looks best at 1080p: `hdmi_mode=16` in `config.txt`.

## Sound

The Lisa has no sound in BMLisa (IDLE emulates none).

## Shut Down

*Shut Down* (menu, web UI, or a keyboard's power key while the menu is
open, which is Fn+F10 on the Pi 400; otherwise it is the Lisa's power
button) halts the Pi the way Linux powers it off. The Pi 400 switches
itself off; Fn+F10 turns it back on. The Pi 4 B halts; unplug it, or
connect GPIO3 to ground, to start it again.
