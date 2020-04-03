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

#include "readlin.h"
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

static int isascii_space(int c)
{
	return c > 0 && c < 128 && isspace(c);
}

/* Reads a line of max READLIN_LINESZ - 1 characters.
 * The new line character is not added.
 * Whitespace trimmed from both ends, and the string is \0 terminated.
 * The two characters \n are changed by one new line character.
 * Returns -1 if EOF, or ~ is found alone on a line.
 * Automatically discards lines starting with # or empty lines.
 * Returns the length of the string (like calling strlen(line)).
 */
int readlin(FILE *fp, char line[])
{
	int c, i, n, lastc;

	i = 0;
	lastc = ' ';
	while ((c = getc(fp)) != EOF) {
		if (c == '\n') {
			if (i == 0)
				continue;
			else
				break;
		}
		if (c == '\r')
			continue;
		if (i == 0 && isascii_space(c))
			continue;
		if (i == 0 && c == '#') {
			do {
				c = getc(fp);
			} while (c != EOF && c != '\n');
			continue;
		}
		if (i < READLIN_LINESZ - 1) {
			if (lastc == '\\' && c == 'n') {
				line[i - 1] = '\n';
			} else {
				line[i++] = c;
			}
			lastc = c;
		}
	}

	/* trim at end */
	n = i - 1;
	while (n >= 0 && isascii_space(line[n])) {
		n--;
	}
	n++;
	line[n] = '\0';

	if ((n == 0 && c == EOF) || (n == 1 && line[0] == '~'))
		return -1;
	else
		return n;
}

/* tokscanf is like a mix between sscanf and strtok.
 *
 * fmt can only be formed from:
 * 	i : like %i
 * 	s : takes a word, needs the argument string, and the maximum size
 * 	S : takes all characters until the end of the line,
 * 	    needs the argument string and the maximum size.
 *
 * For example:
 *
 * char word[5];
 * char str[16];
 * int i;
 * tokscanf(line, "siS", word, sizeof(word), &i, str, sizeof(str));
 *
 * Returns the number of items assigned.
 *
 * Call it the first time with a NULL-terminated string and then with NULL
 * for the rest of items of the same string. For example, the above could
 * have been written:
 * tokscanf(line, "s", word, sizeof(word));
 * tokscanf(NULL, "i", &i);
 * tokscanf(NULL, "S", str, sizeof(str));
 *
 * While scanning S, recognizes the two characters "\n" and substitutes them
 * in the destination string by the single character '\n'.
 */
int tokscanf(char *str, const char *fmt, ...)
{
	va_list ap;
	char *s, *tok;
	size_t i, j, n;
	int r;
	int *pi;

	r = 0;
	va_start(ap, fmt);
	while (fmt != NULL && *fmt != '\0') {
		switch (*fmt++) {
		case 'S':
			/* take a string until the end */
			tok = strtok(str, "");
			s = va_arg(ap, char *);
			n = va_arg(ap, size_t);
			if (s != NULL && n > 0) {
				j = 0;
				if (tok != NULL) {
					for (i = 0; tok[i] != '\0' &&
						j < n - 1; i++, j++)
					{
						if (tok[i] == '\\' && 
							tok[i + 1] == 'n')
						{
							s[j] = '\n';
							i++;
						} else {
							s[j] = tok[i];
						}
					}
				}
				s[j] = '\0';
			}
			r++;
			break;
		case 's':
			/* take a word */
			tok = strtok(str, " ");
			if (tok == NULL) {
				fmt = NULL;
				break;
			}
			s = va_arg(ap, char *);
			n = va_arg(ap, size_t);
			if (s != NULL && n > 0) {
				if (tok != NULL) {
					for (i = 0; tok[i] != '\0' &&
						i < n - 1; i++)
					{
						s[i] = tok[i];
					}
				}
				s[i] = '\0';
			}
			r++;
			break;
		case 'i':
			tok = strtok(str, " ");
			if (tok == NULL) {
				fmt = NULL;
				break;
			}
			pi = va_arg(ap, int *);
			if (sscanf(tok, "%i", pi) == 1)
				r++;
			else
				fmt = NULL;
			break;
		default:
			fmt = NULL;
		}
		str = NULL;
	}

	va_end(ap);
	return r;
}
