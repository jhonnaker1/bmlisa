/*
 * keymap.c - USB keyboard keys to Lisa keyboard codes.
 *
 * The kernel reports keys as USB HID usage codes; the Lisa's keyboard
 * sends its COPS a key's row and column (IDLE's via_keyb.c). The codes are
 * IDLE's (main_sdl.cpp, sdl2lisa_scancode() and its table of the keyboard's
 * rows), for the US keyboard.
 *
 * The Lisa has no Control, Escape, function or arrow keys. Its Apple
 * (command) key is the PC's Windows key, or a Mac keyboard's command key,
 * and Ctrl too (as in IDLE); its Option keys are Alt. Its Enter key, beside
 * the space bar, is the PC's Menu key. Caps Lock locks down on the Lisa,
 * so platform.c sends it pressed and released on alternate presses.
 *
 * This file is part of BMLisa, a bare metal Apple Lisa emulator for the
 * Raspberry Pi. See LICENSE for licensing details.
 */

#include "bmlisa.h"

/* Indexed by USB HID usage; 0 is no key (no Lisa key is 0). */
static const unsigned char usage_to_lisa[256] = {
	[0x04] = 0x70, /* a */  [0x05] = 0x6e, /* b */  [0x06] = 0x6d, /* c */
	[0x07] = 0x7b, /* d */  [0x08] = 0x60, /* e */  [0x09] = 0x69, /* f */
	[0x0a] = 0x6a, /* g */  [0x0b] = 0x6b, /* h */  [0x0c] = 0x53, /* i */
	[0x0d] = 0x54, /* j */  [0x0e] = 0x55, /* k */  [0x0f] = 0x59, /* l */
	[0x10] = 0x58, /* m */  [0x11] = 0x6f, /* n */  [0x12] = 0x5f, /* o */
	[0x13] = 0x44, /* p */  [0x14] = 0x75, /* q */  [0x15] = 0x65, /* r */
	[0x16] = 0x76, /* s */  [0x17] = 0x66, /* t */  [0x18] = 0x52, /* u */
	[0x19] = 0x6c, /* v */  [0x1a] = 0x77, /* w */  [0x1b] = 0x7a, /* x */
	[0x1c] = 0x67, /* y */  [0x1d] = 0x79, /* z */
	[0x1e] = 0x74, /* 1 */  [0x1f] = 0x71, /* 2 */  [0x20] = 0x72, /* 3 */
	[0x21] = 0x73, /* 4 */  [0x22] = 0x64, /* 5 */  [0x23] = 0x61, /* 6 */
	[0x24] = 0x62, /* 7 */  [0x25] = 0x63, /* 8 */  [0x26] = 0x50, /* 9 */
	[0x27] = 0x51, /* 0 */
	[0x28] = 0x48, /* Return */
	[0x2a] = 0x45, /* Backspace */  [0x2b] = 0x78, /* Tab */
	[0x2c] = 0x5c, /* Space */      [0x2d] = 0x40, /* - */
	[0x2e] = 0x41, /* = */          [0x2f] = 0x56, /* [ */
	[0x30] = 0x57, /* ] */          [0x31] = 0x42, /* \ */
	[0x32] = 0x42, /* non-US # */   [0x33] = 0x5a, /* ; */
	[0x34] = 0x5b, /* ' */          [0x35] = 0x68, /* ` */
	[0x36] = 0x5d, /* , */          [0x37] = 0x5e, /* . */
	[0x38] = 0x4c, /* / */          [0x39] = 0x7d, /* Caps Lock */
	[0x53] = 0x20, /* Num Lock: keypad Clear */
	[0x54] = 0x27, /* keypad / */   [0x55] = 0x22, /* keypad * */
	[0x56] = 0x21, /* keypad - */   [0x57] = 0x2b, /* keypad + */
	[0x58] = 0x2f, /* keypad Enter */
	[0x59] = 0x4d, /* keypad 1 */   [0x5a] = 0x2d, /* keypad 2 */
	[0x5b] = 0x2e, /* keypad 3 */   [0x5c] = 0x28, /* keypad 4 */
	[0x5d] = 0x29, /* keypad 5 */   [0x5e] = 0x2a, /* keypad 6 */
	[0x5f] = 0x24, /* keypad 7 */   [0x60] = 0x25, /* keypad 8 */
	[0x61] = 0x26, /* keypad 9 */   [0x62] = 0x49, /* keypad 0 */
	[0x63] = 0x2c, /* keypad . */   [0x64] = 0x43, /* ISO \ (< >) */
	[0x65] = 0x46, /* Menu: Enter */
	[0x67] = 0x23, /* keypad = */
	[0xe0] = 0x7f, /* left Control: Apple */
	[0xe1] = 0x7e, /* left Shift */
	[0xe2] = 0x7c, /* left Alt: left Option */
	[0xe3] = 0x7f, /* left Windows/command: Apple */
	[0xe4] = 0x7f, /* right Control: Apple */
	[0xe5] = 0x7e, /* right Shift */
	[0xe6] = 0x4e, /* right Alt: right Option */
	[0xe7] = 0x7f, /* right Windows/command: Apple */
};

int bmlisa_usage_to_lisa(int usage)
{
	if (usage < 0 || usage > 0xff || usage_to_lisa[usage] == 0)
		return -1;
	return usage_to_lisa[usage];
}
