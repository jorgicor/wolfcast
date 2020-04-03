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

#include "bitmaps.h"
#include "readlin.h"
#include "cbase/cbase.h"
#include "gamelib/vfs.h"
#include "gamelib/bmp.h"
#include "cbase/kassert.h"

#ifndef STDLIB_H
#define STDLIB_H
#include <stdlib.h>
#endif

#include <string.h>

struct bmp_file {
	char name[9];
	int sloti;
	int use_key_color;
	unsigned int key_color;
};

struct bmp_slot {
	struct bmp *pbmp;
	int managed;
};

static struct bmp_file s_bitmap_files[NBITMAPS];

/* slot 0 is like NULL */
static struct bmp_slot s_bitmap_slots[NBITMAPS];

static int s_load_index;
static int s_nbitmap_files;

/* Sets the bitmap at slot i to pbmp.
 * Does nothing if that slot contains a managed bitmap.
 */
void set_bitmap(int i, struct bmp *pbmp)
{
	if (kassert_fails(i > 0 && i < NBITMAPS))
		return;
	if (kassert_fails(!s_bitmap_slots[i].managed))
		return;
	s_bitmap_slots[i].pbmp = pbmp;
}

struct bmp *get_bitmap(int i)
{
	if (i < 0 || i >= NBITMAPS)
		i = 0;
	return s_bitmap_slots[i].pbmp;
}

int load_bitmap_list(void)
{
	int i;
	FILE *fp;
	char line[READLIN_LINESZ];

	fp = open_file("data/bitmaps.txt", NULL);
	if (fp == NULL)
		return 0;

	i = 0;
	while (i < NBITMAPS && readlin(fp, line) != -1) {
		if (tokscanf(line, "isii", &s_bitmap_files[i].sloti,
			     s_bitmap_files[i].name,
			     sizeof(s_bitmap_files[i].name),
			     &s_bitmap_files[i].use_key_color,
			     &s_bitmap_files[i].key_color) == 4)
		{
			i++;
		}
	}

	s_nbitmap_files = i;
	fclose(fp);
	return i;
}

static int load_bitmap_num(int i)
{
	FILE *fp;
	int sloti;
	struct bmp *pbmp;
	struct bmp_file *bmpf;
	char path[24];

	if (i >= s_nbitmap_files)
		return E_BITMAPS_LOAD_EOF;

	bmpf = &s_bitmap_files[i];
	if (bmpf->sloti <= 0 || bmpf->sloti >= NBITMAPS) {
		ktrace("bitmap slot out of range %d (%s)", bmpf->sloti,
				bmpf->name);
		return E_BITMAPS_LOAD_ERROR;
	}

	snprintf(path, sizeof(path), "data/%s.bmp", bmpf->name);
	fp = open_file(path, NULL);
	if (fp == NULL)
		return E_BITMAPS_LOAD_ERROR;

	pbmp = load_bmp_fp(fp, NULL);
	fclose(fp);

	if (pbmp == NULL)
		return E_BITMAPS_LOAD_ERROR;

	pbmp->use_key_color = bmpf->use_key_color != 0;
	pbmp->key_color = bmpf->key_color;
	sloti = s_bitmap_files[i].sloti;
	s_bitmap_slots[sloti].pbmp = pbmp;
	s_bitmap_slots[sloti].managed = 1;

	s_load_index++;
	return E_BITMAPS_LOAD_OK;
}

const char *cur_bitmap_name(void)
{
	if (s_load_index < 0 || s_load_index >= s_nbitmap_files)
		return "* none *";

	return s_bitmap_files[s_load_index].name;
}

int load_next_bitmap(void)
{
	return load_bitmap_num(s_load_index);
}

void bitmaps_done(void)
{
	int i;
	struct bmp_slot *pbmp_slot;

	for (i = 1; i < NBITMAPS; i++) {
		pbmp_slot = &s_bitmap_slots[i];
		if (pbmp_slot->managed) {
			if (kassert(pbmp_slot->pbmp != NULL)) {
				free_bmp(pbmp_slot->pbmp, 1);
				pbmp_slot->pbmp = NULL;
				pbmp_slot->managed = 0;
			}
		}
	}

	s_nbitmap_files = 0;
	s_load_index = 0;
}
