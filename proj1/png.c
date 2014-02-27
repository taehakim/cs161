#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>
#include "png.h"

/*
 * Returns 1 if the two arrays are the same, 0 otherwise.
 */
int array_cmp(char a[], char b[], int n) {
	while(n--) {
		if(a[n] != b[n]) {
			return -1;
		}
	}
	return 0;
}

const static char
	// Every PNG starts with these 8 bytes.
	HEADER[8] = "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a",
	// Chunk type identifiers.
	tEXt[4] = "\x74\x45\x58\x74",
	zTXt[4] = "\x7A\x54\x58\x74",
	tIME[4] = "\x74\x49\x4D\x45";

/* 
 * Ensures that the first 8 bytes of 'f' are equal to 'HEADER'.
 * If the header is valid, returns 0, otherwise -1.
 */
int validate_header(FILE *f) {
	char c;
	for(int i = 0; i < 8; i++) {
		c = fgetc(f);
		if(c == EOF || c != HEADER[i]) { return -1; }
	}
	return 0;
}

/*
 * Validates the checksum of 'data'. If valid, returns 0, otherwise -1.
 */
int validate_checksum(char data[], int length, int checksum) {
	return -1;
}

/*
 * Attempts to parse the length of a chunk from 'f'. If four bytes are read then
 * the integer equivalent is returned, otherwise -1;
 */
int parse_length(FILE *f) {
	char c;
	int i, length = 0;
	for(i = 3; i >= 0; i--) {
		c = fgetc(f);
		if(c == EOF) { return -1; }
		length |= (c << (i * 8));
	}
	if(length < 0) { return -1; }
	return length;
}

/*
 * Attempts to parse the type of a chunk from 'f'. If four bytes are read and
 * match one of the 4 possible types, then an nonnegative integer corresponding
 * to a type is returned, otherwise -1.
 * tEXt: 0
 * zTXt: 1
 * tIME: 2
 */
int parse_chunktype(FILE *f) {
	char c, bytes[4];
	int i;
	for(i = 0; i < 4; i++) {
		c = fgetc(f);
		if(c == EOF) { return -1; }
		bytes[i] = c;
	}
	if(array_cmp(bytes, tEXt, 4) != -1) return 0;
	if(array_cmp(bytes, zTXt, 4) != -1) return 1;
	if(array_cmp(bytes, tIME, 4) != -1) return 2;
	return -1;
}

/*
 * Reads 'length' bytes from 'f' into 'data'. If 'length' bytes are read, 0 is
 * returned, otherwise -1;
 */
int parse_data(FILE *f, char data[], int length) {
	char c;
	for(int i = 0; i < length; i++) {
		c = fgetc(f);
		if(c == EOF) { return -1; }
		data[i] = c;
	}
	return 0;
}

/*
 * Reads from 'f' and attempts to parse a chunk.
 * Returns 1 if a chunk is parsed, 0 if it was the last chunk in the file, and
 * -1 if the chunk was invalid.
 */
int parse_chunk(FILE *f) {
	int length = parse_length(f);
	if(length == -1) { return -1; }
	int chunktype = parse_chunktype(f);
	if(chunktype == -1) { return -1; }
	char data[length];
	if(parse_data(f, data, length) == -1) { return -1; }
	int checksum = parse_checksum(f);
	if(checksum == -1) { return -1; }
	if(validate_checksum(data, length, checksum) == -1) { return -1; }
	switch(chunktype) {
		case 0:
			parse_tEXt()
	}
	return 0;
}

/*
 * Analyze a PNG file.
 * If it is a PNG file, print out all relevant metadata and return 0.
 * If it isn't a PNG file, return -1 and print nothing.
 */
int analyze_png(FILE *f) {
	if(validate_header(f) != -1) {
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
