/*
 * lisa.cpp - runs IDLE, the Lisa emulator, for BMLisa (see lisa.h).
 *
 * Does what IDLE's SDL front end, main_sdl.cpp, did: starts the Lisa
 * (main()), runs it a frame at a time (lisa_loop()), and passes it keys
 * and the mouse (handleSdlKeyEvent() and the mouse events). Compiled as
 * C++ with IDLE's headers, as IDLE is (see idle.mk).
 *
 * This file is part of BMLisa, a bare metal Apple Lisa emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

#include <errno.h>
#include <setjmp.h>
#include <string.h>

#include "config.h"
#include "cpu/m68k.h"
#include "mmu/mmu.h"
#include "io/via.h"
#include "io/8530scc.h"
#include "disk/disk_image.h"
#include "disk/disk_ctrl.h"
#include "time/timer.h"
#include "irq/irq.h"

#include "lisa.h"

/* ---- What IDLE's core needs from its front end --------------------------- */

/* The debugger's breakpoint request, and its windows' text. */
int ask_bkpt = 0;
static char mmu_box_text[0x10000];
static char sharedram_box_text[0x10000];
char *mmu_box = mmu_box_text;
char *sharedram_box = sharedram_box_text;

static int screen_changed = 1;

/* Called by the MMU for every byte written to the screen (main_sdl.cpp
   drew it then); the whole screen is converted once a frame instead. */
extern "C" void redraw_lisa_byte(uint32 video_offset)
{
	(void) video_offset;
	screen_changed = 1;
}

/* ---- Files (see idle_port.h) ------------------------------------------------ */

static char rom_dir[256];
static char hard_disk[512];

void bmlisa_lisa_set_paths(const char *roms, const char *disk)
{
	snprintf(rom_dir, sizeof rom_dir, "%s", roms);
	snprintf(hard_disk, sizeof hard_disk, "%s", disk);
}

#undef fopen

extern "C" FILE *bmlisa_idle_fopen(const char *path, const char *mode)
{
	char full[768];
	if (strncmp(path, "bios/", 5) == 0) {
		snprintf(full, sizeof full, "%s/%s", rom_dir, path + 5);
		return fopen(full, mode);
	}
	if (strcmp(path, "profile.raw") == 0) {
		if (hard_disk[0] == '\0') {
			errno = ENOENT;
			return NULL;
		}
		return fopen(hard_disk, mode);
	}
	if (strcmp(path, "debug.lst") == 0) {
		errno = ENOENT;
		return NULL;
	}
	return fopen(path, mode);
}

int bmlisa_lisa_make_hard_disk(const char *path, int ten_mb)
{
	static unsigned char block[532];
	/* IDLE's sizes (init_via_hdd_io()): blocks of 532 bytes. */
	long blocks = ten_mb ? 0x4c00 : 0x2600, i;
	FILE *f = fopen(path, "wb");
	if (f == NULL)
		return 0;
	for (i = 0; i < blocks; i++)
		if (fwrite(block, sizeof block, 1, f) != 1) {
			fclose(f);
			return 0;
		}
	return fclose(f) == 0;
}

/* ---- Parameter memory ------------------------------------------------------ */

/* The Lisa keeps its settings in 64 bytes of its floppy controller's RAM,
   which a battery keeps when it is switched off: the shared memory at
   $FCC181-$FCC1FF, odd bytes, which is IDLE's floppy_shared_tab[0xC0] to
   [0xFF] (disk_ctrl.c, floppy_shared()). IDLE clears it at every start
   (init_floppy_io()), so the Lisa found it blank each time, and MacWorks
   had no double-click time. BMLisa keeps it in a file instead. */

extern uint8 floppy_shared_tab[0x400];     /* disk_ctrl.c */

#define PARAMETER_MEMORY      0xC0
#define PARAMETER_MEMORY_SIZE 64

static char pram_path[512];
static uint8 pram_saved[PARAMETER_MEMORY_SIZE];

void bmlisa_lisa_set_parameter_memory_file(const char *path)
{
	snprintf(pram_path, sizeof pram_path, "%s", path);
}

/* A file of the wrong size is left alone (and is written over once the
   Lisa changes its settings). */
