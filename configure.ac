#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

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

AC_INIT([wolfcast],
	[0.9],
	[jorge.giner@hotmail.com],
	wolfcast,
	[http://jorgicor.niobe.org/wolfcast])

AC_CONFIG_MACRO_DIR([m4])

AH_TEMPLATE([PP_ORG_NAME], [Name of organization])
AC_DEFINE([PP_ORG_NAME],["jorgicor"])
AH_TEMPLATE([PP_DEBUG], [Enable cheats])
AH_TEMPLATE([PP_DEMO], [Demo version])
AH_TEMPLATE([PP_USE_SDL_DATADIR],
	    [Use SDL to get the data folder instead of using DATADIR])
AC_ARG_ENABLE(debugmode,
	AS_HELP_STRING([--enable-debugmode], [compile debug version]))
AC_ARG_ENABLE(demoversion,
	AS_HELP_STRING([--enable-demoversion], [compile demo version]))
AC_ARG_ENABLE(x11,
	AS_HELP_STRING([--enable-x11], [link with X11 libraries]))
AC_ARG_ENABLE(sdl-datadir,
	AS_HELP_STRING([--enable-sdl-datadir],
		[use SDL to get the data folder instead of using datadir]))

AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror -Wportability foreign])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_SED

AX_CHECK_COMPILE_FLAG([-Wall], [WARN_CFLAGS="-Wall"], [], [])
AX_CHECK_COMPILE_FLAG([-Wdeclaration-after-statement],
	      [WARN_CFLAGS="$WARN_CFLAGS -Wdeclaration-after-statement"],
	      [], [])
AC_SUBST(WARN_CFLAGS)

WINDOWS_RC_VERSION=`echo $PACKAGE_VERSION | sed 's/\./,/g; s/0\(@<:@1-9@:>@\)/\1/g; s/$/,0,0/'`

case "$host" in
*-*-mingw* )
	AC_CHECK_TOOL(WINDRES, windres)
	;;
* )
	WINDRES=
	;;
esac
AM_CONDITIONAL(HAVE_WINDRES, test "$WINDRES" != "")

# Checks for libraries.

# Check for SDL
SDL_VERSION=2.0.4
AM_PATH_SDL2($SDL_VERSION,
	:,
	AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!])
)
LIBS="$LIBS $SDL_LIBS"
AC_SUBST(SDL_FLAGS)
AC_SUBST(ADD_CFLAGS)

# Checks for header files.
AC_CHECK_HEADERS([tgmath.h])

# Checks for typedefs, structures, and compiler characteristics.
if test "${enable_debugmode}" = yes; then
	AC_DEFINE([PP_DEBUG])
fi
if test "${enable_demoversion}" = yes; then
	AC_DEFINE([PP_DEMO])
fi
if test "${enable_sdl_datadir}" = yes; then
	AC_DEFINE([PP_USE_SDL_DATADIR])
fi
AM_CONDITIONAL(USE_SDL_DATADIR, test "${enable_sdl_datadir}" = yes)

# Checks for library functions.
AC_SEARCH_LIBS([pow],[m])
if test "${enable_x11}" = yes; then
	AC_SEARCH_LIBS([XOpenDisplay], [X11])
fi

AC_SUBST(WINDOWS_RC_VERSION)

AC_CONFIG_FILES([Makefile
		 engine/resource.rc])

AC_OUTPUT
