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

#include "gplay_st.h"
#include "raycast.h"
#include "engine/engine.h"
#include "kernel/kernel.h"
#include "cbase/kassert.h"

static void draw(void)
{
	raycast_draw();
}

static void update(void)
{
	const struct kernel_device *d;

	d = kernel_get_device();
	raycast_update();
	if (d->key_first_pressed(KERNEL_KSC_ESC)) {
		kernel_get_device()->stop();
		return;
	}
}

static void enter(const struct state *old_state)
{
	raycast_init();
}

const struct state gplay_st = {
	.enter = enter,
	.update = update,
	.draw = draw,
};