static void load_parameter_memory(void)
{
	uint8 pram[PARAMETER_MEMORY_SIZE];
	FILE *f;
	if (pram_path[0] == '\0' || (f = fopen(pram_path, "rb")) == NULL)
		return;
	if (fread(pram, 1, sizeof pram, f) == sizeof pram && fgetc(f) == EOF) {
		memcpy(&floppy_shared_tab[PARAMETER_MEMORY], pram, sizeof pram);
		memcpy(pram_saved, pram, sizeof pram);
		printf("BMLisa: parameter memory from %s\n", pram_path);
	} else
		printf("BMLisa: %s isn't 64 bytes: not used\n", pram_path);
	fclose(f);
}

int bmlisa_lisa_save_parameter_memory(void)
{
	const uint8 *now = &floppy_shared_tab[PARAMETER_MEMORY];
	FILE *f;
	if (pram_path[0] == '\0' || memcmp(now, pram_saved, sizeof pram_saved) == 0)
		return 1;
	f = fopen(pram_path, "wb");
	if (f == NULL || fwrite(now, 1, sizeof pram_saved, f) != sizeof pram_saved) {
		if (f != NULL)
			fclose(f);
		return 0;
	}
	if (fclose(f) != 0)
		return 0;
	memcpy(pram_saved, now, sizeof pram_saved);
	return 1;
}

/* ---- Stopping ---------------------------------------------------------------- */

/* IDLE calls exit() when the Lisa switches itself off; it comes back here,
   to where the frame (or the start) began. */
static jmp_buf stop_jump;
static int stop_jump_set;
static int powered_off;

extern "C" void bmlisa_idle_exit(int status)
{
	printf("BMLisa: IDLE stopped (%d)\n", status);
	if (stop_jump_set)
		longjmp(stop_jump, 1);
	/* Not while it runs: nothing to go back to. */
	for (;;)
		;
}

/* ---- Starting --------------------------------------------------------------- */

static int lisa_model;
static int io_revision;

const char *bmlisa_lisa_start(int model)
{
	static char error[128];
	const char *err;

	lisa_model = model;
	/* main_sdl.cpp's main(): the I/O board's ROM revision tells the floppy
	   controller which drives there are. */
	io_revision = model == LISA_MODEL_1 ? 0x40 : 0xa8;
	/* No bus error for I/O addresses nothing is at (main_sdl.cpp's
	   default, -ioberr changes it). */
	setWriteWildIO(0);

	if (setjmp(stop_jump) != 0) {
		stop_jump_set = 0;
		return "The hard disk image could not be opened.";
	}
	stop_jump_set = 1;
	err = MemInit(model);
	if (err != NULL) {
		stop_jump_set = 0;
		/* "fatal error needs file bios/341-0175-h" */
		const char *name = strstr(err, "bios/");
		if (name != NULL)
			snprintf(error, sizeof error, "The ROM %s is missing from the roms folder.",
			         name + 5);
		else
			snprintf(error, sizeof error, "%s", err);
		return error;
	}
	init_via_keyb_io();
	check_profile_image();
	init_via_hdd_io(-1);
	init_floppy_io(io_revision);
	load_parameter_memory();
	scc_init();

	m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
	m68k_pulse_reset();
	clock68k = 0;
	stop_jump_set = 0;
	return NULL;
}

int bmlisa_lisa_reopen_hard_disk(void)
{
	extern FILE *profile_file;      /* via_hdd.c */
	FILE *f;
	if (hard_disk[0] == '\0' || (f = fopen(hard_disk, "rb+")) == NULL)
		return 0;
	if (profile_file != NULL)
		fclose(profile_file);
	profile_file = f;
	return 1;
}

/* ---- Running ------------------------------------------------------------------ */

/* Cycles left over from the last line (main_sdl.cpp's "cycles"). */
static int cycles;

/* Runs the CPU until CYCLES reaches END, keeping IDLE's timers up to date
   after each instruction (main_sdl.cpp's loop, with _ACCURATE_TIMERS_ as
   IDLE's config.h has it). Returns 0, or -1 if the CPU stopped. */
