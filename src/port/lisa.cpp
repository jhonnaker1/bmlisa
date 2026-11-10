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

static void detect_hard_disk(void);

void bmlisa_lisa_set_paths(const char *roms, const char *disk)
{
	snprintf(rom_dir, sizeof rom_dir, "%s", roms);
	snprintf(hard_disk, sizeof hard_disk, "%s", disk);
	detect_hard_disk();
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

/* ---- Hard disk images ---------------------------------------------------- */

/* The ProFile's blocks are 532 bytes: 20 of tags, then 512 of data. IDLE's
   images are those blocks one after another (raw). LisaEm's are Disk Copy
   4.2 images: an 84 byte header (a name, then the data's and tags' sizes
   and checksums, big endian), all the blocks' data, then all their tags,
   with the blocks in the order of the Lisa's 5:1 interleave (LisaEm's
   profile.c, deinterleave5()). IDLE's via_hdd.c reads and writes blocks
   through the functions here. */

#define PROFILE_BLOCKS_5MB  0x2600L
#define PROFILE_BLOCKS_10MB 0x4c00L
#define DC42_HEADER 84

/* The ProFile sizes: IDLE's 5 and 10 MB, and the larger ones LisaEm makes
   (its profile.c), with the size its spare table's name gives them. */
static const struct {
	long blocks;
	char size[4];
} profile_sizes[] = {
	{PROFILE_BLOCKS_5MB, "   "}, {PROFILE_BLOCKS_10MB, "10M"}, {32768, "16M"},
	{40960, "20M"}, {65536, "32M"}, {81920, "40M"}, {131072, "64M"},
	{262144, "128"}, {524288, "256"}, {1048576, "512"},
};
#define PROFILE_SIZES ((int) (sizeof profile_sizes / sizeof profile_sizes[0]))

static int profile_size_index(long blocks)
{
	int i;
	for (i = 0; i < PROFILE_SIZES; i++)
		if (profile_sizes[i].blocks == blocks)
			return i;
	return -1;
}

static unsigned long be32(const unsigned char *p)
{
	return (unsigned long) p[0] << 24 | (unsigned long) p[1] << 16 | (unsigned long) p[2] << 8 | p[3];
}

/* The image's blocks if PATH is a ProFile image, raw or Disk Copy 4.2
   (*DC42 says which), else 0. */
static long image_blocks(const char *path, int *dc42)
{
	unsigned char head[DC42_HEADER];
	FILE *f = fopen(path, "rb");
	long size, blocks = 0;
	*dc42 = 0;
	if (f == NULL)
		return 0;
	if (fseek(f, 0, SEEK_END) == 0 && (size = ftell(f)) > 0) {
		if (size % 532 == 0 && profile_size_index(size / 532) >= 0)
			blocks = size / 532;
		else if ((size - DC42_HEADER) % 532 == 0
		         && profile_size_index((size - DC42_HEADER) / 532) >= 0
		         && fseek(f, 0, SEEK_SET) == 0 && fread(head, 1, sizeof head, f) == sizeof head
		         && head[0] <= 63) {
			long n = (size - DC42_HEADER) / 532;
			if (be32(head + 64) == (unsigned long) n * 512 && be32(head + 68) == (unsigned long) n * 20) {
				blocks = n;
				*dc42 = 1;
			}
		}
	}
	fclose(f);
	return blocks;
}

int bmlisa_lisa_hard_disk_blocks(const char *path, int *dc42)
{
	int unused;
	return (int) image_blocks(path, dc42 != NULL ? dc42 : &unused);
}

/* The hard disk's: its blocks, whether it is LisaEm's, and whether its
   checksums need writing again. */
static long disk_blocks;
static int disk_dc42;
static int disk_dirty;

static void detect_hard_disk(void)
{
	disk_blocks = hard_disk[0] != '\0' ? image_blocks(hard_disk, &disk_dc42) : 0;
	disk_dirty = 0;
}

/* Where ProFile block BLOCK is in a LisaEm image: in each 16, block b is
   at (b * 13) mod 16 (LisaEm's deinterleave5() table). */
static long dc42_index(long block)
{
	return (block & ~15L) | ((block * 13) & 15);
}

extern "C" long bmlisa_profile_blocks(void)
{
	return disk_blocks > 0 ? disk_blocks : PROFILE_BLOCKS_10MB;
}

/* The spare table: IDLE's own for 5 and 10 MB (a ProFile and a Widget);
   for larger disks LisaEm's (get_profile_spare_table()): "PROFILE 40M",
   device $000010, and the disk's blocks. After 4 bytes of status: the
   name (13 bytes), the device (3), the firmware (2), the blocks (3), the
   bytes a block (2), the spares (1, and 2 used), then $FFFFFF. */
extern "C" int bmlisa_profile_spare(int i)
{
	extern unsigned char profile_str[], widget_str[];     /* via_hdd.c */
	static unsigned char table[34];
	enum { LENGTH = 34 };
	int k = profile_size_index(disk_blocks);
	if (i < 0 || i >= LENGTH)
		return 0;
	if (k <= 0)
		return profile_str[i];
	if (k == 1)
		return widget_str[i];
	memset(table, 0, sizeof table);
	memcpy(table + 4, "PROFILE      ", 13);
	memcpy(table + 4 + 8, profile_sizes[k].size, 3);
	table[4 + 15] = 0x10;
	table[4 + 16] = 0x03;
	table[4 + 17] = 0x98;
	table[4 + 18] = (unsigned char) (disk_blocks >> 16);
	table[4 + 19] = (unsigned char) (disk_blocks >> 8);
	table[4 + 20] = (unsigned char) disk_blocks;
	table[4 + 21] = 0x02;
	table[4 + 22] = 0x14;
	table[4 + 23] = 0x20;
	table[4 + 26] = table[4 + 27] = table[4 + 28] = 0xff;
	return table[i];
}

extern "C" long bmlisa_profile_size(FILE *f)
{
	if (disk_dc42)
		return disk_blocks * 532;
	return fseek(f, 0, SEEK_END) == 0 ? ftell(f) : -1;
}

extern "C" int bmlisa_profile_read(FILE *f, int block, unsigned char *buf)
{
	if (f == NULL)
		return 0;
	if (!disk_dc42) {
		if (fseek(f, (long) block * 532, SEEK_SET) != 0)
			return 0;
		return (int) fread(buf, 1, 532, f);
	}
	memset(buf, 0, 532);
	if (block < 0 || block >= disk_blocks)
		return 0;
	block = (int) dc42_index(block);
	if (fseek(f, DC42_HEADER + disk_blocks * 512 + (long) block * 20, SEEK_SET) != 0
	    || fread(buf, 1, 20, f) != 20
	    || fseek(f, DC42_HEADER + (long) block * 512, SEEK_SET) != 0
	    || fread(buf + 20, 1, 512, f) != 512)
		return 0;
	return 532;
}

extern "C" int bmlisa_profile_write(FILE *f, int block, const unsigned char *buf)
{
	if (f == NULL)
		return 0;
	if (!disk_dc42) {
		if (fseek(f, (long) block * 532, SEEK_SET) != 0)
			return 0;
		return (int) fwrite(buf, 532, 1, f);
	}
	if (block < 0 || block >= disk_blocks)
		return 0;
	disk_dirty = 1;
	block = (int) dc42_index(block);
	if (fseek(f, DC42_HEADER + disk_blocks * 512 + (long) block * 20, SEEK_SET) != 0
	    || fwrite(buf, 1, 20, f) != 20
	    || fseek(f, DC42_HEADER + (long) block * 512, SEEK_SET) != 0
	    || fwrite(buf + 20, 1, 512, f) != 512)
		return 0;
	return 1;
}

/* Disk Copy's checksum (as LisaEm's libdc42 has it): each big endian word
   added, then the sum rotated right a bit. */
static unsigned long dc42_sum(FILE *f, long offset, long length, unsigned long sum)
{
	static unsigned char chunk[65536];
	if (fseek(f, offset, SEEK_SET) != 0)
		return sum;
	while (length > 0) {
		long n = length < (long) sizeof chunk ? length : (long) sizeof chunk, i;
		if ((long) fread(chunk, 1, n, f) != n)
			break;
		for (i = 0; i + 1 < n; i += 2) {
			sum = (sum + ((unsigned long) chunk[i] << 8) + chunk[i + 1]) & 0xffffffffUL;
			sum = ((sum >> 1) | (sum << 31)) & 0xffffffffUL;
		}
		length -= n;
	}
	return sum;
}

int bmlisa_lisa_sync_hard_disk(void)
{
	extern FILE *profile_file;      /* via_hdd.c */
	FILE *f = profile_file;
	unsigned char sums[8];
	unsigned long data, tags;
	if (!disk_dc42 || !disk_dirty || f == NULL)
		return 1;
	data = dc42_sum(f, DC42_HEADER, disk_blocks * 512, 0);
	/* The tags' leave out the first block's (libdc42: "not a bug"). */
	tags = dc42_sum(f, DC42_HEADER + disk_blocks * 512 + 20, (disk_blocks - 1) * 20, 0);
	sums[0] = data >> 24; sums[1] = data >> 16; sums[2] = data >> 8; sums[3] = data;
	sums[4] = tags >> 24; sums[5] = tags >> 16; sums[6] = tags >> 8; sums[7] = tags;
	if (fseek(f, 72, SEEK_SET) != 0 || fwrite(sums, 1, sizeof sums, f) != sizeof sums)
		return 0;
	disk_dirty = 0;
	return 1;
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
	if (hard_disk[0] != '\0')
		check_profile_image();
	init_via_hdd_io(-1);            /* the ProFile is off if there's none */
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

int bmlisa_lisa_change_hard_disk(const char *path)
{
	FILE *f;
	if ((f = fopen(path, "rb+")) == NULL)
		return 0;
	fclose(f);
	/* The old disk's checksums, if it is LisaEm's; bmlisa_lisa_power_on()
	   then opens the new one. */
	bmlisa_lisa_sync_hard_disk();
	snprintf(hard_disk, sizeof hard_disk, "%s", path);
	detect_hard_disk();
	return 1;
}

void bmlisa_lisa_eject_hard_disk(void)
{
	extern FILE *profile_file;      /* via_hdd.c */
	bmlisa_lisa_sync_hard_disk();
	if (profile_file != NULL)
		fclose(profile_file);
	profile_file = NULL;
	hard_disk[0] = '\0';
	detect_hard_disk();
	via_turn_off_PROFILE();
}

/* Every device as at power on, as when a Lisa is switched off and on: the
   CPU's reset (bmlisa_lisa_reset()) left them as they were, interrupting
   and part way through, and the boot ROM then stopped (its error 52, or
   the processor halted). The hard disk is opened again (it may be
   another), and the parameter memory is kept. */
void clean_fifo(void);              /* via_keyb.c: the COPS's queue */

static void power_cycle_devices(void)
{
	extern FILE *profile_file;      /* via_hdd.c */
	extern int irq_status;          /* irq.c */
	bmlisa_lisa_save_parameter_memory();
	irq_status = 0;
	m68k_set_irq(M68K_IRQ_NONE);
	via_reset();
	clean_fifo();
	init_via_keyb_io();
	if (profile_file != NULL)
		fclose(profile_file);
	profile_file = NULL;
	if (hard_disk[0] != '\0')
		check_profile_image();
	bmlisa_profile_power_on();
	init_via_hdd_io(-1);            /* the ProFile is off if there's none */
	init_floppy_io(io_revision);
	load_parameter_memory();
	scc_init();
	bmlisa_mmu_power_on();
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
	power_cycle_devices();
	bmlisa_lisa_reset();
}

void bmlisa_lisa_nmi(void)
{
	extern int irq_status;          /* irq.c */
	/* Musashi takes the NMI on the way up to level 7 and keeps it until it
	   is taken; the level then goes back to IDLE's, so that the next NMI
	   is a new edge. */
	m68k_set_irq(M68K_IRQ_7);
	m68k_set_irq((irq_status & IRQ2_MASK) ? M68K_IRQ_2
	             : (irq_status & IRQ1_MASK) ? M68K_IRQ_1 : M68K_IRQ_NONE);
}

/* ---- Floppies -------------------------------------------------------------- */

int bmlisa_lisa_floppy_drives(void)
{
	return lisa_model == LISA_MODEL_1 ? 2 : 1;
}

/* IDLE's drive for the Lisa's DRIVE: the Lisa 2's is IDLE's second
   (main_sdl.cpp leaves the first out of its menu for the Lisa 2's I/O
   ROM). */
static int idle_drive(int drive)
{
	return lisa_model != LISA_MODEL_1 ? 1 : drive;
}

int bmlisa_lisa_floppy_present(int drive)
{
	return drive >= 0 && drive <= 1 && bmlisa_floppy_present(idle_drive(drive));
}

void bmlisa_lisa_eject_floppy(int drive)
{
	if (drive >= 0 && drive <= 1)
		unclamp(idle_drive(drive));
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
