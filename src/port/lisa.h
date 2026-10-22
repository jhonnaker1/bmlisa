/*
 * lisa.h - the emulated Lisa, as BMLisa's C code sees it (lisa.cpp, which
 * runs IDLE).
 *
 * This file is part of BMLisa, a bare metal Apple Lisa emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMLISA_LISA_H_
#define BMLISA_LISA_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The screen: 720 x 364 pixels, one bit each (1 is black), 90 bytes a
   line. */
#define LISA_SCREEN_W 720
#define LISA_SCREEN_H 364
#define LISA_SCREEN_BYTES_PER_LINE 90

/* IDLE's Lisa frame: 382 lines of 220 cycles at 5 MHz, 59.5 a second
   (main_sdl.cpp's loop). */
#define LISA_FRAME_RATE (5000000.0 / (382 * 220))

/* The Lisa models IDLE emulates, by their boot ROMs: the Lisa 1 (two
   Twiggy drives) and the Lisa 2 (one 3.5" drive). */
enum { LISA_MODEL_1 = 1, LISA_MODEL_2 = 2 };

/* Where IDLE's files are: the folder with the ROMs, and the ProFile hard
   disk image (which must exist; see bmlisa_lisa_make_hard_disk()). */
void bmlisa_lisa_set_paths(const char *rom_dir, const char *hard_disk);

/* The Lisa's parameter memory (its battery-backed settings: the startup
   device, the Lisa Office System's preferences, MacWorks's Mac parameter
   RAM with its double-click speed) is kept in the file at PATH: read when
   the Lisa starts, written by bmlisa_lisa_save_parameter_memory() when it
   changed. Set before bmlisa_lisa_start(). */
void bmlisa_lisa_set_parameter_memory_file(const char *path);
/* Writes the parameter memory out if it changed. Returns 0 if it can't. */
int bmlisa_lisa_save_parameter_memory(void);

/* Makes a blank ProFile image of 10 MB (or 5 MB) at PATH. Returns 0 if it
   can't. */
int bmlisa_lisa_make_hard_disk(const char *path, int ten_mb);

/* Starts the Lisa. Returns NULL, or what is wrong (a ROM missing). */
const char *bmlisa_lisa_start(int model);

/* Runs one frame (LISA_FRAME_RATE). Returns 0, 1 if the Lisa switched
   itself off (after it, bmlisa_lisa_power_on() starts it again), or -1 if
   the CPU stopped (IDLE's debugger would have been called). */
int bmlisa_lisa_run_frame(void);

/* The screen (see above), and whether it changed since last asked. */
const uint8_t *bmlisa_lisa_screen(int *changed);

/* A key, as a Lisa keyboard code (row and column, see keymap.c), pressed
   or released. */
void bmlisa_lisa_key(int code, int pressed);
/* The mouse: moved, and its button. */
void bmlisa_lisa_mouse(int dx, int dy);
void bmlisa_lisa_mouse_button(int pressed);
/* The power button: the Lisa's software then shuts down (and switches the
   Lisa off). */
void bmlisa_lisa_power_button(void);
/* Starts the Lisa again after it switched off, or resets it. */
void bmlisa_lisa_power_on(void);
void bmlisa_lisa_reset(void);
/* The NMI key (IDLE's F12): Lisa software's debugger, if it has one. */
void bmlisa_lisa_nmi(void);

/* The floppy drives: the Lisa 2 has one (DRIVE is then ignored), the
   Lisa 1 two, 0 (upper) and 1 (lower). */
int bmlisa_lisa_floppy_drives(void);
/* Puts a floppy image (Disk Copy 4.2) in DRIVE, taking out the one there
   (the Lisa ejects disks itself, but can't be told one was taken out).
   Returns 0 if it can't. */
int bmlisa_lisa_insert_floppy(const char *path, int drive);
/* The hard disk image changed (bmlisa_lisa_set_paths()): opened again.
   Returns 0 if it can't be. */
int bmlisa_lisa_reopen_hard_disk(void);

#ifdef __cplusplus
}
#endif

#endif /* BMLISA_LISA_H_ */
