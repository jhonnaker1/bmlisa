/*
 * idle_port.h - included before each of IDLE's files (-include, see
 * idle.mk), so that IDLE runs on BMLisa with few changes.
 *
 * fopen(): IDLE opens its files by fixed names in the current folder: the
 * ROMs as bios/341-0175-h and the like, the ProFile hard disk as
 * profile.raw, a debug.lst of functions to trace. There is no current
 * folder on the Pi, and BMLisa keeps ROMs in roms/ and lets the hard disk
 * be chosen, so bmlisa_idle_fopen() (lisa.cpp) finds them; other paths
 * (floppy images, which BMLisa names in full) are opened as they are.
 *
 * The hard disk: IDLE read and wrote a raw image of 5 or 10 MB; six lines
 * of its via_hdd.c call BMLisa's functions instead (below), which also
 * read LisaEm's hard disks, and larger ones.
 *
 * exit(): IDLE stops the program when the Lisa switches itself off (its
 * COPS gets the power off command) or a hard disk image can't be made.
 * bmlisa_idle_exit() goes back to BMLisa's frame loop instead (lisa.cpp),
 * which shows that the Lisa is off.
 *
 * This file is part of BMLisa, a bare metal Apple Lisa emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMLISA_IDLE_PORT_H_
#define BMLISA_IDLE_PORT_H_

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

FILE *bmlisa_idle_fopen(const char *path, const char *mode);
void bmlisa_idle_exit(int status) __attribute__((noreturn));
/* The ProFile's blocks (532 bytes: 20 of tags, then 512 of data) in the
   hard disk image, raw or LisaEm's Disk Copy 4.2 (lisa.cpp); IDLE's
   via_hdd.c calls these in place of its fseek(), fread() and fwrite(), and
   bmlisa_profile_size() in place of ftell() (the size as a raw image). */
int bmlisa_profile_read(FILE *f, int block, unsigned char *buf);
int bmlisa_profile_write(FILE *f, int block, const unsigned char *buf);
long bmlisa_profile_size(FILE *f);
/* The hard disk's blocks, and byte I of its spare table (the ProFile's
   answer to block $FFFFFF: its name and size), for via_hdd.c. */
long bmlisa_profile_blocks(void);
int bmlisa_profile_spare(int i);
/* Added to via_hdd.c: its ProFile as at power on (another image, or the
   Lisa switched off and on). */
void bmlisa_profile_power_on(void);
/* Added to mmu.c: the MMU and memory as at power on. */
void bmlisa_mmu_power_on(void);
/* Added to disk_image.c: whether IDLE's DRIVE has a disk. */
int bmlisa_floppy_present(int drive);

#ifdef __cplusplus
}
#endif

#define fopen bmlisa_idle_fopen
#define exit bmlisa_idle_exit

#endif /* BMLISA_IDLE_PORT_H_ */
