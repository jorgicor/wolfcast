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

#include "raycast.h"
#include "engine/engine.h"
#include "engine/bitmaps.h"
#include "engine/input.h"
#include "gamelib/bmp.h"
#include "cbase/cbase.h"
#include "cbase/kassert.h"
#include "cbase/floatint.h"
#include "cfg/cfg.h"
#include <math.h>
#include <float.h>
#include <limits.h>
#include <string.h>

/* Tiles in map:
 *
 * Empty: 0000 0000
 *
 * Wall: 00ii iiii
 *        i: wall index in s_walls[]. Note that this starts at 1, so
 *           s_walls[0] is never addressed.
 *
 * Doors: 10ii iiii
 *        i: at map loading, contains index in s_walls[].
 *           When prepared in prepare_map_doors(), contains door index in
 *           s_doors[].
 *
 * Push walls: 01ii iiii
 *        i: index in s_walls[]. When prepared in prepare_map_pwalls(),
 *           contains push wall index in s_pwalls[].
 */

enum {
	BMP_WALL = 1,
	BMP_WALL2 = 2,
	BMP_CEIL = 3,
	BMP_FLOOR = 4,
	BMP_DOOR = 5,
	/* decimal bits */
	FS = 14,
	FONE = 1 << FS,
	DOT5 = 1 << (FS - 1),
	FOV = 60,
	FOV_D2 = FOV >> 1,
	RAYS = SCRW,
	GRIDS = 6,
	GRIDW = 1 << GRIDS,
	GRIDM = GRIDW - 1,
	NOT_GRIDM = ~GRIDM,
	/* FOV / RAYS = 0.46875 */
	/* ANGLE_INC = 120, */
	SLICEH = GRIDW,
	SCRHMID = SCRH / 2,
	/* (360/60) * RAYS angle increments */
	NANGLES = (360 / FOV) * RAYS,
	A90 = NANGLES / 4,
	A45 = A90 / 2,
	A180 = A90 * 2,
	A225 = A180 + A45,
	A270 = A90 * 3,
	A360 = NANGLES,
	AFOV = RAYS,
	AFOV_D2 = AFOV / 2,
	MAPW = 15,
	MAPH = 8,
	MAPSZ = MAPW * MAPH,
	/* 277 = (RAYS / 2) / tan( toradians( FOV / 2)  ) */
	DST_PLANE = 277,
	WALK_SPEED = 3,
	TURN_SPEED = 8,
	NWALLS = 64,
	NDOORS = 64,
	NPWALLS = 64,

	TILE_TYPE_MASK = 0xc0,
	EMPTY_TILE = 0,
	WALL_TILE = 0x00,
	DOOR_TILE = 0x80,
	PWALL_TILE = 0x40,

	WALL_INDEX = 0x3f,
	DOOR_DIR_H = 0,
	DOOR_DIR_V = 1,
	DOOR_INDEX = WALL_INDEX,
	PWALL_INDEX = WALL_INDEX,
};

/* Image buffer pixels. */
static unsigned int s_buf_pixels[SCRW * SCRH];

/* Image buffer bmp. */
static struct bmp s_buf_bmp = {
	.w = SCRW,
	.h = SCRH,
	.pitch = SCRW * 4,
	.palsz = 0,
	.pal = NULL,
	.use_key_color = 0,
	.key_color = 0,
	.pixels = (unsigned char *) s_buf_pixels
};

/* position and viewing angle of the player */
static int view_angle;
static float view_x, view_y;

static struct visplane {
	int xmin, xmax, ymin;
	short ys[SCRW];
} s_visplane;

static float s_zbuf[SCRW];

struct wall {
	struct bmp *pbmp;
};

static struct wall s_walls[NWALLS];

/* Door instance.
 * iwall: s_wall index. iwall + 1: index for sides.
 * xopen: how much is open, 0 is fully open, GRIDW fully closed.
 * dir: DOOR_DIR_H or DOOR_DIR_V.
 */
struct door {
	unsigned char iwall;
	unsigned char xopen;
	unsigned char dir;
};

static struct door s_doors[NDOORS];
static int s_ndoors;

/* Push wall instance.
 * iwall: s_wall index.
 * xopen: how much is open, 0 is fully open, GRIDW fully closed.
 * dir: DOOR_DIR_H or DOOR_DIR_V.
 */
struct pwall {
	unsigned char iwall;
	unsigned char xopen;
	unsigned char dir;
};

static struct pwall s_pwalls[NPWALLS];
static int s_npwalls;

static struct bmp *s_ceil_pbmp;
static struct bmp *s_floor_pbmp;

static unsigned char s_map[] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0x82, 1,
	1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0x82, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 0, 0, 0, 0, 0, 0, 0x41, 0, 0, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

static float sintab[NANGLES];
static float isintab[NANGLES];
static float tantab[NANGLES];
static float itantab[NANGLES];

