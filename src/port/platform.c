/*
 * platform.c - BMLisa's front end for IDLE, the Lisa emulator.
 *
 * Does what IDLE's SDL front end (main_sdl.cpp) did, on top of the Circle
 * kernel services in src/host.h: starts the Lisa (through lisa.cpp, which
 * runs IDLE), runs it a frame at a time, shows its screen, and passes on
 * keys and the mouse. The menu is in menu.c and osd.c.
 *
 * Screen: the Lisa's is 720 x 364 pixels, one bit each; it is drawn into
 * the frame buffer in the monitor's colour. Its pixels are taller than
 * they are wide (the screen is 4:3).
 *
 * Timing: IDLE's Lisa runs 59.5 frames a second (LISA_FRAME_RATE). When
 * the display refreshes at (close to) that rate, each frame is shown on its
 * own display refresh, so the mouse moves smoothly; otherwise as many
 * frames are run as the time since starting calls for.
 *
 * This file is part of BMLisa, a bare metal Apple Lisa emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>

#include "../host.h"
#include "bmlisa.h"
#include "lisa.h"
#include "osd.h"

/* ---- Settings ---------------------------------------------------------------- */

const char *bmlisa_sd_root(void)
{
	return host_option("root", "/");
}

static int option_on(const char *name, int default_value)
{
	const char *value = host_option(name, NULL);
	if (value == NULL || *value == '\0')
		return default_value;
	return strcmp(value, "1") == 0 || strcmp(value, "on") == 0
	    || strcmp(value, "yes") == 0 || strcmp(value, "true") == 0;
}

/* PATH in the card's DIR folder ("" for the root). */
static void card_path(char *path, size_t size, const char *dir, const char *name)
{
	const char *root = bmlisa_sd_root();
	size_t len = strlen(root);
	snprintf(path, size, "%s%s%s%s%s", root, len > 0 && root[len - 1] == '/' ? "" : "/",
	         dir, dir[0] && name[0] ? "/" : "", name);
}

static void save_setting(const char *name, const char *value)
{
	if (!host_option_set(name, value) || !host_options_save())
		printf("BMLisa: cannot save %s= in bmlisa.cfg\n", name);
}

/* ---- Video ------------------------------------------------------------------- */

/* The frame buffer: the Lisa's screen, and room for the on-screen display's
   640x460. */
#define FB_W LISA_SCREEN_W
#define FB_H 460

static uint16_t *fb_pixels;
static int fb_pitch_px;
static int osd_view;

static double refresh_rate = 60.0;
static int vsync_lock;

uint16_t *bmlisa_framebuffer(int *pitch_px)
{
	*pitch_px = fb_pitch_px;
	return fb_pixels;
}

/* The Lisa's screen is 4:3 (its pixels about 2:3); aspect= in cmdline.txt
   multiplies that. */
static void apply_view(void)
{
	double aspect = 4.0 / 3.0 * host_option_double("aspect", 1.0);
	if (osd_view)
		host_video_set_view(0, 0, 640, 460, 640.0 / 460.0);
	else
		host_video_set_view(0, 0, LISA_SCREEN_W, LISA_SCREEN_H, aspect);
}

static int redraw_all = TRUE;

void bmlisa_osd_view(int on)
{
	osd_view = on;
	apply_view();
	if (!on)
		redraw_all = TRUE;
}

static void clear_framebuffer(void)
{
	int y;
	for (y = 0; y < FB_H; y++)
		memset(fb_pixels + y * fb_pitch_px, 0, FB_W * 2);
}

static void measure_refresh_rate(void)
{
	enum { SKIP = 3, COUNT = 20 };
	double start = 0.0, rate;
	int i;
	for (i = 0; i < SKIP + COUNT; i++) {
		host_video_present(TRUE);
		if (i == SKIP - 1)
			start = host_time();
	}
	rate = COUNT / (host_time() - start);
	if (rate > 20.0 && rate < 250.0)
		refresh_rate = rate;
	printf("BMLisa: display refresh rate %.3f Hz\n", refresh_rate);
}

