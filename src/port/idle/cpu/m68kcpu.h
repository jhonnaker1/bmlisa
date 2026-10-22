/*
 * cpu/m68kcpu.h for IDLE (see m68k.h here): the parts of its Musashi's
 * internals IDLE's MMU uses, its bus errors and the supervisor bit, from
 * lisa_cpu.c.
 *
 * This file is part of BMLisa, a bare metal Apple Lisa emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMLISA_IDLE_CPU_M68KCPU_H_
#define BMLISA_IDLE_CPU_M68KCPU_H_

#include "m68k.h"

/* The access a bus error happened in (Musashi's values). */
#define MODE_READ  0x10
#define MODE_WRITE 0

#define m68ki_bus_error(ADDR, WRITE_MODE) bmlisa_bus_error((ADDR), (WRITE_MODE))

/* IDLE's config.h reads the supervisor bit as m68ki_cpu.s_flag. */
#define m68ki_cpu bmlisa_cpu_flags()

#endif /* BMLISA_IDLE_CPU_M68KCPU_H_ */
