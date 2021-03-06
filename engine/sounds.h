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

#ifndef SOUNDS_H
#define SOUNDS_H

enum {
	E_SOUNDS_LOAD_OK,
	E_SOUNDS_LOAD_EOF,
	E_SOUNDS_LOAD_ERROR,
	E_SOUNDS_BIGFNAME,
};

enum {
	NSOUNDS = 64
};

struct wav;

int load_sound_list(void);
int load_next_sound(void);
const char *cur_sound_name(void);
void sounds_done(void);

void set_sound(int i, struct wav *pwav);
struct wav *get_sound(int i);

#endif