static void update_sync_mode(void)
{
	const char *mode = host_option("vsync", "auto");
	if (strcmp(mode, "off") == 0 || strcmp(mode, "0") == 0)
		vsync_lock = FALSE;
	else if (strcmp(mode, "on") == 0 || strcmp(mode, "1") == 0)
		vsync_lock = TRUE;
	else {
		/* The Lisa's 59.5 Hz: a 60 Hz display is right. */
		double ratio = refresh_rate / LISA_FRAME_RATE;
		vsync_lock = ratio > 0.98 && ratio < 1.02;
	}
	printf("BMLisa: display %.2f Hz, Lisa %.2f Hz, %s\n", refresh_rate, LISA_FRAME_RATE,
	       vsync_lock ? "synchronised to the display" : "timer paced");
}

double bmlisa_refresh_rate(void)
{
	return refresh_rate;
}

int bmlisa_vsync_locked(void)
{
	return vsync_lock;
}

void bmlisa_video_settings_changed(void)
{
	update_sync_mode();
	apply_view();
}

/* The display's resolution setting changed (the Pi 5): the mode is set
   again for the Lisa's rate (from BMA800XL). */
void bmlisa_display_mode_changed(void)
{
	if (host_display_match_rate(LISA_FRAME_RATE))
		measure_refresh_rate();
	update_sync_mode();
	apply_view();
}

/* ---- Monitors --------------------------------------------------------- */

/* The Lisa's screen is white on black; green and amber are there for
   those who like them. */
static const struct {
	const char *key;            /* monitor= */
	const char *name;
	uint16_t rgb565;            /* the phosphor's colour */
} monitors[BMLISA_MONITORS] = {
	{"white", "White", 0xffff},
	{"green", "Green", (0x33 >> 3) << 11 | (0xff >> 2) << 5 | (0x33 >> 3)},
	{"amber", "Amber", (0xff >> 3) << 11 | (0xb0 >> 2) << 5 | 0},
};

static int monitor;

int bmlisa_monitor(void)
{
	return monitor;
}

const char *bmlisa_monitor_name(int m)
{
	return m >= 0 && m < BMLISA_MONITORS ? monitors[m].name : "";
}

void bmlisa_set_monitor(int m)
{
	if (m < 0 || m >= BMLISA_MONITORS)
		return;
	save_setting("monitor", monitors[m].key);
	monitor = m;
	redraw_all = TRUE;
}

static int monitor_from_option(void)
{
	const char *want = host_option("monitor", "white");
	int m;
	for (m = 0; m < BMLISA_MONITORS; m++)
		if (strcmp(want, monitors[m].key) == 0)
			return m;
	return 0;
}

/* The Lisa's screen into the frame buffer, when it changed: each bit set is
   a black pixel. The bottom lines are drawn every frame, as the
   notification is drawn over them. */
static void copy_screen(void)
{
	int changed, y;
	const uint8_t *screen = bmlisa_lisa_screen(&changed);
	uint16_t on = monitors[monitor].rgb565;
	int first = changed || redraw_all ? 0 : LISA_SCREEN_H - OSD_OVERLAY_LINES;
	redraw_all = FALSE;
	for (y = first; y < LISA_SCREEN_H; y++) {
		const uint8_t *src = screen + (size_t) y * LISA_SCREEN_BYTES_PER_LINE;
		uint16_t *dst = fb_pixels + (size_t) y * fb_pitch_px;
		int x;
		for (x = 0; x < LISA_SCREEN_BYTES_PER_LINE; x++) {
			unsigned bits = src[x];
			int b;
			for (b = 0; b < 8; b++)
				*dst++ = (bits & (0x80 >> b)) ? 0 : on;
		}
	}
}

/* ---- Files ---------------------------------------------------------------- */

/* IDLE writes to the hard disk and floppy images as the Lisa writes to its
   disks, but the SD card's file system caches what it can. Since the Pi can
   be switched off at any time, push everything out to the card once a
   second, and the parameter memory (lisa.h) if it changed. */
static void sync_files(void)
{
	int fd;
	if (!bmlisa_lisa_save_parameter_memory())
		printf("BMLisa: cannot save the parameter memory\n");
	fflush(NULL);
	for (fd = 3; fd < 30; fd++)
		fsync(fd);
}

/* ---- Models and ROMs ------------------------------------------------------ */

/* The models, in the order BMLisa prefers them, with IDLE's names for their
   boot ROM's two chips (mmu.c, MemInit()). Both need the video state PROM,
   vidstate.rom, too. MAME's lisa2.zip has the Lisa 2's under these names. */
