/*
Copyright (c) 2020 Jorge Giner Cordero

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "input.h"
#include "kernel/kernel.h"
#include "cbase/cbase.h"
#include "cbase/kassert.h"

static struct vkey_codes {
	int ksc1, ksc2, ksc3;
} s_keys[GAME_NKEYS] = {
	{ -1, KERNEL_KSC_UP, -1 },
	{ -1, KERNEL_KSC_DOWN, KERNEL_KSC_TAB },
	{ -1, KERNEL_KSC_LEFT, -1 },
	{ -1, KERNEL_KSC_RIGHT, -1 },
	{ KERNEL_KSC_X, KERNEL_KSC_RETURN, -1 },
	{ KERNEL_KSC_C, -1, -1 },
	{ -1, -1, -1 },
	{ -1, KERNEL_KSC_ESC, -1 },
};

/* Maps joystick 0 scancodes to virtual keys. */
static struct pad_vkeys {
	int key;
} s_buttons[KERNEL_NPAD_BUTTONS] = {
	{ KEYA },	/* A */
	{ KEYB },	/* B */
	{ KEYX },	/* X */
	{ KEYY },	/* Y */
	{ -1 },		/* BACK */
	{ -1 },		/* GUIDE */
	{ KEYY },	/* START */
	{ -1 },		/* LSTICK */
	{ -1 },		/* RSTICK */
	{ KDOWN },	/* LSHOULDER */
	{ KDOWN },	/* RSHOULDER */
	{ KUP },	/* DUP */
	{ KDOWN },	/* DDOWN */
	{ KLEFT },	/* DLEFT */
	{ KRIGHT },	/* DRIGHT */
};

enum {
	AXIS_LIMIT = 8000
};

static struct axis_dir {
	int axis;
	int dir;
} s_axis_dir[GAME_NKEYS] = {
	{ KERNEL_AXIS_V1, -1 },
	{ KERNEL_AXIS_V1, 1 },
	{ KERNEL_AXIS_H1, -1 },
	{ KERNEL_AXIS_H1, 1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
};

/* Redefine a game key to a kernel scan code.
 * Meant to be used with: 
 *	KUP, KDOWN, KLEFT, KRIGHT,
 *     	KEYA, KEYB, KEYX, KEYY .
 */
void redefine_key(int game_key, int ksc)
{
	if (game_key < 0 || game_key > KEYY)
		return;

	if (ksc < 0 || ksc >= KERNEL_KSC_PAD0_A)
		return;

	s_keys[game_key].ksc1 = ksc;
}

/* Returns the kernel value of a game key.
 * Returns -1 if game_key is not
 *	KUP, KDOWN, KLEFT, KRIGHT,
 *     	KEYA, KEYB, KEYX, KEYY .
 */
int get_game_key_value(int game_key)
{
	if (kassert_fails(game_key >= 0 && game_key <= KEYY)) {
		return -1;
	}
	return s_keys[game_key].ksc1;
}

int is_key_down(int game_key)
{
	const struct kernel_device *kdev;
	int i, down, val, j;

	if (game_key < 0 || game_key >= GAME_NKEYS)
		return 0;

	down = 0;
	kdev = kernel_get_device();
	for (i = 0; !down && i < KERNEL_NPADS; i++) {
		for (j = 0; !down && j < NELEMS(s_buttons); j++) {
			if (s_buttons[j].key == game_key) {
				down |= kdev->key_down(
					KERNEL_KSC_PAD0_A + j +
					i * KERNEL_NPAD_BUTTONS);
			}
		}
	}

	/* See analog axis */
	if (!down && s_axis_dir[game_key].axis != -1) {
		for (i = 0; !down && i < KERNEL_NPADS; i++) {
			val = kdev->get_axis_value(i,
				s_axis_dir[game_key].axis);
			if (s_axis_dir[game_key].dir > 0)
				down = val > AXIS_LIMIT;
			else
				down = val < -AXIS_LIMIT;
		}
	}

	if (!down) {
		down |= kdev->key_down(s_keys[game_key].ksc1);
		down |= kdev->key_down(s_keys[game_key].ksc2);
		down |= kdev->key_down(s_keys[game_key].ksc3);
	}

	return down;
}

int is_first_pressed(int game_key)
{
	const struct kernel_device *kdev;
	int i, down, j;

	if (game_key < 0 || game_key >= GAME_NKEYS)
		return 0;

	down = 0;
	kdev = kernel_get_device();
	for (i = 0; !down && i < KERNEL_NPADS; i++) {
		for (j = 0; !down && j < NELEMS(s_buttons); j++) {
			if (s_buttons[j].key == game_key) {
				down |= kdev->key_first_pressed(
					KERNEL_KSC_PAD0_A + j +
					i * KERNEL_NPAD_BUTTONS);
			}
		}
	}

	if (!down) {
		down |= kdev->key_first_pressed(s_keys[game_key].ksc1);
		down |= kdev->key_first_pressed(s_keys[game_key].ksc2);
		down |= kdev->key_first_pressed(s_keys[game_key].ksc3);
	}

	return down;
}

void input_init(void)
{
}