enum {
	STATE_IDLE,
	STATE_GIRO,
	STATE_WALK,
};

static int state = STATE_IDLE;

enum {
	GIRO_STEP = NANGLES / 16,
	WALK_STEPS = 2, // 4
	WALK_STEP = GRIDW / 4, //WALK_STEPS;
};

static int giro_step;
static int walk_step;
static int walk_steps;
static int s_changed;

static int s_flat_ceiling = 0;
static int s_flat_floor = 0;
static unsigned int s_ceiling_color = 0xff00;
static unsigned int s_floor_color = 0xff;

static float s_diaglen;

#define PI 0x1.921fb54442d18p+1 

#define toradians(degrees) ((degrees) * PI / 180.0)

static void draw(void);

static int fixangle(int a)
{
	if (a < 0) {
		a += A360; 
	} else if (a >= A360) {
		a -= A360;
	}

	return a;
}

// fill the first quadrant of sintab [0-90]
static void start_tables(void)
{
	int i;
	double step;

	step = PI / A180;
	for (i = 0; i < A90; i++) {
		sintab[i] = sin(step * i);
	}

	sintab[A90] = 1;
}

static void gen_tables(void)
{
	int i, tmp;

	start_tables();

	/* sin quadrant [91-180] */
	for (i = 0; i < A90; i++) {
		sintab[A180 - i] = sintab[i];
	}

	/* sin quadrant [181-359] */
	for (i = 1; i < A180; i++) {
		sintab[A360 - i] = -sintab[i];
	}

	/* 1 / sin */
	for (i = 0; i < A360; i++) {
		isintab[i] = 1 / sintab[i];
	}

	/* tangent and 1 / tan */
	for (i = 0; i < NANGLES; i++) {
		tmp = fixangle(A90 - i);
		tantab[i] = sintab[i] * isintab[tmp];
		itantab[i] = sintab[tmp] * isintab[i];
	}

	s_diaglen = sqrt(GRIDW*GRIDW*2);
}

static int is_wall(int wtype)
{
	return wtype != 0 && (wtype & TILE_TYPE_MASK) == WALL_TILE;
}

static int wall_index(int wtype)
{
	return (wtype & WALL_INDEX) & (NWALLS - 1);
}

static int is_door(int wtype)
{
	return (wtype & TILE_TYPE_MASK) == DOOR_TILE;
}

/* Returns the index into s_doors. */
static int door_index(int wtype)
{
	return (wtype & DOOR_INDEX) & (NDOORS - 1);
}

/* is_door must be checked before. */
static int is_hdoor(int wtype)
{
	return s_doors[door_index(wtype)].dir == DOOR_DIR_H;
}

/* is_door must be checked before. */
static int is_vdoor(int wtype)
{
	return !is_hdoor(wtype);
}

static int is_pwall(int wtype)
{
	return (wtype & TILE_TYPE_MASK) == PWALL_TILE;
}

/* Returns the index in s_pwalls. */
static int pwall_index(int wtype)
{
	return (wtype & PWALL_INDEX) & (NPWALLS - 1);
}

/* is_pwall must be checked before. */
static int is_hpwall(int wtype)
{
	return s_pwalls[pwall_index(wtype)].dir == DOOR_DIR_H;
}

/* is_pwall must be checked before. */
static int is_vpwall(int wtype)
{
	return !is_hpwall(wtype);
}

/* tx, ty are in tile coordinates. */
static int wall_at_tile(int tx, int ty)
{
	if (tx < 0 || tx >= MAPW || ty < 0 || ty >= MAPH)
		return 1;

	return s_map[ty * MAPW + tx]; 
}

/* x, y are in world coordinates (ie, each GRIDW units 1 tile). */
static int wall_at(int x, int y)
{
	return wall_at_tile(x >> GRIDS, y >> GRIDS);
}

static void load_floors(void)
{
	s_ceil_pbmp = get_bitmap(BMP_CEIL);
	s_floor_pbmp = get_bitmap(BMP_FLOOR);
}

static void load_walls(void)
{
	memset(s_walls, 0, sizeof(s_walls));
	s_walls[0].pbmp = get_bitmap(BMP_WALL);
	s_walls[1].pbmp = get_bitmap(BMP_WALL2);
	s_walls[2].pbmp = get_bitmap(BMP_DOOR);
	s_walls[3].pbmp = get_bitmap(BMP_WALL);
}

/* Links the s_push_walls with the tiles.
 * Puts the push direction for horizontal or vertical, by
 * checking surrounding tiles.
 */
