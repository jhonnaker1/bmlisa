/*
 * scc.cpp - the Lisa's serial ports, a Zilog 8530 SCC, with nothing
 * plugged in.
 *
 * IDLE's own (io/8530scc.c, Mini vMac's SCC under version 2 of the GNU
 * GPL only, which BMLisa can't combine with Circle) is left out. That one
 * kept the chip's registers but nothing was connected to it, and it raised
 * no interrupts (its notify hook was never set): software saw ports with
 * no cable, sending at once. This is that, BMLisa's own: scc_init(), which
 * IDLE's io/8530scc.h declares and lisa.cpp calls, puts it at the I/O
 * addresses IDLE's did.
 *
 *  - Writing: WR0's pointer (and its "point high" command) chooses the
 *    next register; the registers are kept; WR9 resets a channel or both;
 *    data written is sent at once (and dropped).
 *  - Reading: RR0 says the transmitter is empty and nothing came in, RR1
 *    that everything was sent; RR2 is the interrupt vector, RR3 no
 *    interrupt pending, RR8 the received data (none), RR12, RR13 and RR15
 *    the registers written; RR4-7, 9, 11 and 14 repeat others, as on the
 *    chip.
 *
 * Compiled with IDLE's headers, as lisa.cpp is.
 *
 * This file is part of BMLisa, a bare metal Apple Lisa emulator for
 * the Raspberry Pi. See LICENSE for licensing details.
 */

#include <string.h>

#include "config.h"
#include "mmu/mmu.h"
#include "io/8530scc.h"

enum { CHANNEL_A, CHANNEL_B };

/* RR0: Tx buffer empty, Tx underrun/EOM. RR1: all sent, residue code. */
#define RR0_IDLE 0x44
#define RR1_IDLE 0x07

static struct {
	unsigned char wr[16];
} channel[2];
static unsigned char wr2, wr9;     /* shared by both channels */
static int pointer;                /* the register the next access is to */

static void reset_channel(int c)
{
	memset(channel[c].wr, 0, sizeof channel[c].wr);
}

static unsigned char read_register(int c, int reg)
{
	switch (reg) {
	case 0: case 4:
		return RR0_IDLE;
	case 1: case 5:
		return RR1_IDLE;
	case 2: case 6:
		return wr2;
	case 3: case 7:
		return 0;               /* no interrupt pending */
	case 8:
		return 0;               /* nothing received */
	case 10: case 14:
		return 0;
	case 12:
		return channel[c].wr[12];
	case 13: case 9:
		return channel[c].wr[13];
	case 15: case 11:
		return channel[c].wr[15];
	default:
		return 0;
	}
}

static void write_register(int c, int reg, unsigned char value)
{
	switch (reg) {
	case 0:
		/* The register pointer, and "point high". */
		pointer = (value & 0x07) | (((value >> 3) & 0x07) == 1 ? 8 : 0);
		channel[c].wr[0] = value;
		return;
	case 2:
		wr2 = value;
		return;
	case 8:
		return;                 /* sent, to nothing */
	case 9:
		switch (value & 0xc0) {
		case 0xc0:              /* hardware reset */
			reset_channel(CHANNEL_A);
			reset_channel(CHANNEL_B);
			wr2 = 0;
			break;
		case 0x80:
			reset_channel(CHANNEL_A);
			break;
		case 0x40:
			reset_channel(CHANNEL_B);
			break;
		}
		wr9 = value & 0x3f;
		return;
	default:
		channel[c].wr[reg] = value;
		return;
	}
}

/* An access at the SCC's address A (IDLE's SCC_Access(), with the address
   halved): bit 0 chooses the channel (0 is B), bit 1 the control or data
   register. */
static int scc_access_at(unsigned long a, int value, int write)
{
	int c = (a & 1) ? CHANNEL_A : CHANNEL_B;
	int reg;
	if (((a >> 1) & 1) == 0) {
		reg = pointer;          /* control: the register pointed to */
		pointer = 0;
	} else
		reg = 8;                /* data */
	if (write) {
		write_register(c, reg, (unsigned char) value);
		return value;
	}
	return read_register(c, reg);
}

static void scc_access(uint32 address, int *val, int mode, int size)
{
	int result = scc_access_at(address >> 1, *val, mode == IO_WRITE);
	(void) size;
	if (mode != IO_WRITE)
		*val = result;
}

static void scc_access16(uint32 address, int *val, int mode, int size)
{
	int result;
	if (size != IO_WORD) {
		*val = 0xff;
		return;
	}
	result = scc_access_at(address >> 1, *val, mode == IO_WRITE);
	if (mode != IO_WRITE)
		*val = result;
}

void scc_init(void)
{
	uint32 i;
	for (i = 0xd000; i < 0xd400; i += 2) {
		registerIoFuncAdr(i + 1, &scc_access);
		registerIoFuncAdr(i, &scc_access16);
	}
	reset_channel(CHANNEL_A);
	reset_channel(CHANNEL_B);
	wr2 = wr9 = 0;
	pointer = 0;
}
