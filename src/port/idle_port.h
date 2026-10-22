/*
 * idle_port.h - included before each of IDLE's files (-include, see
 * idle.mk), so that IDLE runs unmodified on BMLisa.
 *
 * fopen(): IDLE opens its files by fixed names in the current folder: the
 * ROMs as bios/341-0175-h and the like, the ProFile hard disk as
 * profile.raw, a debug.lst of functions to trace. There is no current
 * folder on the Pi, and BMLisa keeps ROMs in roms/ and lets the hard disk
 * be chosen, so bmlisa_idle_fopen() (lisa.cpp) finds them; other paths
 * (floppy images, which BMLisa names in full) are opened as they are.
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

#ifdef __cplusplus
}
#endif

#define fopen bmlisa_idle_fopen
#define exit bmlisa_idle_exit

#endif /* BMLISA_IDLE_PORT_H_ */
