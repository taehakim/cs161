#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>
#include "png.h"

int seen_time;

// Every PNG starts with these 8 bytes.
static char HEADER[8] = "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a";
// Chunk types.
static char* CHUNK_TYPES[3] = {
	"\x74\x45\x58\x74", // tEXt
	"\x7A\x54\x58\x74", // zTXt
	"\x74\x49\x4D\x45"  // tIME
};

/*
 * Returns 0 if the two arrays are the same, -1 otherwise.
 */
int array_cmp(char a[], char b[], int n) {
	while(n--) {
		if(a[n] != b[n]) { return -1; }
	}
	return 0;
}

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
 * Attempts to parse four bytes from 'f' and convert them to an int and then
 * return that int, otherwise return -1;
 */
int parse_int(FILE *f) {
	int c, i = 4, length = 0;
	while(i--) {
		c = fgetc(f);
		if(c == EOF) { return -1; }
		length |= (c << (i * 8));
	}
	return length;
}

/*
 * Returns the index of 'chunktype' in CHUNK_TYPES, otherwise -1.
 */
int parse_chunktype(FILE *f) {
	char bytes[4];
	int i, c;
	for(i = 0; i < 4; i++) {
		c = fgetc(f);
		if(c == EOF) { return -1; }
		bytes[i] = c;
	}
	for(i = 0; i < 3; i++) {
		if(array_cmp(bytes, CHUNK_TYPES[i], 4) != -1) {
			return i;
		}
	}
	return 3;
}

uLong generate_checksum(int chunktype, char data[], int length) {
	uLong crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, (Bytef*) CHUNK_TYPES[chunktype], 4);
	return crc32(crc, (Bytef*) data, length);
}

/*
 * Parses 'data' expecting a tEXt chunk. Returns 0 if parsing succeeds,
 * otherwise -1.
 */
int parse_tEXt(char data[], int length) {
	return -1;
}

/*
 * Parses 'data' expecting a zTXt chunk. Returns 0 if parsing succeeds,
 * otherwise -1.
 */
int parse_zTXt(char data[], int length) {
	return -1;
}

/*
 * Parses 'data' expecting a tIME chunk. Returns 0 if parsing succeeds,
 * otherwise -1.
 */
int parse_tIME(char data[], int length) {
	// All tIME chunks should be 7 bytes long.
	if(length != 7) { return -1; }
	int year = (data[0] << 8) | data[1];
	printf("Timestamp: %d/%d/%d %d:%d:%d",
		data[2], data[3], year,
		data[4], data[5], data[6]);
	return 0;
}

/*
 * Reads from 'f' and attempts to parse a chunk.
 * Returns 1 if a chunk is parsed, 0 if it was the last chunk in the file, and
 * -1 if the chunk was invalid.
 */
int parse_chunk(FILE *f) {
	// Parse length.
	printf("Parsing length\n");
	int length = parse_int(f);
	if(length < 0) { return -1; }
	// Parse chunktype.
	printf("Parsing chunk type\n");
	int chunktype = parse_chunktype(f);
	if(chunktype < 0) { return -1; }
	// Unknown chunk type, skip.
	if(chunktype > 2) {
		fseek(f, length + 4, SEEK_CUR);
	} else {
		// Initialize data buffer;
		char* data = malloc(sizeof(char) * length);
		if(data == NULL) { exit(1); }
		// Read data buffer.
		printf("Copying data\n");
		if(fread(data, sizeof(char), length, f) != length) { return -1; }
		// Parse checksum.
		printf("Parsing checksum\n");
		int expected_checksum = parse_int(f);
		if(expected_checksum == -1) { return -1; }
		// Generate checksum.
		int actual_checksum = generate_checksum(chunktype, data, length);
		// Compare checksums.
		printf("Comparing checksums\n");
		printf("%d\n", actual_checksum);
		printf("%d\n", expected_checksum);
		if(actual_checksum != expected_checksum) { return -1; }
		// Parse data based on chunk type.
		printf("Parsing data\n");
		int parse_data = -1;
		switch(chunktype) {
			case 0: parse_data = parse_tEXt(data, length); break;
			case 1: parse_data = parse_zTXt(data, length); break;
			case 2: parse_data = parse_tIME(data, length); break;
		}
		// Free ALL the data!
		free(data);
		if(parse_data == -1) { return -1; }
	}
	// Return 0 if this is the last chunk in the file.
	if(feof(f)) { return 0; }
	// Return 1 to keep parsing.
	return 1;
}

/*
 * Analyze a PNG file.
 * If it is a PNG file, print out all relevant metadata and return 0.
 * If it isn't a PNG file, return -1 and print nothing.
 */
int analyze_png(FILE *f) {
	seen_time = 0;
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
