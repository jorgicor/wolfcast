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

#include "cfg/cfg.h"
#include "game_if.h"
#include "engine.h"
#include "bitmaps.h"
#include "sounds.h"
#include "input.h"
#include "menu.h"
#include "gamelib/bmp.h"
#include "gamelib/mixer.h"
#include "gamelib/state.h"
#include "gamelib/vfs.h"
#include "gamelib/ngetopt.h"
#include "kernel/kernel.h"
#include "cbase/kassert.h"
#include "SDL.h"

#ifndef STDLIB_H
#define STDLIB_H
#include <stdlib.h>
#endif

#ifndef STDIO_H
#define STDIO_H
#include <stdio.h>
#endif

#ifndef TIME_H
#define TIME_H
#include <time.h>
#endif

int main(int argc, char *argv[])
{
	static struct ngetopt_opt ops[] = {
		{ "editor", 0, 'e' },
		{ NULL, 0, 0 },
	};

	char c;
	struct ngetopt ngo;

	srand(time(0));

	/* Log to console always. */
	kassert_init();
	kassert_set_log_fun(kernel_get_device()->trace);

	ngetopt_init(&ngo, argc, argv, ops);
	do {
		c = ngetopt_next(&ngo);
		switch (c) {
		case '?':
			ktrace("unrecognized option %s", ngo.optarg);
			break;
		case ':':
			ktrace("the -%c option needs an argument",
				(char) ngo.optopt);
			break;
		}
	} while (c != -1);

	/*
	 * All modules are initialized here.
	 */

	bmp_draw_init();
	state_init();
	vfs_init();
	mixer_init();
	input_init();

	if (s_game_if.init) {
		/* Init game modules */
		s_game_if.init();
	}

	/*
	 * Main loop
	 */

	engine_run();

	/*
	 * All modules are de-initialized here.
	 */

	if (s_game_if.done) {
		/* End game modules */
		s_game_if.done();
	}

	bitmaps_done();
	sounds_done();

	return EXIT_SUCCESS;
}