static const struct {
	const char *key;            /* machine= in cmdline.txt / bmlisa.cfg */
	const char *name;
	int model;                  /* IDLE's */
	const char *rom_hi, *rom_lo;
} machines[BMLISA_MACHINES] = {
	{"lisa2", "Lisa 2", LISA_MODEL_2, "341-0175-h", "341-0176-h"},
	{"lisa1", "Lisa 1", LISA_MODEL_1, "L175REVC.bin", "L176REVC.bin"},
};

static int current_machine = -1;
static char rom_dir[256];

static int rom_is_there(const char *name, long size)
{
	char path[320];
	struct stat st;
	snprintf(path, sizeof path, "%s/%s", rom_dir, name);
	return stat(path, &st) == 0 && S_ISREG(st.st_mode) && st.st_size == size;
}

int bmlisa_machine_available(int machine)
{
	return machine >= 0 && machine < BMLISA_MACHINES
	    && rom_is_there(machines[machine].rom_hi, 8192)
	    && rom_is_there(machines[machine].rom_lo, 8192)
	    && rom_is_there("vidstate.rom", 256);
}

/* The model to start: machine= if its ROMs are there, else the first that
   has them. */
static int choose_machine(void)
{
	const char *want = host_option("machine", "auto");
	int m;
	for (m = 0; m < BMLISA_MACHINES; m++)
		if (strcmp(want, machines[m].key) == 0 && bmlisa_machine_available(m))
			return m;
	for (m = 0; m < BMLISA_MACHINES; m++)
		if (bmlisa_machine_available(m))
			return m;
	return -1;
}

int bmlisa_machine(void)
{
	return current_machine;
}

const char *bmlisa_machine_name(int machine)
{
	return machine >= 0 && machine < BMLISA_MACHINES ? machines[machine].name : "none";
}

/* IDLE starts its Lisa once: another model starts after a reboot. */
void bmlisa_set_machine(int machine)
{
	if (!bmlisa_machine_available(machine) || machine == current_machine)
		return;
	save_setting("machine", machines[machine].key);
	bmlisa_request_power(0);
}

/* ---- Speed ------------------------------------------------------------------ */

static int fast;

int bmlisa_fast(void)
{
	return fast;
}

void bmlisa_set_fast(int on)
{
	fast = on != 0;
}

/* ---- Disks ------------------------------------------------------------------ */

static char hard_disk[512];
static char floppy_name[2][512];

const char *bmlisa_hard_disk(void)
{
	return hard_disk;
}

/* A Lisa 1 has two floppy drives, a Lisa 2 one. The name is forgotten once
   the disk is out (the Lisa ejects them itself). */
const char *bmlisa_floppy(int drive)
{
	if (drive < 0 || drive >= 2)
		return NULL;
	if (!bmlisa_lisa_floppy_present(drive))
		floppy_name[drive][0] = '\0';
	return floppy_name[drive][0] ? floppy_name[drive] : NULL;
}

void bmlisa_eject_floppy(int drive)
{
	if (bmlisa_lisa_floppy_drives() == 1)
		drive = 0;
	if (drive < 0 || drive > 1)
		return;
	bmlisa_lisa_eject_floppy(drive);
	floppy_name[drive][0] = '\0';
	printf("BMLisa: floppy drive %d ejected\n", drive + 1);
}

int bmlisa_insert_floppy(const char *path, int drive)
{
	if (bmlisa_lisa_floppy_drives() == 1)
		drive = 0;
	if (drive < 0 || drive > 1 || !bmlisa_lisa_insert_floppy(path, drive)) {
		printf("BMLisa: cannot use %s as a floppy\n", path);
		return FALSE;
	}
	printf("BMLisa: floppy %s in drive %d\n", path, drive + 1);
	snprintf(floppy_name[drive], sizeof floppy_name[drive], "%s", path);
	return TRUE;
}

/* The hard disk image from harddisk= (a path on the card), or
   disks/profile.raw; made, blank (10 MB, as a ProFile-10), if it isn't
   there, so that an operating system can be installed on it. */