static void prepare_map_pwalls(void)
{
	int i, x, y, iwall;
	int too_many;

	s_npwalls = 0;
	too_many = 0;
	for (y = 0, i = 0; y < MAPH; y++) {
		for (x = 0; x < MAPW; x++, i++) {
			if (is_pwall(s_map[i])) {
				iwall = wall_index(s_map[i]);
				s_map[i] = PWALL_TILE;
				/* Link with push wall in s_pwalls */
				if (s_npwalls < NPWALLS) {
					s_pwalls[s_npwalls].iwall = iwall;
					s_pwalls[s_npwalls].xopen = 0;
					if (is_wall(wall_at_tile(x, y - 1)) &&
					    is_wall(wall_at_tile(x, y + 1)))
					{
						s_pwalls[s_npwalls].dir =
							DOOR_DIR_V;
					}
					s_map[i] |= s_npwalls;
					s_npwalls++;
				} else {
					/* Too many...  */
					too_many = 1;
					s_map[i] = EMPTY_TILE;
				}
			}
		}
	}

	if (too_many) {
		ktrace("Too many push walls in map.");
	}
}

/* Links the s_doors doors with the tiles.
 * Puts the door direction for horizontal or vertical doors, by
 * checking surrounding tiles.
 */
static void prepare_map_doors(void)
{
	int i, x, y, iwall;
	int too_many;

	s_ndoors = 0;
	too_many = 0;
	for (y = 0, i = 0; y < MAPH; y++) {
		for (x = 0; x < MAPW; x++, i++) {
			if (is_door(s_map[i])) {
				iwall = wall_index(s_map[i]);
				s_map[i] = DOOR_TILE;
				/* Link with door in s_doors */
				if (s_ndoors < NDOORS) {
					s_doors[s_ndoors].iwall = iwall;
					s_doors[s_ndoors].xopen = GRIDW;
					if (is_wall(wall_at_tile(x, y - 1)) &&
					    is_wall(wall_at_tile(x, y + 1)))
					{
						s_doors[s_ndoors].dir =
							DOOR_DIR_V;
					}
					s_map[i] |= s_ndoors;
					s_ndoors++;
				} else {
					/* Too many doors...  */
					too_many = 1;
					s_map[i] = EMPTY_TILE;
				}
			}
		}
	}

	if (too_many) {
		ktrace("Too many doors in map.");
	}
}

static void reset(void)
{
	load_floors();
	load_walls();
	prepare_map_doors();
	prepare_map_pwalls();
	s_changed = 1;
	view_angle = 0;
	view_x = GRIDW * 4 + (GRIDW >> 1);
	view_y = GRIDW * 3 + (GRIDW >> 1);
}

void init(void)
{
	gen_tables();
	reset();
};

static void view_up(void)
{
	walk_steps = WALK_STEPS;
	walk_step = WALK_STEP;
	state = STATE_WALK;
}

static void view_down(void)
{
	walk_steps = WALK_STEPS;
	walk_step = -WALK_STEP;
	state = STATE_WALK;
}

static void view_left(void)
{
	giro_step = GIRO_STEP;
	state = STATE_GIRO;
}

static void view_right(void)
{
	giro_step = -GIRO_STEP;
	state = STATE_GIRO;
}

static void update_doors(void)
{
	int i;

	for (i = 0; i < s_ndoors; i++) {
		s_changed = 1;
		if (s_doors[i].xopen == 0) {
			s_doors[i].xopen = GRIDW;
		} else {
			s_doors[i].xopen--;
		}
	}
}

static void update_pwalls(void)
{
	int i;

	for (i = 0; i < s_npwalls; i++) {
		s_changed = 1;
		s_pwalls[i].xopen = (s_pwalls[i].xopen + 1) % (GRIDW + 1);
	}
}

void raycast_update(void)
{
	if (state == STATE_GIRO) {
		s_changed = 1;
		view_angle = view_angle + giro_step;
		if (view_angle < 0) {
			view_angle += A360;
		} else if (view_angle >= A360) {
			view_angle -= A360;
		}

		// System.out.println(view_angle);
		if (view_angle == 0 || view_angle == A90 ||
		    view_angle == A180 || view_angle == A270)
	       	{
			state = STATE_IDLE;
		}
	} else if (state == STATE_WALK) {
		s_changed = 1;
		if (view_angle == 0) {
			view_x += walk_step;
		} else if (view_angle == A90) {
			view_y -= walk_step;
		} else if (view_angle == A180) {
			view_x -= walk_step;
		} else if (view_angle == A270) {
			view_y += walk_step;
		}

		walk_steps--;
		if (walk_steps == 0) {
			state = STATE_IDLE;
		}
	} else {
		if (is_key_down(KLEFT)) {
			s_changed = 1;
			view_angle += TURN_SPEED;
			if (view_angle >= A360) {
				view_angle -= A360;
			}
			// view_left();
		} else if (is_key_down(KRIGHT)) {
			s_changed = 1;
			view_angle -= TURN_SPEED;
			if (view_angle < 0) {
				view_angle += A360;
			}
			// view_right();
		}
		
		if (is_key_down(KUP)) {
			s_changed = 1;
			view_x += sintab[fixangle(A90 + view_angle)] *
			       	  WALK_SPEED;
			view_y -= sintab[view_angle] * WALK_SPEED;
			// view_up();
		} else if (is_key_down(KDOWN)) {
			s_changed = 1;
			view_x += sintab[fixangle(A90 + view_angle)] *
			       	  -WALK_SPEED;
			view_y -= sintab[view_angle] * -WALK_SPEED;
			// view_down();
		}
	}

	update_doors();
	update_pwalls();

	if (s_changed) {
		draw();
		s_changed = 0;
	}
}

