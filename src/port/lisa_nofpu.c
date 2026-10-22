/*
 * lisa_nofpu.c - the floating point functions Musashi's FPU calls, which
 * the Lisa never does.
 *
 * Musashi 4 emulates the 68881/68882 and the 68040's FPU (its m68kfpu.c,
 * which m68kcpu.c includes) with SoftFloat 2b (third_party/musashi/
 * softfloat). SoftFloat's licence makes those who use it indemnify its
 * author, a restriction the GNU GPL doesn't allow, so BMLisa doesn't build
 * it. The Lisa's processor is a 68000, which has no FPU: Musashi gives its
 * F-line instructions the 68000's Line 1111 exception (m68k_in.c's
 * 040fpu0 and 040fpu1 check for a 68030 or later), so the FPU code is
 * never run, but it is linked. These stand in for the SoftFloat functions
 * it calls, and say so in the log if they ever are.
 *
 * This file is part of BMLisa, a bare metal Apple Lisa emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#include <stdio.h>

#include "musashi/m68kcpu.h"   /* SoftFloat's types (softfloat.h) */

int8 float_rounding_mode = 0;

static void not_a_68000(const char *name)
{
	static int said;
	if (!said) {
		printf("BMLisa: the 68000 has no FPU, but %s was called\n", name);
		said = 1;
	}
}

static floatx80 zero_x80(const char *name)
{
	floatx80 z = {0, 0};
	not_a_68000(name);
	return z;
}

floatx80 int32_to_floatx80(int32 a) { (void) a; return zero_x80(__func__); }
floatx80 float32_to_floatx80(float32 a) { (void) a; return zero_x80(__func__); }
floatx80 float64_to_floatx80(float64 a) { (void) a; return zero_x80(__func__); }

int32 floatx80_to_int32(floatx80 a) { (void) a; not_a_68000(__func__); return 0; }
int32 floatx80_to_int32_round_to_zero(floatx80 a) { (void) a; not_a_68000(__func__); return 0; }
float32 floatx80_to_float32(floatx80 a) { (void) a; not_a_68000(__func__); return 0; }
float64 floatx80_to_float64(floatx80 a) { (void) a; not_a_68000(__func__); return 0; }

floatx80 floatx80_add(floatx80 a, floatx80 b) { (void) a; (void) b; return zero_x80(__func__); }
floatx80 floatx80_sub(floatx80 a, floatx80 b) { (void) a; (void) b; return zero_x80(__func__); }
floatx80 floatx80_mul(floatx80 a, floatx80 b) { (void) a; (void) b; return zero_x80(__func__); }
floatx80 floatx80_div(floatx80 a, floatx80 b) { (void) a; (void) b; return zero_x80(__func__); }
floatx80 floatx80_rem(floatx80 a, floatx80 b) { (void) a; (void) b; return zero_x80(__func__); }
floatx80 floatx80_sqrt(floatx80 a) { (void) a; return zero_x80(__func__); }

flag floatx80_is_nan(floatx80 a) { (void) a; not_a_68000(__func__); return 0; }
