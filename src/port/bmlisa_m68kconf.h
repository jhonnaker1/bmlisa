/*
 * bmlisa_m68kconf.h - how BMLisa builds Musashi, the 68000 emulator
 * (third_party/musashi, unmodified), in place of its m68kconf.h: Musashi's
 * m68k.h includes this file when MUSASHI_CNF names it (see idle.mk).
 *
 * The settings are those of IDLE's own copy of Musashi (3.3, changed by
 * IDLE, which BMLisa doesn't use; see docs/THIRD_PARTY.md): a 68000 only,
 * with its prefetch, address errors and function codes. Two of IDLE's
 * changes to Musashi are made with Musashi's callbacks instead:
 *
 *  - Interrupts: the Lisa's interrupt lines stay up until the device
 *    lowers them (IDLE's irq.c), so taking an interrupt mustn't clear the
 *    level, as Musashi's own acknowledge does (IDLE took that out of its
 *    copy). bmlisa_int_ack() only answers "autovector".
 *  - The instruction before last, which IDLE's bus errors need (its
 *    REG_PPPC): bmlisa_instr_hook() keeps it.
 *
 * Bus errors themselves are lisa_cpu.c's.
 *
 * This file is part of BMLisa, a bare metal Apple Lisa emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#ifndef BMLISA_M68KCONF_H_
#define BMLISA_M68KCONF_H_

#ifdef __cplusplus
extern "C" {
#endif
int bmlisa_int_ack(int int_level);
void bmlisa_instr_hook(unsigned int pc);
#ifdef __cplusplus
}
#endif

/* Musashi's m68kconf.h guard: its own file is never wanted with this one. */
#define M68KCONF__HEADER

#define M68K_OPT_OFF             0
#define M68K_OPT_ON              1
#define M68K_OPT_SPECIFY_HANDLER 2

#define M68K_COMPILE_FOR_MAME       M68K_OPT_OFF

/* The Lisa's processor is a 68000. */
#define M68K_EMULATE_010            M68K_OPT_OFF
#define M68K_EMULATE_EC020          M68K_OPT_OFF
#define M68K_EMULATE_020            M68K_OPT_OFF
#define M68K_EMULATE_030            M68K_OPT_OFF
#define M68K_EMULATE_040            M68K_OPT_OFF

#define M68K_SEPARATE_READS         M68K_OPT_OFF
#define M68K_SIMULATE_PD_WRITES     M68K_OPT_ON

#define M68K_EMULATE_INT_ACK        M68K_OPT_SPECIFY_HANDLER
#define M68K_INT_ACK_CALLBACK(A)    bmlisa_int_ack(A)

#define M68K_EMULATE_BKPT_ACK       M68K_OPT_OFF
#define M68K_BKPT_ACK_CALLBACK()    ((void) 0)

#define M68K_EMULATE_TRACE          M68K_OPT_OFF

#define M68K_EMULATE_RESET          M68K_OPT_OFF
#define M68K_RESET_CALLBACK()       ((void) 0)

#define M68K_CMPILD_HAS_CALLBACK    M68K_OPT_OFF
#define M68K_CMPILD_CALLBACK(v, r)  ((void) 0)

#define M68K_RTE_HAS_CALLBACK       M68K_OPT_OFF
#define M68K_RTE_CALLBACK()         ((void) 0)

#define M68K_TAS_HAS_CALLBACK       M68K_OPT_OFF
#define M68K_TAS_CALLBACK()         1

#define M68K_ILLG_HAS_CALLBACK      M68K_OPT_OFF
#define M68K_ILLG_CALLBACK(opcode)  0

#define M68K_TRAP_HAS_CALLBACK      M68K_OPT_OFF
#define M68K_TRAP_CALLBACK(trap)    0

#define M68K_EMULATE_FC             M68K_OPT_OFF
#define M68K_SET_FC_CALLBACK(A)     ((void) 0)

#define M68K_MONITOR_PC             M68K_OPT_OFF
#define M68K_SET_PC_CALLBACK(A)     ((void) 0)

#define M68K_INSTRUCTION_HOOK       M68K_OPT_SPECIFY_HANDLER
#define M68K_INSTRUCTION_CALLBACK(pc) bmlisa_instr_hook(pc)

#define M68K_EMULATE_PREFETCH       M68K_OPT_ON
#define M68K_EMULATE_ADDRESS_ERROR  M68K_OPT_ON

#define M68K_LOG_ENABLE             M68K_OPT_OFF
#define M68K_LOG_1010_1111          M68K_OPT_OFF
#define M68K_LOG_TRAP               M68K_OPT_OFF
#define M68K_LOG_FILEHANDLE         stderr

#define M68K_EMULATE_PMMU           M68K_OPT_OFF

#define M68K_USE_64_BIT             M68K_OPT_ON

#endif /* BMLISA_M68KCONF_H_ */
