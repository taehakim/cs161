#include <stdio.h>
#include "fpeek.h"

/*
 * Returns the next byte in 'f' without advancing the position.
 */
int fpeek(FILE *f) {
	int c = fgetc(f);
	ungetc(c, f);
	return c;
}