static int run_until(int end)
{
	idle_time delta;
	delta.seconds = 0;
	while (cycles < end) {
		int instr_cycles = m68k_execute(1);
		/* The processor halted (a bus error while taking one): IDLE's
		   loop went to its debugger. */
		if (instr_cycles == -1 || ask_bkpt)
			return -1;
		/* Memory is shared with the video: a cycle in four (IDLE's). */
		cycles += ((instr_cycles / 4) + 1) * 4;
		delta.subseconds = ((instr_cycles / 4) + 1) * 4;
		idle_timer_update_clock(delta);
		idle_check_timers();
	}
	return 0;
}

static unsigned long frame_number;

int bmlisa_lisa_run_frame(void)
{
	int line;
	if (powered_off)
		return 1;
	if (setjmp(stop_jump) != 0) {
		stop_jump_set = 0;
		powered_off = 1;
		return 1;
	}
	stop_jump_set = 1;
	/* 364 lines shown, each with its horizontal blank... */
	for (line = 0; line < 364; line++) {
		set_trace_time((int) ((frame_number << 16) | line));
		if (run_until(180) < 0)
			goto stopped;
		beginHBL();
		if (run_until(220) < 0)
			goto stopped;
		endHBL();
		cycles -= 220;
	}
	/* ...then the vertical blank, 18 lines. */
	if (beginVBL())
		set_irq_line(VBLIRQ);
	for (line = 364; line < 382; line++) {
		set_trace_time((int) ((frame_number << 16) | line));
		if (run_until(220) < 0)
			goto stopped;
		cycles -= 220;
	}
	if (endVBL())
		reset_irq_line(VBLIRQ);
	frame_number++;
	stop_jump_set = 0;
	return 0;
stopped:
	stop_jump_set = 0;
	return -1;
}

const uint8_t *bmlisa_lisa_screen(int *changed)
{
	static const uint8_t *last;
	const uint8_t *screen = getVideoMemory();
	*changed = screen_changed || screen != last;
	screen_changed = 0;
	last = screen;
	return screen;
}

/* ---- Input --------------------------------------------------------------- */

/* Keys are the COPS's codes: the key's row and column, with bit 7 set when
   it is pressed (sdl2lisa_scancode()). */
void bmlisa_lisa_key(int code, int pressed)
{
	via_keyb_event_key((uint8) ((code & 0x7f) | (pressed ? 0x80 : 0)));
}

void bmlisa_lisa_mouse(int dx, int dy)
{
	/* The COPS sends at most a byte a direction at a time. */
	while (dx != 0 || dy != 0) {
		int x = dx < -128 ? -128 : dx > 127 ? 127 : dx;
		int y = dy < -128 ? -128 : dy > 127 ? 127 : dy;
		via_keyb_update_mouse_pos((uint8) (int8) x, (uint8) (int8) y);
		dx -= x;
		dy -= y;
	}
}

/* The mouse button is a key to the COPS, code 6. */
void bmlisa_lisa_mouse_button(int pressed)
{
	via_keyb_event_key(pressed ? 0x86 : 0x06);
}

void bmlisa_lisa_power_button(void)
{
	via_keyb_power_off();
}

void bmlisa_lisa_reset(void)
{
	/* main_sdl.cpp's lisa_reset() */
	mmuReset();
	m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
	ask_bkpt = 0;
	m68k_pulse_reset();
}

void bmlisa_lisa_power_on(void)
{
	powered_off = 0;
	bmlisa_lisa_reset();
}

void bmlisa_lisa_nmi(void)
{
	m68k_set_irq(M68K_IRQ_7);
}

/* ---- Floppies -------------------------------------------------------------- */

int bmlisa_lisa_floppy_drives(void)
{
	return lisa_model == LISA_MODEL_1 ? 2 : 1;
}

int bmlisa_lisa_insert_floppy(const char *path, int drive)
{
	char name[512];
	/* The Lisa 2's drive is IDLE's second (main_sdl.cpp leaves the first
	   out of its menu for the Lisa 2's I/O ROM). */
	if (lisa_model != LISA_MODEL_1)
		drive = 1;
	if (drive < 0 || drive > 1)
		return 0;
	snprintf(name, sizeof name, "%s", path);
	unclamp(drive);
	return dc42_insert(name, drive) == 0;
}
