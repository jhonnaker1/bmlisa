/*
 * cpu/m68k.h for IDLE: its 68000, Musashi 3.3 as IDLE changed it, isn't
 * in BMLisa (its licence allows non-commercial use only), so IDLE's files
 * find this one instead ("../cpu/m68k.h": see src/port/idle/README.md).
 * It is Musashi 4's m68k.h (third_party/musashi), with the registers
 * IDLE's copy had beyond it (lisa_cpu.c).
 *
 * This file is part of BMLisa, a bare metal Apple Lisa emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMLISA_IDLE_CPU_M68K_H_
#define BMLISA_IDLE_CPU_M68K_H_

#include "musashi/m68k.h"
#include "lisa_cpu.h"

#define M68K_REG_PPPC         BMLISA_M68K_REG_PPPC
#define M68K_REG_CPU_RUN_MODE BMLISA_M68K_REG_CPU_RUN_MODE
#define M68K_REG_CPU_STOPPED  BMLISA_M68K_REG_CPU_STOPPED

#define m68k_get_reg bmlisa_m68k_get_reg

#endif /* BMLISA_IDLE_CPU_M68K_H_ */
