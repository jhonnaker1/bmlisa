/*
 * bmlisa.h - shared declarations for BMLisa's port of IDLE.
 *
 * This file is part of BMLisa, a bare metal Apple Lisa emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMLISA_PORT_H_
#define BMLISA_PORT_H_

#include <stdint.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* USB HID usage codes (keyboard page) that BMLisa itself uses. */
enum {
	USAGE_A = 0x04,
	USAGE_RETURN = 0x28, USAGE_ESCAPE = 0x29, USAGE_BACKSPACE = 0x2a,
	USAGE_TAB = 0x2b, USAGE_SPACE = 0x2c, USAGE_CAPS_LOCK = 0x39,
	USAGE_F1 = 0x3a, USAGE_F4 = 0x3d, USAGE_F10 = 0x43, USAGE_F11 = 0x44,
	USAGE_F12 = 0x45,
	USAGE_PAUSE = 0x48, USAGE_HOME = 0x4a, USAGE_PAGEUP = 0x4b,
	USAGE_DELETE = 0x4c, USAGE_END = 0x4d, USAGE_PAGEDOWN = 0x4e,
	USAGE_RIGHT = 0x4f, USAGE_LEFT = 0x50, USAGE_DOWN = 0x51, USAGE_UP = 0x52,
	USAGE_KP_ENTER = 0x58, USAGE_POWER = 0x66,
	USAGE_LCTRL = 0xe0, USAGE_LSHIFT = 0xe1, USAGE_LALT = 0xe2,
	USAGE_RCTRL = 0xe4, USAGE_RSHIFT = 0xe5, USAGE_RALT = 0xe6
};

/* The Lisa has no function keys: F12 is BMLisa's menu, F10 the Lisa's
   power button. */
#define KEY_MENU   USAGE_F12
#define KEY_POWER  USAGE_F10

/* platform.c */
void bmlisa_main(void);
const char *bmlisa_sd_root(void);
/* The emulator's frame buffer, for the on-screen display. */
uint16_t *bmlisa_framebuffer(int *pitch_px);
/* Shows the on-screen display's 640x460 area (TRUE) or the Lisa's screen
   (FALSE). */
void bmlisa_osd_view(int on);
/* Called while a menu waits: keeps the files written out and runs network
   requests. */
void bmlisa_idle(void);
double bmlisa_refresh_rate(void);
int bmlisa_vsync_locked(void);
void bmlisa_video_settings_changed(void);
/* The display's resolution setting changed (the Pi 5). */
void bmlisa_display_mode_changed(void);
/* The models, the one running (-1 if there is no ROM), and choosing
   another (it starts again; kept in bmlisa.cfg). */
enum { BMLISA_LISA2, BMLISA_LISA1, BMLISA_MACHINES };
int bmlisa_machine(void);
const char *bmlisa_machine_name(int machine);
int bmlisa_machine_available(int machine);
void bmlisa_set_machine(int machine);
/* The screen's colours: white, green or amber on black (kept in
   bmlisa.cfg). */
enum { BMLISA_MONITORS = 3 };
int bmlisa_monitor(void);
const char *bmlisa_monitor_name(int monitor);
void bmlisa_set_monitor(int monitor);
/* Speed: the Lisa's own 5 MHz, or as fast as the Pi can. */
int bmlisa_fast(void);
void bmlisa_set_fast(int fast);
/* The hard disk image, and choosing another (the Lisa starts again; kept
   in bmlisa.cfg). */
const char *bmlisa_hard_disk(void);
int bmlisa_set_hard_disk(const char *path);
/* Puts a floppy image in drive DRIVE (0, or the Lisa 1's 1). */
int bmlisa_insert_floppy(const char *path, int drive);
const char *bmlisa_floppy(int drive);
/* Reboot or power off, closing files first. */
void bmlisa_request_power(int power_off);
/* Boots the Lisa again, as switching it off and on does. */
void bmlisa_restart(void);
/* Web UI requests waiting to be done; menus close for them. */
int bmlisa_pending_requests(void);

/* keymap.c: USB HID usage to Lisa keyboard code, or -1. */
int bmlisa_usage_to_lisa(int usage);

/* menu.c */
void bmlisa_menu(void);

#endif /* BMLISA_PORT_H_ */
