#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>
#include "png.h"

const static char
	// Every PNG starts with these 8 bytes.
	HEADER[8] = "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a",
	// Chunk type identifiers.
	tEXt[4] = "\x74\x45\x58\x74",
	xTXt[4] = "\x7A\x54\x58\x74",
	tIME[4] = "\x74\x49\x4D\x45";

/* 
 * Ensures that the first 8 bytes of 'f' are equal to 'HEADER'.
 * If the header is valid, returns 1, otherwise 0.
 */
int valid_header(FILE *f) {
	char c;
	for(int i = 0; i < 8; i++) {
		c = fgetc(f);
		if(c == EOF || c != HEADER[i]) {
			return 0;
		}
	}
	return 1;
}

/*
 * Reads from 'f' and attempts to parse a chunk.
 * Returns 1 if a chunk is parsed, 0 if it was the last chunk in the file, and
 * -1 if the chunk was invalid.
 */
int parse_chunk(FILE *f) {
	return -1;
}

/*
 * Analyze a PNG file.
 * If it is a PNG file, print out all relevant metadata and return 0.
 * If it isn't a PNG file, return -1 and print nothing.
 */
int analyze_png(FILE *f) {
	if(valid_header(f)) {
		int c;
		while((c = parse_chunk(f))) {
			if(c < 0) {
				return -1;
			}
		}
		return 0;
	}
    return -1;
}