void raycast_draw()
{
	draw_bmp_kct(&s_buf_bmp, 0, 0, &s_screen, NULL, 0, 0);
}

enum {
	COLUMNH = 64,
	COLUMNH_F = 64 << 8,
	COLUMN_FS = 16,
};

static void reset_visplane(void)
{
	int x;

	for (x = 0; x < SCRW; x++) {
		s_visplane.ys[x] = SCRH;
	}
	s_visplane.ymin = SCRH;
	s_visplane.xmin = SCRW;
	s_visplane.xmax = 0;
}

/* x is screen column, y where floor starts... */
static void add_visplane_column(int x, int y)
{
	s_visplane.ys[x] = y;
	if (y < s_visplane.ymin) {
		s_visplane.ymin = y;
	}
}

/* Sets the xminx and xmax of the visplane. */
static void set_visplane_bbox(void)
{
	int x;

	for (x = 0; x < SCRW; x++) {
		if (s_visplane.ys[x] < SCRH) {
			s_visplane.xmin = x;
			break;
		}
	}

	for (x = SCRW - 1; x >= s_visplane.xmin; x--) {
		if (s_visplane.ys[x] < SCRH) {
			s_visplane.xmax = x + 1;
			break;
		}
	}
}

static void draw_visplane_bbox(void)
{
	struct bmp *bp;

	bp = &s_buf_bmp;
	set_draw_color(0xff0000);
	draw_line(bp, s_visplane.xmin, s_visplane.ymin,
		       	s_visplane.xmin, SCRH);
	draw_line(bp, s_visplane.xmax - 1, s_visplane.ymin,
		  s_visplane.xmax - 1, SCRH);
	draw_line(bp, s_visplane.xmin, s_visplane.ymin,
		       	s_visplane.xmax -1, s_visplane.ymin);
}

/* Draw the column 'col [0-63] of bitmap 'ibmp at screen column 'x.
 * Actually 'col is a bitmap row index, since we have the wall bitmaps
 * rotated 90 degrees.
 * 'wh is the desired height to paint the column, so it will be scaled as
 * needed.
 * Updates the floor-ceiling visplane.
 */
static void draw_wall_column(struct bmp *sbmp, int col, int wh, int x)
{
	int y, py, xinc;
	struct bmp *dbmp;
	unsigned int *dpix;
	int dpitch;

	if (sbmp == NULL) {
		sbmp = s_walls[0].pbmp;
	}

	dbmp = &s_buf_bmp;
	dpix = ((unsigned int *) dbmp->pixels) + x;
	dpitch = dbmp->pitch >> 2;
       
	if (wh & 1) {
		wh--;
	}

	if (wh == 0) {
		return;
	}

	xinc = (COLUMNH << COLUMN_FS) / wh;
	if (wh <= SCRH) {
		y = (SCRH - wh) >> 1;
		add_visplane_column(x, SCRH - y);
		x = 0;
	} else {
		y = 0;
		add_visplane_column(x, SCRH - y);
		x = ((wh - SCRH) >> 1) * xinc;
		wh = SCRH;
	}

#if 0
	for (x = 0; x < SCRH; x++) {
		*dpix = 0;
		dpix += dpitch;
	}
	return;
#endif

	if (s_flat_ceiling) {
		py = 0;
		while (py < y) {
			*dpix = s_ceiling_color;
			dpix += dpitch;
			py++;
		}
	} else {
		py = y;
		dpix += dpitch * y;
	}
	py += wh;

	if (sbmp == NULL) {
		while (wh > 0) {
			*dpix = 0;
			dpix += dpitch;
			wh--;
		}
	} else {
		x = ((col * COLUMNH) << COLUMN_FS) + x;
		while (wh > 0) {
			*dpix = sbmp->pal[sbmp->pixels[x >> COLUMN_FS]];
			dpix += dpitch;
			x += xinc;
			wh--;
		}
	}

	if (s_flat_floor) {
		while (py < SCRH) {
			*dpix = s_floor_color;
			dpix += dpitch;
			py++;
		}
	}
}

