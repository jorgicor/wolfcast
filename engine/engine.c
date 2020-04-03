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
#include "cfg/cfg.h"
#include "readlin.h"
#include "load_st.h"
#include "input.h"
#include "engine/sounds.h"
#include "gamelib/vfs.h"
#include "gamelib/bmp.h"
#include "gamelib/mixer.h"
#include "kernel/kernel.h"
#include "cbase/kassert.h"
#include <ctype.h>
#include <string.h>
#include <locale.h>

/* If we go step by step. */
static int s_step_mode;

/* The real screen. We can draw directly after a begin_draw()
 * and before end_draw(). */
struct bmp s_screen; 

int s_screen_valid;

static void begin_draw(void)
{
	const struct kernel_device *d;
	struct kernel_canvas *kcanvas;

	d = kernel_get_device();
	kcanvas = d->get_canvas();

	s_screen.pixels = (unsigned char *) kcanvas->pixels;
	s_screen.w = kcanvas->w;
	s_screen.h = kcanvas->h;
	s_screen.pitch = kcanvas->pitch;

	reset_clip(s_screen.w, s_screen.h);
	s_screen_valid = 1;
}

static void end_draw(void)
{
	if (kassert_fails(s_screen_valid))
		return;

	s_screen_valid = 0;
}

static void on_frame(void *data)
{
	const struct kernel_device *kd;

	kd = kernel_get_device();

	begin_draw();

	/*
	 * The first time, set the first state.
	 */
	if (g_state == NULL) {
		switch_to_state(&load_st);
	}

	if (1)  {
		if (kd->key_first_pressed(KERNEL_KSC_P))
			s_step_mode = !s_step_mode;
	}

	if (!s_step_mode || kd->key_first_pressed(KERNEL_KSC_SPACE)) {
		update_state();
	}

	end_draw();
}

static void on_sound(void *data, unsigned char *samples, int nsamples)
{
	mixer_generate((short *) samples, nsamples, 1);
}

static const struct kernel_config kcfg = {
	.title = PACKAGE_NAME,
	.canvas_width = SCRW,
	.canvas_height = SCRH,
	.fullscreen = !PP_DEBUG,
	.maximized = !PP_DEBUG,
	.frames_per_second = FPS,
	.on_frame = on_frame,
	.on_sound = on_sound,
	.hint_scale_quality = 0,
	.hint_vsync = 1,
};

int engine_run(void)
{
	int ret;
	const struct kernel_device *d;

	d = kernel_get_device();
	ret = d->run(&kcfg, NULL);
	return ret;
}