static int open_hard_disk(void)
{
	const char *want = host_option("harddisk", "");
	struct stat st;
	FILE *f;
	if (strcmp(want, "none") == 0) {
		/* Ejected (the menu): no ProFile. */
		hard_disk[0] = '\0';
		printf("BMLisa: no hard disk\n");
		return TRUE;
	}
	if (want[0] == '/')
		snprintf(hard_disk, sizeof hard_disk, "%s", want);
	else if (want[0] != '\0')
		card_path(hard_disk, sizeof hard_disk, "", want);
	else {
		/* disks/profile.raw, or LisaEm's disks/profile.dc42 if only that
		   is there. */
		char dc42[sizeof hard_disk];
		card_path(hard_disk, sizeof hard_disk, "disks", "profile.raw");
		card_path(dc42, sizeof dc42, "disks", "profile.dc42");
		if (stat(hard_disk, &st) != 0 && stat(dc42, &st) == 0)
			snprintf(hard_disk, sizeof hard_disk, "%s", dc42);
	}
	if (stat(hard_disk, &st) != 0) {
		if (bmlisa_lisa_make_hard_disk(hard_disk, TRUE))
			printf("BMLisa: made a blank hard disk, %s\n", hard_disk);
		else
			printf("BMLisa: cannot make a hard disk, %s\n", hard_disk);
	}
	f = fopen(hard_disk, "rb+");
	if (f == NULL) {
		printf("BMLisa: cannot open the hard disk %s\n", hard_disk);
		return FALSE;
	}
	fclose(f);
	printf("BMLisa: hard disk %s\n", hard_disk);
	return TRUE;
}

/* A ProFile image of 5 or 10 MB, or one of LisaEm's larger sizes: raw, or
   LisaEm's Disk Copy 4.2 (lisa.cpp). Anything else, a wrong size, IDLE
   took as it was, and the Lisa only showed its error 84. */
static int is_hard_disk_image(const char *path)
{
	return bmlisa_lisa_hard_disk_blocks(path, NULL) != 0;
}

/* Another hard disk: the Lisa starts again from it, on a ProFile switched
   off and on (lisa.cpp). (Changing just the file under IDLE's ProFile left
   the boot ROM with its error 52.) */
int bmlisa_set_hard_disk(const char *path)
{
	if (!is_hard_disk_image(path) || !bmlisa_lisa_change_hard_disk(path))
		return FALSE;
	snprintf(hard_disk, sizeof hard_disk, "%s", path);
	save_setting("harddisk", hard_disk);
	printf("BMLisa: hard disk %s\n", hard_disk);
	bmlisa_restart();
	return TRUE;
}

/* The hard disk unplugged: no ProFile until another is chosen (kept, as
   harddisk=none). */
void bmlisa_eject_hard_disk(void)
{
	bmlisa_lisa_eject_hard_disk();
	hard_disk[0] = '\0';
	save_setting("harddisk", "none");
	printf("BMLisa: hard disk ejected\n");
}

/* ---- Power ---------------------------------------------------------------- */

/* The Lisa switched itself off (its software shut down): its screen goes
   dark until a key or the mouse button switches it on. */
static int lisa_off;

void bmlisa_restart(void)
{
	lisa_off = FALSE;
	bmlisa_lisa_power_on();
	redraw_all = TRUE;
}

void bmlisa_power_button(void)
{
	if (!lisa_off)
		bmlisa_lisa_power_button();
}

static void lisa_switched_off(void)
{
	lisa_off = TRUE;
	clear_framebuffer();
	osd_notify("The Lisa is off: a key or click switches it on.");
	printf("BMLisa: the Lisa switched itself off\n");
	if (!bmlisa_lisa_sync_hard_disk())
		printf("BMLisa: cannot write the hard disk's checksums\n");
	sync_files();
}

/* ---- Requests from the web UI ------------------------------------------ */

static volatile int request_power = -1;   /* 0 reboot, 1 power off */
static volatile int request_coldstart = FALSE;
static char autostart_path[600];
static volatile int request_autostart = FALSE;

void bmlisa_request_power(int power_off)
{
	request_power = power_off ? 1 : 0;
}

/* Called by the web UI. */
void bmlisa_request_coldstart(void)
{
	request_coldstart = TRUE;
}

/* Called by the web UI ("start this file"), on core 0: the emulator takes
   it up between frames (handle_requests()). One at a time. */
void emu_autostart_interrupt(const char *path)
{
	if (request_autostart)
		return;
	snprintf(autostart_path, sizeof autostart_path, "%s", path);
	__sync_synchronize();
	request_autostart = TRUE;
}

int bmlisa_pending_requests(void)
{
	return request_power >= 0 || request_coldstart || request_autostart;
}

