#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>
#include "png.h"

// Every PNG starts with these 8 bytes.
const static char HEADER[8] = "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a";

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

}

/*
 * Analyze a PNG file.
 * If it is a PNG file, print out all relevant metadata and return 0.
 * If it isn't a PNG file, return -1 and print nothing.
 */
int analyze_png(FILE *f) {
	if(valid_header(f)) {
		int c;
		while(c = parse_chunk(f)) {
			if(c == -1) {
				return -1;
			}
		}
		return 0;
	}
    return -1;
}
