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

#include "engine.h"
#include "load_st.h"
#include "bitmaps.h"
#include "sounds.h"
#include "input.h"
#include "game_if.h"
#include "gamelib/vfs.h"
#include "gamelib/wav.h"
#include "gamelib/bmp.h"
#include "gamelib/mixer.h"
#include "kernel/kernel.h"
#include "cbase/kassert.h"
#include "cfg/cfg.h"
#include <stdlib.h>
#include <string.h>

enum {
	LOAD_BITMAP_LIST,
	LOAD_NEXT_BITMAP,
	LOAD_SOUND_LIST,
	LOAD_NEXT_SOUND,
	LOAD_DATA_LOADED,
	LOAD_LAST_STATE
};

static int s_state;

enum {
	POINTW = 3
};

static void draw_point(int x, int y, int color)
{
	unsigned char *pb;
	unsigned int *pi;

	if (x < 0 || x > s_screen.w - POINTW)
		return;
	if (y < 0 || y > s_screen.h - POINTW)
		return;
	pb = s_screen.pixels + y * s_screen.pitch;
	pi = (unsigned int *) pb;
	pi += x;
	for (y = 0; y < POINTW; y++) {
		for (x = 0; x < POINTW; x++) {
			*(pi + x) = color;
		}
		pb = (unsigned char *) pi;
		pb += s_screen.pitch;
		pi = (unsigned int *) pb;
	}
}

static void draw_loading(void)
{
	int x, y, w, i;

	y = (s_screen.h - POINTW) / 2;
	w = (POINTW + 1) * LOAD_LAST_STATE;
	x = (s_screen.w - w) / 2;
	for (i = 0; i < LOAD_LAST_STATE; i++) {
		if (i >= s_state) {
			draw_point(x, y, 0x888888);
		} else {
			draw_point(x, y, 0xffffff);
		}
		x += POINTW + 1;
	}
}

static void draw(void)
{
	unsigned char *pb;
	int y;

	pb = s_screen.pixels;
	for (y = 0; y < s_screen.h; y++) {
		memset(pb, 0, s_screen.w * sizeof(unsigned int));
		pb += s_screen.pitch;
	}

	draw_loading();
}

static void load_next_bitmaps(void)
{
	int r;

	r = load_next_bitmap();
	if (r == E_BITMAPS_LOAD_EOF) {
		s_state++;
	} else if (r != E_BITMAPS_LOAD_OK) {
		ktrace("cannot load bitmap %s", cur_bitmap_name());
		exit(EXIT_FAILURE);
	}
}

static void load_next_sounds(void)
{
	int r;

	r = load_next_sound();
	if (r == E_SOUNDS_LOAD_EOF) {
		s_state++;
	} else if (r != E_SOUNDS_LOAD_OK) {
		ktrace("cannot load sound %s", cur_sound_name());
		exit(EXIT_FAILURE);
	}
}

/*
 * Bitmaps are loaded.
 * We can do other initialization depending on that.
 */
static void on_data_loaded(void)
{
	mixer_set_volume(50);
	s_state++;
}

static void update(void)
{
	switch (s_state) {
	case LOAD_BITMAP_LIST: load_bitmap_list(); s_state++; break;
	case LOAD_NEXT_BITMAP: load_next_bitmaps(); break;
	case LOAD_SOUND_LIST: load_sound_list(); s_state++; break;
	case LOAD_NEXT_SOUND: load_next_sounds(); break;
	case LOAD_DATA_LOADED: on_data_loaded(); break;
	case LOAD_LAST_STATE:
	       if (s_game_if.set_first_state) {
		       s_game_if.set_first_state();
	       }
	       if (g_state == &load_st) {
		       ktrace("game first state not set!");
		       exit(EXIT_FAILURE);
	       }
	       break;
	}
}

static void enter(const struct state *old_state)
{
	vfs_set_base_path(kernel_get_device()->get_data_path());
	ktrace("data path is %s", kernel_get_device()->get_data_path());
	s_state = 0;
}

const struct state load_st = {
	.enter = enter,
	.update = update,
	.draw = draw,
};