/* A hard disk image becomes the hard disk (and the Lisa starts from it); a
   floppy image goes in the drive. */
static void autostart(const char *path)
{
	if (is_hard_disk_image(path)) {
		printf("Web UI: starting %s\n", path);
		if (!bmlisa_set_hard_disk(path))
			printf("Web UI: cannot use %s\n", path);
	} else {
		printf("Web UI: inserting %s\n", path);
		bmlisa_insert_floppy(path, 0);
	}
}

static void handle_requests(void)
{
	if (request_autostart) {
		__sync_synchronize();
		autostart(autostart_path);
		request_autostart = FALSE;
	}
	if (request_coldstart) {
		request_coldstart = FALSE;
		printf("Web UI: restart\n");
		bmlisa_restart();
	}
	if (request_power >= 0) {
		int power_off = request_power;
		request_power = -1;
		printf("%s\n", power_off ? "Shutting down" : "Rebooting");
		if (!bmlisa_lisa_sync_hard_disk())
			printf("BMLisa: cannot write the hard disk's checksums\n");
		sync_files();
		if (power_off)
			host_power_off();
		else
			host_reboot();
	}
}

static double last_sync_time;

void bmlisa_idle(void)
{
	double now = host_time();
	if (now - last_sync_time >= 1.0) {
		last_sync_time = now;
		sync_files();
	}
	if (request_power >= 0)
		handle_requests();
}

/* ---- Input --------------------------------------------------------------- */

static int open_menu;
static int caps_locked;
static int pad_last_menu[HOST_PADS];

static void process_input(void)
{
	int usage, pressed, pad, dx, dy, buttons;
	static int last_buttons;

	while (host_key_event(&usage, &pressed)) {
		int code;
		if (usage == USAGE_POWER || usage == KEY_POWER) {
			if (pressed)
				bmlisa_power_button();
			continue;
		}
		if (usage == KEY_MENU) {
			if (pressed)
				open_menu = TRUE;
			continue;
		}
		if (lisa_off) {
			if (pressed)
				bmlisa_restart();
			continue;
		}
		code = bmlisa_usage_to_lisa(usage);
		if (code < 0)
			continue;
		if (usage == USAGE_CAPS_LOCK) {
			/* Locks down on the Lisa: pressed once, released the next
			   time. */
			if (!pressed)
				continue;
			caps_locked = !caps_locked;
			pressed = caps_locked;
		}
		bmlisa_lisa_key(code, pressed);
	}

	if (host_mouse_read(&dx, &dy, &buttons)) {
		if (lisa_off) {
			if ((buttons & 1) && !(last_buttons & 1))
				bmlisa_restart();
		} else {
			if (dx != 0 || dy != 0)
				bmlisa_lisa_mouse(dx, dy);
			/* The Lisa's mouse has one button: any of the USB mouse's. */
			if ((buttons != 0) != (last_buttons != 0))
				bmlisa_lisa_mouse_button(buttons != 0);
		}
		last_buttons = buttons;
	}

	/* Gamepad Home, or Select and Start together, open the menu. */
	for (pad = 0; pad < HOST_PADS; pad++) {
		int state = host_pad_state(pad);
		int menu;
		if (state < 0)
			state = 0;
		menu = (state & HOST_PAD_HOME)
		       || ((state & HOST_PAD_SELECT) && (state & HOST_PAD_START));
		if (menu && !pad_last_menu[pad])
			open_menu = TRUE;
		pad_last_menu[pad] = menu;
	}
}

/* ---- Main loop ------------------------------------------------------------- */

/* Shows the web UI's address once the network is up. */
static void show_network_address(void)
{
	static int shown;
	char address[32];
	if (shown || !option_on("webui", TRUE))
		return;
	if (host_network_address(address, sizeof(address))) {
		char text[64];
		snprintf(text, sizeof(text), "Web UI: http://%s/", address);
		osd_notify(text);
		shown = TRUE;
	}
}

static void stop_with(const char *message)
{
	printf("BMLisa: stopped: %s\n", message);
	osd_open();
	osd_message(message, FALSE);
	for (;;) {
		bmlisa_idle();
		handle_requests();
		host_video_present(TRUE);
	}
}