/* Returns 1 if hit and ax, ay will be the point hit.
 * Returns 0 if not hit and ax, ay will be left untouched.
 * a is angle, ax, ay point on tile side hit.
 */
static int uldwall_hhit(int a, float *ax, float *ay, int *tex_x)
{
	int alfa, beta, tx;
	float d;

	if (a <= A45 || a >= A180)
		return 0;

	alfa = fixangle(A180 - a);
	beta = fixangle(a - A45);
	tx = float_to_int(*ax) & NOT_GRIDM;
	d = (*ax - tx) * sintab[alfa] * isintab[beta];
	if (d < 0 || d >= s_diaglen) {
		return 0;
	}
	*tex_x = float_to_int(d * GRIDW / s_diaglen);
	*ax = tx + sintab[fixangle(A90 + A45)] * d;
	*ay -= sintab[A45] * d;
	return 1;
}

static int uldwall_vhit(int a, float *ax, float *ay, int *tex_x)
{
	int alfa, beta, ty;
	float d;

	if (a <= A45 || a >= A225)
		return 0;

	alfa = fixangle(a - A90);
	beta = fixangle(A225 - a);
	ty = float_to_int(*ay) & NOT_GRIDM;
	d = (*ay - ty) * sintab[alfa] * isintab[beta];
	if (d < 0 || d >= s_diaglen) {
		return 0;
	}
	*tex_x = float_to_int((s_diaglen - d) * GRIDW / s_diaglen);
	*ax -= sintab[A45] * d;
	*ay = ty + sintab[fixangle(A90 + A45)] * d;
	return 1;
}

/* Draws scan at (x=[ax, bx[, y) */
static void draw_floor_scan(int ax, int bx, int y, float xp, float yp,
	       		     float dx, float dy)
{
	int ui, vi, ti, dfloor, dceil;
	struct bmp *dbmp;
	unsigned int *fpix, *cpix;

	dbmp = &s_buf_bmp;
	fpix = cpix = NULL;

	dfloor = !s_flat_floor && s_floor_pbmp;
	dceil = !s_flat_ceiling && s_ceil_pbmp;

	if (dfloor) {
		fpix = (unsigned int *) (dbmp->pixels + y * dbmp->pitch) + ax;
	}

	if (dceil) {
		cpix = (unsigned int *) (dbmp->pixels +
			       	         (SCRH - y - 1) * dbmp->pitch) + ax;
	}

	if (fpix == NULL && cpix == NULL) {
		return;
	}

	while (ax < bx) {
		ui = float_to_int(xp) & GRIDM; 
		vi = float_to_int(yp) & GRIDM; 
		ti = (vi << GRIDS) + ui;
		if (fpix) {
			*fpix++ = s_floor_pbmp->pal[s_floor_pbmp->pixels[ti]];
		}
		if (cpix) {
			*cpix++ = s_ceil_pbmp->pal[s_ceil_pbmp->pixels[ti]];
		}
		xp += dx;
		yp += dy;
		ax++;
	}

#if 0
	// .9 fixed point
	ui = (int) (xp * 512); 
	vi = (int) (yp * 512);
	deltax = (int) (dx * 512);
	deltay = (int) (dy * 512);
	for (x = 0; x < RAYS; x++) {
		ti = ((vi>>3) & 0x0fc0) | ((ui>>9) & GRIDM);
		*dpix++ = floor_bmp->pal[floor_bmp->pixels[ti]];
		*dpix2++ = ceil_bmp->pal[ceil_bmp->pixels[ti]];
		ui += deltax;
		vi += deltay;
	}
#endif
}

/* y where floor starts on screen */
static void draw_floor_scans(int y, float xp, float yp, float dx, float dy)
{
	int a, b;

	a = -1;
	for (b = s_visplane.xmin; b < s_visplane.xmax; b++) {
		if (a == -1) {
		       if (s_visplane.ys[b] <= y) {
			       a = b;
		       }
		} else if (s_visplane.ys[b] > y) {
			draw_floor_scan(a, b, y, xp + dx*a, yp + dy*a, dx, dy);
			a = -1;
		}
	}

	if (a != -1) {
		draw_floor_scan(a, b, y, xp + dx * a, yp + dy * a, dx, dy);
	}
}

/* y is where the floor line starts on screen, that is, it is in range
 * [SCRHMID + 1, SCRH[.
 */
