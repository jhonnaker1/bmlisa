/*
 * lisa_cpu.c - what IDLE needs from its 68000 that Musashi doesn't do the
 * same way (see bmlisa_m68kconf.h and src/port/idle/cpu/).
 *
 * IDLE ran on its own copy of Musashi 3.3, which it changed for the Lisa;
 * BMLisa runs Musashi 4 (third_party/musashi), unmodified, and does those
 * changes here, after IDLE's (its cpu/m68kcpu.h, BSD licence):
 *
 *  - Bus errors (bmlisa_bus_error()): the Lisa's MMU raises one for an
 *    address that isn't mapped (IDLE's mmu.c), and its software, the
 *    Office System above all, relies on how. Musashi 4's bus error writes
 *    a 68010's frame; the 68000's is written here, with IDLE's corrections
 *    to the program counter stacked: Musashi runs an instruction at a time
 *    without the 68000's prefetch, so for a jump, a return or JSR that
 *    faults, the 68000 had already moved on in a way IDLE measured on a
 *    real Lisa.
 *  - The instruction before last (IDLE's REG_PPPC), for those corrections.
 *  - Registers IDLE asks for that Musashi doesn't have, and the supervisor
 *    bit for the MMU (IDLE's GetS()).
 *
 * This file is part of BMLisa, a bare metal Apple Lisa emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#include <setjmp.h>

#include "musashi/m68kcpu.h"
#include "idle/cpu/lisa_cpu.h"

extern jmp_buf m68ki_bus_error_jmp_buf;     /* m68kcpu.c */
extern int ask_bkpt;                        /* lisa.cpp, for IDLE */

/* ---- The instruction before last --------------------------------------- */

static unsigned int instr_pc, instr_pc_before;

void bmlisa_instr_hook(unsigned int pc)
{
	instr_pc_before = instr_pc;
	instr_pc = pc;
}

/* ---- Interrupts ---------------------------------------------------------- */

/* The level stays: the device lowers it (IDLE's irq.c). */
int bmlisa_int_ack(int int_level)
{
	(void) int_level;
	return M68K_INT_ACK_AUTOVECTOR;
}

/* ---- Registers ------------------------------------------------------------- */

unsigned int bmlisa_m68k_get_reg(void *context, int reg)
{
	switch (reg) {
	case BMLISA_M68K_REG_PPPC:
		return instr_pc_before;
	case BMLISA_M68K_REG_CPU_RUN_MODE:
		return CPU_RUN_MODE;
	case BMLISA_M68K_REG_CPU_STOPPED:
		return CPU_STOPPED;
	default:
		return m68k_get_reg(context, (m68k_register_t) reg);
	}
}

struct bmlisa_cpu_flags bmlisa_cpu_flags(void)
{
	struct bmlisa_cpu_flags flags;
	flags.s_flag = FLAG_S;
	return flags;
}

/* ---- Bus errors ------------------------------------------------------------ */

/* The program counter the 68000 stacks for a bus error in the instruction
   IR (IDLE's m68ki_stack_frame_buserr()). SR is the status register before
   the exception. */
static unsigned int stacked_pc(unsigned int sr)
{
	switch (REG_IR) {
	/* Measured on a real Lisa (IDLE): the 68000 had prefetched from the
	   jump's destination. */
	case 0x4e75:    /* RTS */
	case 0x4ef9:    /* JMP absolute long */
	/* IDLE's reckoning (the German Office System 2.0 needs JMP (An)). */
	case 0x4ed0: case 0x4ed1: case 0x4ed2: case 0x4ed3:
	case 0x4ed4: case 0x4ed5: case 0x4ed6: case 0x4ed7:
	case 0x4ed8: case 0x4ed9: case 0x4eda: case 0x4edb:
	case 0x4edc: case 0x4edd: case 0x4ede: case 0x4edf:
	case 0x4e73:    /* RTE */
		return (instr_pc_before + 2) & CPU_ADDRESS_MASK;
	/* JSR: the return address pushed is taken back off. */
	case 0x4eb9:    /* JSR absolute long */
	case 0x4e90:    /* JSR (A0) */
	case 0x4ea8:    /* JSR d16(A0) */
		if (sr & 0x2000)
			REG_SP = MASK_OUT_ABOVE_32(REG_SP + 4);       /* the SSP */
		else
			REG_USP = MASK_OUT_ABOVE_32(REG_USP + 4);
		return (instr_pc_before + 2) & CPU_ADDRESS_MASK;
	case 0x4a2f:    /* TST.B d16(A7) */
		return (REG_PPC + 2) & CPU_ADDRESS_MASK;
	default:
		return REG_PC & CPU_ADDRESS_MASK;
	}
}

void bmlisa_bus_error(unsigned int address, int write_mode)
{
	unsigned int sr;

	/* A bus error while one is being taken halts the processor; IDLE then
	   asked for its debugger (ask_bkpt), and BMLisa stops (lisa.cpp). */
	if (CPU_RUN_MODE == RUN_MODE_BERR_AERR_RESET_WSF) {
		CPU_STOPPED = STOP_LEVEL_HALT;
		SET_CYCLES(0);
		ask_bkpt = 1;
		return;
	}
	CPU_RUN_MODE = RUN_MODE_BERR_AERR_RESET_WSF;

	m68ki_aerr_address = address;
	m68ki_aerr_write_mode = (unsigned int) write_mode;
	CPU_PREF_ADDR = 0x1000;     /* the prefetch is lost */

	sr = m68ki_init_exception();

	/* The 68000's frame: PC, SR, the instruction, the address, and the
	   access (R/W, instruction/not, function code). */
	m68ki_push_32(stacked_pc(sr));
	m68ki_push_16(sr);
	m68ki_push_16(REG_IR);
	m68ki_push_32(m68ki_aerr_address);
	m68ki_push_16(m68ki_aerr_write_mode | CPU_INSTR_MODE | m68ki_aerr_fc);

	m68ki_jump_vector(EXCEPTION_BUS_ERROR);

	/* IDLE's timing: the exception's cycles, less the instruction's. */
	USE_CYCLES(CYC_EXCEPTION[EXCEPTION_ADDRESS_ERROR] - CYC_INSTRUCTION[REG_IR]);
	CPU_RUN_MODE = RUN_MODE_NORMAL;

	/* Back to m68k_execute(), which goes on at the handler. */
	longjmp(m68ki_bus_error_jmp_buf, 1);
}