/* Runs a frame of the Lisa; returns FALSE if it can't go on. */
static int run_frame(void)
{
	int r;
	if (lisa_off)
		return TRUE;
	r = bmlisa_lisa_run_frame();
	if (r == 1)
		lisa_switched_off();
	return r >= 0;
}

void bmlisa_main(void)
{
	int pitch;
	const char *err;
	double last_second, start, frames_run;

	if (host_video_init(FB_W, FB_H, &fb_pixels, &pitch) != 0) {
		printf("BMLisa: no display\n");
		for (;;)
			host_sleep_us(1000000);
	}
	fb_pitch_px = pitch / 2;
	clear_framebuffer();
	apply_view();
	/* Where the host can set the video mode (the Pi 4), the display runs
	   at the Lisa's 59.5 Hz, then it is timed. */
	host_display_match_rate(LISA_FRAME_RATE);
	measure_refresh_rate();
	update_sync_mode();
	monitor = monitor_from_option();

	card_path(rom_dir, sizeof rom_dir, "roms", "");
	{
		/* The Lisa's battery-backed settings (lisa.h). */
		char pram[512];
		card_path(pram, sizeof pram, "", "bmlisa.pram");
		bmlisa_lisa_set_parameter_memory_file(pram);
	}
	current_machine = choose_machine();
	if (current_machine < 0) {
		printf("BMLisa: no Lisa ROMs in roms/ (see roms/ROMS.txt)\n");
		stop_with("There are no Lisa ROMs in the roms folder. See roms/ROMS.txt on the card.");
	}
	if (!open_hard_disk())
		stop_with("The hard disk image could not be opened or made: is the card full, or locked?");
	if (hard_disk[0] != '\0' && !is_hard_disk_image(hard_disk)) {
		printf("BMLisa: %s isn't a ProFile image\n", hard_disk);
		stop_with("The hard disk image isn't a ProFile's (5 or 10 MB, or LisaEm's "
		          "up to 512 MB): a raw image (blocks of 532 bytes) or LisaEm's (.dc42).");
	}
	bmlisa_lisa_set_paths(rom_dir, hard_disk);
	printf("BMLisa: %s, ROMs %s and %s\n", machines[current_machine].name,
	       machines[current_machine].rom_hi, machines[current_machine].rom_lo);
	err = bmlisa_lisa_start(machines[current_machine].model);
	if (err != NULL) {
		printf("BMLisa: %s\n", err);
		stop_with(err);
	}
	{
		const char *floppy = host_option("floppy", "");
		if (floppy[0] != '\0') {
			char path[512];
			if (floppy[0] == '/')
				snprintf(path, sizeof path, "%s", floppy);
			else
				card_path(path, sizeof path, "", floppy);
			bmlisa_insert_floppy(path, 0);
		}
	}

	last_second = start = host_time();
	frames_run = 0;
	for (;;) {
		double now;

		process_input();
		if (open_menu) {
			open_menu = FALSE;
			bmlisa_menu();
			redraw_all = TRUE;
			start = host_time();
			frames_run = 0;
		}

		if (fast) {
			/* As many frames as fit in most of a display refresh, up to
			   16 (the simulator's time only moves on with the display). */
			double until = host_time() + 0.8 / refresh_rate;
			int n = 0;
			do {
				if (!run_frame())
					stop_with("The Lisa's processor stopped. The log (see the User Guide) says where.");
			} while (++n < 16 && host_time() < until && !lisa_off);
		} else if (vsync_lock) {
			if (!run_frame())
				stop_with("The Lisa's processor stopped. The log (see the User Guide) says where.");
		} else {
			/* The frames the time since starting calls for, at most a
			   few at once (after a pause, it carries on from there). */
			int n = 0;
			now = host_time();
			while (frames_run < (now - start) * LISA_FRAME_RATE && n < 4) {
				if (!run_frame())
					stop_with("The Lisa's processor stopped. The log (see the User Guide) says where.");
				frames_run++;
				n++;
			}
			if (n == 4) {
				start = now;
				frames_run = 0;
			}
		}
		if (lisa_off)
			clear_framebuffer();
		else
			copy_screen();
		osd_draw_notification(fb_pixels, fb_pitch_px, FB_W, LISA_SCREEN_H);
		host_video_present(vsync_lock || fast);

		now = host_time();
		if (now - last_second >= 1.0) {
			last_second = now;
			sync_files();
			show_network_address();
		}
		handle_requests();
	}
}
