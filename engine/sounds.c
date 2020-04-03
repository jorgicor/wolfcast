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

#include "sounds.h"
#include "readlin.h"
#include "cbase/cbase.h"
#include "gamelib/vfs.h"
#include "gamelib/wav.h"
#include "cbase/kassert.h"

#ifndef STDLIB_H
#define STDLIB_H
#include <stdlib.h>
#endif

#include <string.h>

struct wav_file {
	char name[9];
	int sloti;
};

struct wav_slot {
	struct wav *pwav;
	int managed;
};

static struct wav_file s_wav_files[NSOUNDS];

/* slot 0 is like NULL */
static struct wav_slot s_wav_slots[NSOUNDS];

static int s_load_index;
static int s_nsound_files;

/* Sets the sound at slot i to pwav.
 * Does nothing if that slot contains a managed sound.
 */
void set_sound(int i, struct wav *pwav)
{
	if (kassert_fails(i > 0 && i < NSOUNDS))
		return;
	if (kassert_fails(!s_wav_slots[i].managed))
		return;
	s_wav_slots[i].pwav = pwav;
}

struct wav *get_sound(int i)
{
	if (i < 0 || i >= NSOUNDS)
		i = 0;
	return s_wav_slots[i].pwav;
}

int load_sound_list(void)
{
	int i;
	FILE *fp;
	char line[READLIN_LINESZ];

	fp = open_file("data/sounds.txt", NULL);
	if (fp == NULL)
		return 0;

	i = 0;
	while (i < NSOUNDS && readlin(fp, line) != -1) {
		if (tokscanf(line, "is", &s_wav_files[i].sloti,
			     s_wav_files[i].name,
			     sizeof(s_wav_files[i].name)))
		{
			i++;
		}
	}

	s_nsound_files = i;
	fclose(fp);
	return i;
}

static int load_sound_num(int i)
{
	FILE *fp;
	int sloti;
	struct wav *pwav;
	struct wav_file *wavf;
	char path[24];

	if (i >= s_nsound_files)
		return E_SOUNDS_LOAD_EOF;

	wavf = &s_wav_files[i];
	if (wavf->sloti <= 0 || wavf->sloti >= NSOUNDS) {
		ktrace("sound slot out of range %d (%s)", wavf->sloti,
				wavf->name);
		return E_SOUNDS_LOAD_ERROR;
	}

	snprintf(path, sizeof(path), "data/%s.wav", wavf->name);
	fp = open_file(path, NULL);
	if (fp == NULL)
		return E_SOUNDS_LOAD_ERROR;

	pwav = load_wav_fp(fp);
	fclose(fp);

	if (pwav == NULL)
		return E_SOUNDS_LOAD_ERROR;

	sloti = s_wav_files[i].sloti;
	s_wav_slots[sloti].pwav = pwav;
	s_wav_slots[sloti].managed = 1;

	s_load_index++;
	return E_SOUNDS_LOAD_OK;
}

const char *cur_sound_name(void)
{
	if (s_load_index < 0 || s_load_index >= s_nsound_files)
		return "* none *";

	return s_wav_files[s_load_index].name;
}

int load_next_sound(void)
{
	return load_sound_num(s_load_index);
}

void sounds_done(void)
{
	int i;
	struct wav_slot *pwav_slot;

	for (i = 1; i < NSOUNDS; i++) {
		pwav_slot = &s_wav_slots[i];
		if (pwav_slot->managed) {
			if (kassert(pwav_slot->pwav != NULL)) {
				free_wav(pwav_slot->pwav);
				pwav_slot->pwav = NULL;
				pwav_slot->managed = 0;
			}
		}
	}

	s_nsound_files = 0;
	s_load_index = 0;
}
