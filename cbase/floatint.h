/* Public domain. */

#ifndef FLOATINT_H
#define FLOATINT_H

/* Conversion from float to int is very slow on my computer (x86).
 * This trick taken from 
 *
 * https://www.cs.uaf.edu/2009/fall/cs301/lecture/12_09_float_to_int.html
 *
 * makes it go fast! But only if inlined...
 */

union ifloat {
	float f;
	int i;
};

/* Cast a float to an integer (can be faster than '(int) float_number'). */
static inline int float_to_int(float f)
{
	union ifloat ifl;

	ifl.f = f + (1<<23);
	return ifl.i & 0x7fFFff;
}

#endif
