# Copyright (c) 2020 Jorge Giner Cordero
# 
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
# 
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

.rc.o:
	$(WINDRES) $< -o $@

AM_CFLAGS = $(WARN_CFLAGS)

dist_noinst_SCRIPTS = bootstrap

nobase_dist_doc_DATA = README NEWS COPYING AUTHORS

dist_pkgdata_DATA = gcdb/gamecontrollerdb.txt gcdb/README-gcdb

EXTRA_DIST = icon/icon.ico

# TODO: how to make this name the same as the package name in AC_INIT?
bin_PROGRAMS = app

app_CPPFLAGS = 

if !USE_SDL_DATADIR
app_CPPFLAGS += -DPP_DATADIR='"$(pkgdatadir)-data"' \
		     -DPP_PROG_DATADIR='"$(pkgdatadir)"'
endif

app_CFLAGS = $(SDL_CFLAGS) $(AM_CFLAGS)

if HAVE_WINDRES
RESOURCE = engine/resource.rc
else
RESOURCE = 
endif

app_SOURCES =	$(RESOURCE) \
		cfg/cfg.h \
		\
		cbase/cbase.h cbase/cbase.c \
		cbase/kassert.h cbase/kassert.c \
		cbase/floatint.h \
		\
		kernel/kernel.h kernel/kernel.c \
		kernel/kernel_snd.h kernel/kernel_snd.c \
		kernel/kernel_snd_sdl.h kernel/kernel_snd_sdl.c \
		kernel/kernel_snd_null.h kernel/kernel_snd_null.c \
		\
		gamelib/ngetopt.h gamelib/ngetopt.c \
		gamelib/bmp_load.h gamelib/bmp_load.c \
		gamelib/bmp.h gamelib/bmp_draw.c \
		gamelib/wav.h gamelib/wav_load.c \
		gamelib/vfs.h gamelib/vfs.c \
		gamelib/mixer.h gamelib/mixer.c \
		gamelib/state.h gamelib/state.c \
		\
		engine/bitmaps.c engine/bitmaps.h \
		engine/sounds.c engine/sounds.h \
		engine/main.c \
		engine/game_if.h \
		engine/engine.h engine/engine.c \
		engine/input.h engine/input.c \
		engine/load_st.h engine/load_st.c \
		engine/readlin.h engine/readlin.c \
		\
		game/game_if.c \
		game/raycast.h game/raycast.c \
		game/gplay_st.h game/gplay_st.c