static void draw_floor_line(int y)
{
	int a, b;
	float xp, yp, d, dp, dx, dy;
	float xp2, yp2;
	enum { PLAYERH = SLICEH >> 1 };

	a = view_angle + AFOV_D2;
	b = iabs(a - view_angle);
	a = fixangle(a);

	/* perpendicular distance to point on floor */
	dp = (float) DST_PLANE * PLAYERH / (y - SCRHMID);

	/* distance to point on floor (projected on floor) */
	d = dp * isintab[fixangle(A90 + b)];

	/* position on floor */
	xp = view_x + d * sintab[fixangle(A90 + a)];
	yp = view_y - d * sintab[a];

	/* position on floor of opposite side of the view */
	xp2 = view_x + d * sintab[fixangle(A90 + a - AFOV)];
	yp2 = view_y - d * sintab[fixangle(a - AFOV)];

	dx = (xp2 - xp) / RAYS;
	dy = (yp2 - yp) / RAYS;

#if 0
	ktrace("a %f", sqrt((xp2 - xp) * (xp2 - xp) +
		 	    (yp2 - yp) * (yp2 - yp)));

	ktrace("xp2 yp2 %f %f", xp2, yp2);

	/* vector perpendicular to view vector */
	dx = sintab[view_angle];
	dy = -sintab[fixangle(A90 + view_angle)];
	k = RAYS * dp / DST_PLANE;
	ktrace("k %f", k);
	dx *= k;
	dy *= k;
	xp2 = xp + dx;
	yp2 = yp - dy;
	ktrace("xp2 yp2 %f %f", xp2, yp2);

	ktrace("b %f", sqrt((xp2 - xp) * (xp2 - xp) +
		 	    (yp2 - yp) * (yp2 - yp)));

	dx /= RAYS;
	dy /= -RAYS;
#endif

	draw_floor_scans(y, xp, yp, dx, dy);
}

static void draw_floor(void)
{
	int y;

	for (y = s_visplane.ymin; y < SCRH; y++) {
		draw_floor_line(y);
	}
}

/* Returns the column hit or -1.
 * ax and ay will contain the point hit if column >= 0.
 * is_door and is_hdoor must be checked before.
 */
static int hit_hdoor(int wtype, float xinc, float yinc, float *ax, float *ay,
		     struct bmp **ppbmp)
{
	float ix;
	int idoor, tx;
	struct door *pdoor;

	idoor = door_index(wtype);
	pdoor = &s_doors[idoor];
	if (pdoor->xopen == 0) {
		/* Fully open. */
		return -1;
	}

	ix = *ax;
	tx = float_to_int(ix) & NOT_GRIDM;
	ix += xinc / 2;
	if ((float_to_int(ix) & NOT_GRIDM) != tx) {
		/* Not in the same tile, no hit. */
		return -1;
	}
	
	tx = float_to_int(ix) & GRIDM;
	if (pdoor->xopen > tx) {
		/* We hit the visible zone of the door... */
		*ax = ix;
		*ay += yinc / 2;
		*ppbmp = s_walls[wall_index(pdoor->iwall)].pbmp;
		return GRIDW - pdoor->xopen + tx;
	}

	return -1;
}

/* Returns the column hit or -1.
 * ax and ay will contain the point hit if column >= 0.
 * is_door and is_vdoor must be checked before.
 */
static int hit_vdoor(int wtype, float xinc, float yinc, float *ax, float *ay,
		     struct bmp **ppbmp)
{
	float iy;
	int idoor, ty;
	struct door *pdoor;

	idoor = door_index(wtype);
	pdoor = &s_doors[idoor];
	if (pdoor->xopen == 0) {
		/* Fully open. */
		return -1;
	}

	iy = *ay;
	ty = float_to_int(iy) & NOT_GRIDM;
	iy += yinc / 2;
	if ((float_to_int(iy) & NOT_GRIDM) != ty) {
		/* Not in the same tile, no hit. */
		return -1;
	}
	
	ty = float_to_int(iy) & GRIDM;
	if (pdoor->xopen > ty) {
		/* We hit the visible zone of the door... */
		*ax += xinc / 2;
		*ay = iy;
		*ppbmp = s_walls[wall_index(pdoor->iwall)].pbmp;
		return GRIDW - pdoor->xopen + ty;
	}

	return -1;
}

/* Returns the column hit or -1.
 * ax and ay will contain the point hit if column >= 0.
 * is_pwall and is_hwall must be checked before.
 */
static int hit_hpwall(int wtype, float xinc, float yinc, float *ax, float *ay,
		      struct bmp **ppbmp)
{
	float ix;
	int ipwall, tx;
	struct pwall *pwall;

	ipwall = pwall_index(wtype);
	pwall = &s_pwalls[ipwall];
	if (pwall->xopen == GRIDW) {
		/* Fully open. */
		return -1;
	}

	ix = *ax;
	tx = float_to_int(ix) & NOT_GRIDM;
	ix += (xinc / GRIDW) * pwall->xopen;
	if ((float_to_int(ix) & NOT_GRIDM) != tx) {
		/* Not in the same tile, no hit. */
		return -1;
	}
	
	tx = float_to_int(ix) & GRIDM;
	*ax = ix;
	*ay += (yinc / GRIDW) * pwall->xopen;
	*ppbmp = s_walls[wall_index(pwall->iwall)].pbmp;
	return tx;
}

