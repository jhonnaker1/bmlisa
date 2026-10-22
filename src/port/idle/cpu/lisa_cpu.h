/*
 * lisa_cpu.h - lisa_cpu.c's functions: what IDLE's 68000 (Musashi 3.3, as
 * IDLE changed it) had that Musashi 4 doesn't.
 *
 * This file is part of BMLisa, a bare metal Apple Lisa emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMLISA_IDLE_CPU_LISA_CPU_H_
#define BMLISA_IDLE_CPU_LISA_CPU_H_

#ifdef __cplusplus
extern "C" {
#endif

/* The registers IDLE's Musashi had beyond Musashi 4's, for
   bmlisa_m68k_get_reg(): the instruction before last, the run mode
   (whether an exception is being taken) and the stopped state. */
#define BMLISA_M68K_REG_PPPC         0x1000
#define BMLISA_M68K_REG_CPU_RUN_MODE 0x1001
#define BMLISA_M68K_REG_CPU_STOPPED  0x1002

/* m68k_get_reg(), with those too. */
unsigned int bmlisa_m68k_get_reg(void *context, int reg);

/* A bus error in the access at ADDRESS (WRITE_MODE: Musashi's MODE_READ
   or MODE_WRITE), as IDLE's MMU raises them: the 68000's exception is
   taken and the instruction left (it doesn't return), but for a bus error
   while one is being taken, which halts the processor. */
void bmlisa_bus_error(unsigned int address, int write_mode);

/* The supervisor bit, as IDLE's GetS() reads it (m68ki_cpu.s_flag). */
struct bmlisa_cpu_flags {
	unsigned int s_flag;
};
struct bmlisa_cpu_flags bmlisa_cpu_flags(void);

#ifdef __cplusplus
}
#endif

#endif /* BMLISA_IDLE_CPU_LISA_CPU_H_ */
