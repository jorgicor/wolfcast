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

#ifndef BITMAPS_H
#define BITMAPS_H

enum {
	E_BITMAPS_LOAD_OK,
	E_BITMAPS_LOAD_EOF,
	E_BITMAPS_LOAD_ERROR,
	E_BITMAPS_BIGFNAME,
};

enum {
	NBITMAPS = 64
};

struct bmp;

int load_bitmap_list(void);
int load_next_bitmap(void);
const char *cur_bitmap_name(void);
void bitmaps_done(void);

void set_bitmap(int i, struct bmp *pbmp);
struct bmp *get_bitmap(int i);

#endif