/* Returns the column hit or -1.
 * ax and ay will contain the point hit if column >= 0.
 * is_pwall and is_vpwall must be checked before.
 */
static int hit_vpwall(int wtype, float xinc, float yinc, float *ax, float *ay,
		      struct bmp **ppbmp)
{
	float iy;
	int ipwall, ty;
	struct pwall *pwall;

	ipwall = pwall_index(wtype);
	pwall = &s_pwalls[ipwall];
	if (pwall->xopen == GRIDW) {
		/* Fully open. */
		return -1;
	}

	iy = *ay;
	ty = float_to_int(iy) & NOT_GRIDM;
	iy += (yinc / GRIDW) * pwall->xopen;
	if ((float_to_int(iy) & NOT_GRIDM) != ty) {
		/* Not in the same tile, no hit. */
		return -1;
	}
	
	ty = float_to_int(iy) & GRIDM;
	*ax += (xinc / GRIDW) * pwall->xopen;
	*ay = iy;
	*ppbmp = s_walls[wall_index(pwall->iwall)].pbmp;
	return ty;
}

static struct bmp *get_hwall_bmp(int a, int wtype, int px, int py)
{
	int wdtype, iwall;

	if (a > 0 && a < A180) {
		py++;
	} else if (a > A180 && a < A360) {
		py--;
	}

	wdtype = wall_at(px, py);
	if (is_door(wdtype) && is_vdoor(wdtype)) {
		iwall = wall_index(s_doors[door_index(wdtype)].iwall + 1);
	} else {
		iwall = wall_index(wtype);
	}

	return s_walls[iwall].pbmp;
}

/* Cast a ray of at angle 'a and hit an horizontal wall.
 * 'b is the angle between 'a and view_angle, in absolute value.
 * Returns the distance to the hit point or FLT_MAX.
 * If not FLT_MAX, and 'column will be column of the wall hit.
 */
static float hit_hwall(int a, int b, int *column, struct bmp **ppbmp)
{
	int iter, wtype, px, py;
	float d, ax, ay, xinc, yinc;

	iter = 0;
	if (a == 0 || a == A180) {
		d = FLT_MAX;
		ax = 0;
		ay = 0;
	} else { 
		if (a > 0 && a < A180)  {
			// facing up
			ay = (float_to_int(view_y) & NOT_GRIDM) - 1;
			yinc = -GRIDW;
		} else {
			// ray facing down
			ay = (float_to_int(view_y) & NOT_GRIDM) + GRIDW;
			yinc = GRIDW;
		}

		if (a == A90 || a == A270) {
			ax = view_x;
			xinc = 0;
		} else {
			ax = view_x + (view_y - ay) * itantab[a];	
			xinc = yinc * -itantab[a];
		}

		for (;;) {
			iter++;
			px = float_to_int(ax);
			py = float_to_int(ay);
			wtype = wall_at(px, py);
			if (is_door(wtype) && is_hdoor(wtype)) {
				*column = hit_hdoor(wtype, xinc, yinc,
					       	    &ax, &ay, ppbmp);
				if (*column >= 0) {
					break;
				}
			} else if (is_pwall(wtype) && is_hpwall(wtype)) {
				*column = hit_hpwall(wtype, xinc, yinc,
					       	     &ax, &ay, ppbmp);
				if (*column >= 0) {
					break;
				}
			}

			if (is_wall(wtype)) {
				*column = float_to_int(ax) & GRIDM;
				*ppbmp = get_hwall_bmp(a, wtype, px, py);
				// *ppbmp = s_walls[wall_index(wtype)].pbmp;
				break;
			}

			ax += xinc;
			ay += yinc;
		}

		kassert(iter <= MAPW);

		d = (view_y - ay) * isintab[a] * sintab[fixangle(A90 + b)];
		if (d < 0) {
			d = -d;
		}
	}

	return d;
}

static struct bmp *get_vwall_bmp(int a, int wtype, int px, int py)
{
	int wdtype, iwall;

	if (a < A90 || a > A270) {
		px--;
	} else if (a > A90 && a < A270) {
		px++;
	}

	wdtype = wall_at(px, py);
	if (is_door(wdtype) && is_hdoor(wdtype)) {
		iwall = wall_index(s_doors[door_index(wdtype)].iwall + 1);
	} else {
		iwall = wall_index(wtype);
	}

	return s_walls[iwall].pbmp;
}

/* Cast a ray of at angle 'a and hit an vertical wall.
 * 'b is the angle between 'a and view_angle, in absolute value.
 * Returns the distance to the hit point or FLT_MAX.
 * If not FLT_MAX, and 'column will be column of the wall hit.
 */
static float hit_vwall(int a, int b, int *column, struct bmp **ppbmp)
{
	int iter, wtype, px, py;
	float d, ax, ay, xinc, yinc;

	iter = 0;
	if (a == A90 || a == A270) {
		d = FLT_MAX;
		ax = 0;
		ay = 0;
	} else {
		if (a > A90 && a < A270) {
			// facing left
			ax = (float_to_int(view_x) & NOT_GRIDM) - 1;
			xinc = -GRIDW;
		} else {
			// facing right
			ax = (float_to_int(view_x) & NOT_GRIDM) + GRIDW;
			xinc = GRIDW;
		}

		if (a == 0 || a == A180) {
			ay = view_y;
			yinc = 0;
		} else {
			ay = view_y + (view_x - ax) * tantab[a]; 
			yinc = xinc * -tantab[a]; 
		}

		for (;;) {
			iter++;
			px = float_to_int(ax);
			py = float_to_int(ay);
			wtype = wall_at(px, py);
			if (is_door(wtype) && is_vdoor(wtype)) {
				*column = hit_vdoor(wtype, xinc, yinc,
					       	    &ax, &ay, ppbmp);
				if (*column >= 0) {
					break;
				}
			} else if (is_pwall(wtype) && is_vpwall(wtype)) {
				*column = hit_vpwall(wtype, xinc, yinc,
					       	     &ax, &ay, ppbmp);
				if (*column >= 0) {
					break;
				}
			}

			if (is_wall(wtype)) {
				*column = py & GRIDM;
				*ppbmp = get_vwall_bmp(a, wtype, px, py);
				// *ppbmp = s_walls[wall_index(wtype)].pbmp;
				break;
			}

			ax += xinc;
			ay += yinc;
		}

		kassert(iter <= MAPW);

		d = (view_x - ax) * isintab[fixangle(A90 + a)] *
		     sintab[fixangle(A90 + b)];

		if (d < 0) {
			d = -d;
		}

	}

	return d;
}

static void draw_walls(void)
{
	int angle, wh, x, a, b; 
	int col, vcol;
	float d, vd;
	struct bmp *pbmp, *pvbmp;

	pbmp = pvbmp = NULL;
	angle = view_angle + AFOV_D2;
	col = vcol = 0;
	for (x = 0; x < RAYS; x++, angle--) {
		a = angle;
		b = iabs(a - view_angle);
		a = fixangle(a);
		d = hit_hwall(a, b, &col, &pbmp);
		vd = hit_vwall(a, b, &vcol, &pvbmp);
		if (vd <= d) {
			col = vcol;
			d = vd;
			pbmp = pvbmp;
		}
		
		s_zbuf[x] = d;
		if (d > 0) {
			wh = float_to_int(SLICEH * DST_PLANE / d);
			draw_wall_column(pbmp, col, wh, x);
		}
	}
}

static void draw(void)
{
	reset_visplane();
	draw_walls();
	set_visplane_bbox();
	draw_floor();
	// draw_visplane_bbox();
}

#if 0
static void drawSprite(int x, int y, byte[] spr) {
	// traslacion
	x -= view_x;
	y -= view_y;

	// rotamos el centro del sprite para tenerlo en coordenadas de la vista
	int a = fixangle(A90 + view_angle);
	int b = fixangle(view_angle);
	long_t xp = x * sin[a] - y * sin[b];

	// xp es la distancia del centro al plano (con su perspectiva ya corregida)
	// if (xp < DST_VPLANE)
	if (xp <= 0)
		return;

	// yp es la distancia del centro al centro de vision
	long_t yp = x * sin[b] + y * sin[a]; 

	// a es la altura de la columna para todo el sprite, ya que todo el
	// sprite mira directamente a la vista
	// tambien es su anchura, puesto que es cuadrado
	a = (int) ((((WALLMUL_F << 1) / xp) + 1) >> 1);

	// proyectamos yp
	b = SCRMID - (a >> 1) + (int) ((((((yp * DST_VPLANE) >> FS) << 1) / xp) + 1) >> 1);

	// System.out.println(a + "," + b);

	_drawSpr(a, b);
 }

static void _drawSpr(int wh, int x0) {
	if (x0 >= SCRW) return;
	if (x0 + wh < 0) return;

	int ws = COLUMNH_F / wh;	// wall step

	int xa = x0;		// first column on screen
	int xb = x0 + wh;	// last column on screen
	int t0 = 0;
	if (xa < 0) {
		xa = 0;
		t0 = -ws*x0;	// first column on texture
	}

	if (xb >= SCRW) {
		xb = SCRW - 1;		
	}

	if (wh > SCRH)
		wh = SCRH;

	for ( ; xa < xb; xa++) {
		// System.out.println("t0 = " + (t0 >> 8));
		drawScan(wall, t0 >> 8, buf, wh, xa); 
		t0 += ws;
	}
}
#endif

void raycast_init(void)
{
	init();
}
