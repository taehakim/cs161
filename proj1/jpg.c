#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "jpg.h"
#include "fpeek.h"

// A valid TIFF file starts with these 6 bytes.
const static unsigned char TIFF_HEADER[6] = "\x45\x78\x69\x66\x00\x00";

// An ASCII UserComment begins with these 8 bytes.
const static unsigned char ASCII_USER_COMMENT[8] = "\x41\x53\x43\x49\x49\x00\x00\x00";

// An array of the various valid tagids.
const static unsigned short TAGIDS[15] = {
	0x010d, // DocumentName
	0x010e, // ImageDescription
	0x010f, // Make
	0x0110, // Model
	0x0131, // Software
	0x0132, // DateTime
	0x013b, // Artist
	0x013c, // HostComputer
	0x8298, // Copyright
	0xA004, // RelatedSoundFile
	0x9003, // DateTimeOriginal
	0x9004, // DateTimeDigitized
	0x927c, // MakerNote
	0x9286, // UserComment
	0xA420  // ImageUniqueID
};

// An array of tag names corresponding to TAGIDS.
const static char* TAG_NAMES[15] = {
	"DocumentName",      // Name of the document from which the image was obtained.
	"ImageDescription",  // A Description of the image.
	"Make",              // Make of the camera.
	"Model",             // Model of the camera.
	"Software",          // Software used to create the image.
	"DateTime",          // Date and time of image creation.
	"Artist",            // Creator of the image.
	"HostComputer",      // Computed used to create the image.
	"Copyright",         // Copyright notice.
	"RelatedSoundFile",  // Name of a related audio file.
	"DateTimeOriginal",  // Date and time or original image creation.
	"DateTimeDigitized", // Date and time when image was stored as digital data.
	"MakerNote",         // Manufacturer-defined information.
	"UserComment",       // Comments on the image from the creator.
	"ImageUniqueID"      // A unique identifier assigned to the image.
};

/*
 * Reads two bytes from 'f' and returns an int from the two bytes, if two bytes
 * could not be read, then -1 is returned.
 */
int parse_short(FILE *f, int big_endian) {
	// Fetch the most significant byte.
	int high = fgetc(f);
	if(high == EOF) { return -1; }
	// Fetch the least significant byte.
	int low = fgetc(f);
	if(low == EOF) { return -1; }
	// Combine the two bytes into an int.
	if(big_endian) {
		return (high << 8) | low;
	} else {
		return high | (low << 8);
	}
}

/*
 * Parses a chunk marker. Returns the marker if successfully parsed, otherwise
 * -1;
 */
int parse_marker(FILE *f) {
	int marker = parse_short(f, 1);
	if(marker == -1) { return -1; }
	// Ensure the marker is within the valid marker range.
	if(marker < 0xff01 || marker > 0xfffe) { return -1; }
	return marker;
}

/*
 * Parses a chunk length. Returns the length if successfully parsed, otherwise
 * -1.
 */
int parse_length(FILE *f) {
	int length = parse_short(f, 1);
	if(length < 0) { return -1; }
	return length;
}

/*
 * Returns how long the data section of a superchunk is. If a valid superchunk
 * data section could not be found, then return -1. A superchunk data section
 * ends when the byte 0xff is not followed by the byte 0x00.
 */
int find_next_chunk(FILE *f) {
	// EOI (End Of Image) is at the end of the file and has no data.
	if(fpeek(f) == EOF) { return 0; }
	int c;
	// Read a byte until the end of the file.
	while((c = fgetc(f)) != EOF) {
		// If the byte was 0xff, check whether the next byte is 0x00.
		if(c == 0xff && fpeek(f) != 0x00) {
			// Rewind the stream back past 0xff to the end of the data section.
			ungetc(c, f);
			return 0;
		}
	}
	// If an EOF was reached and it wasn't the first byte, it's an invalid file
	// because the last chunk should be an EOI (End Of Image) superchunk which
	// has no data.
	return -1;
}

/*
 * Returns 0 if 'marker' refers to a superchunk marker, otherwise returns -1.
 */
int is_super_chunk(int marker) {
	return (marker >= 0xffd0 && marker <= 0xffda) ? 0 : -1;
}

/*
 * Returns 0 if 'marker' refers to an APP1 chunk marker, otherwise returns -1.
 */
int is_app1_chunk(int marker) {
	return (marker == 0xffe1) ? 0 : -1;
}

/*
 * Returns 0 if the next 6 bytes from 'f' are equal to TIFF_HEADER, otherwise returns
 * -1.
 */
int validate_tiff_header(FILE *f) {
	int c, i;
	for(i = 0; i < sizeof(TIFF_HEADER); i++) {
		c = fgetc(f);
		if(c == EOF || c != TIFF_HEADER[i]) { return -1; }
	}
	return 0;
}

/*
 * Attempts to read four bytes from 'f' as the offset to the 0th IFD in little-
 * endian format.
 */
int parse_offset(FILE *f) {
	unsigned char offset[4];
	if(fread(offset, sizeof(unsigned char), 4, f) != 4) { return -1; }
	return offset[0] |
		(offset[1] <<  8) |
		(offset[2] << 16) |
		(offset[3] << 24);
}

/*
 * Returns 0 if the datatype corresponds to an ASCII or undefined datatype,
 * -1 otherwise.
 */
int is_string_datatype(int datatype) {
	return (datatype == 2 || datatype == 7) ? 0 : -1;
}

/*
 * Returns 0 if the tagid corresponds to the Exif IFD ptr tagid, -1 otherwise.
 */
int is_exif_ptr_tagid(int tagid) {
	return (tagid == 0x8769) ? 0 : -1;
}

/*
 * Returns 0 if the tagid corresponds to a UserComment, -1 otherwise.
 */
int is_user_comment(int tagid) {
	return (tagid == 0x9286) ? 0 : -1;
}

/*
 * Returns 0 if the next 8 bytes from 'f' correspond to the UserComment
 * character set identifier for ASCII, otherwise returns -1. If the character
 * set identifier does correspond to ASCII, then 'count' is decremented by the
 * length of the identifier.
 */
int validate_ascii_user_comment(FILE *f, int *count) {
	int c, i;
	for(i = 0; i < sizeof(ASCII_USER_COMMENT); i++) {
		if((c = fgetc(f)) != ASCII_USER_COMMENT[i]) { return -1; }
	}
	*count -= sizeof(ASCII_USER_COMMENT);
	return 0;
}

/*
 * Prints the tag name corresponding to 'tagid' and returns 0, or returns -1 if
 * the tagid was not found in TAGIDS.
 */
int print_tag_name(int tagid) {
	int i;
	for(i = 0; i < sizeof(TAGIDS); i++) {
		if(tagid == TAGIDS[i]) {
			printf("%s: ", TAG_NAMES[i]);
			return 0;
		}
	}
	return -1;
}

/*
 * Prints the 4 chars represented within the int.
 */
void print_value(unsigned int value) {
	printf("%.4s\n", (char*) &value);
}

/*
 * Print 'count' bytes from 'f' unless the data is null terminated, in which
 * case, stop printing when we encounter a null character. If we ever each the
 * EOF, return -1, otherwise return 0.
 */
int print_offset_data(FILE *f, int count) {
	int c;
	while(count--) {
		// If we get to the EOF, then there was an error.
		if((c = fgetc(f)) == EOF) { return -1; }
		// If we encounter the null character, stop printing.
		if(c == 0) { break; }
		printf("%c", c);
	}
	// Put each key and value on a new line.
	printf("\n");
	return 0;
}

/*
 * Parse an IFD section in the PNG and returns the Exif IFD ptr or -1 if parsing
 * fails. Returns an Exif pointer if found. This function is responsible for
 * not modifying the file position when it returns.
 */
int parse_ifd(FILE *f, int offset) {
	// Store the initial position of the file.
	long int position = ftell(f);
	// Seek to the offset.
	if(fseek(f, offset, SEEK_CUR) != 0) { return -1; }
	// Parse how many tags are in this IFD and ensure it is nonnegative.
	int tags = parse_short(f, 0);
	if(tags < 0) { return -1; }
	// Vars for the while loop.
	int exif_ptr = 0, tagid, datatype, count, offset_or_value;
	// Loop for each tag there should be.
	while(tags--) {
		// Read the tagid.
		tagid = parse_short(f, 0);
		if(tagid == -1) { return -1; }
		// Read the datatype.
		datatype = parse_short(f, 0);
		if(datatype == -1) { return -1; }
		// Read the count and ensure it is nonnegative.
		count = parse_offset(f);
		if(count < 0) { return -1; }
		// Read the offset or value.
		offset_or_value = parse_offset(f);
		if(offset_or_value == -1) { return -1; }
		// Check whether the tagid is the Exif IFD ptr.
		if(is_exif_ptr_tagid(tagid) != -1) {
			// Store the pointer to return later.
			exif_ptr = offset_or_value;
		// Check whether the datatype is either ASCII or undefined.
		} else if(is_string_datatype(datatype) != -1) {
			// Print the tag name. If the tagid was not found in TAGIDS, then -1
			// was returned and we can skip over parsing the value.
			if(print_tag_name(tagid) != -1) {
				// If count is less than or equal to 4, then the value can fit
				// within the offset_or_value field itself.
				if(count <= 4) {
					print_value(offset_or_value);
				// Otherwise offset_or_value defines where the data is located
				// farther along in the data.
				} else {
					// Store the current position of the file.
					long int tag_position = ftell(f);
					// Position the file back to the beginning of the TIFF
					// section.
					if(fseek(f, position, SEEK_SET) != 0) { return -1; }
					// Position the file at the tag's data.
					if(fseek(f, offset_or_value, SEEK_CUR) != 0) { return -1; }
					// Check whether the tagid does not refer to a UserComment
					// or whether the UserComment is ASCII. If the UserComment
					// is ASCII, then count is decremented by the length of the
					// character set identifier.
					if(is_user_comment(tagid) != 0 || validate_ascii_user_comment(f, &count) == 0) {
						if(print_offset_data(f, count) == -1) {
							return -1;
						}
					}
					// Reset the position of the file.
					if(fseek(f, tag_position, SEEK_SET) != 0) { return -1; }
				}
			}
		}
	}
	// Reset the position of the file.
	if(fseek(f, position, SEEK_SET) != 0) { return -1; }
	return exif_ptr;
}

/*
 * Parses an APP1 chunk and returns 0 if successful, otherwise returns -1. This
 * function does not advance the position of the file.
 */
int parse_app1_chunk(FILE *f) {
	// Validate the APP1 header.
	if(validate_tiff_header(f) == -1) { return -1; }
	// Validate endianness, always little (0x49 0x49) in this project.
	if(fgetc(f) != 0x49 || fgetc(f) != 0x49) { return -1; }
	// Valiate magic string, always 0x2a 0x00 in this project.
	if(fgetc(f) != 0x2a || fgetc(f) != 0) { return -1; }
	// Parse the offset of the 0th IFD.
	int offset = parse_offset(f);
	if(offset == -1) { return -1; }
	// Rewind the stream back the beginning of the TIFF file, before the
	// endianness and magic string fields and the offset.
	if(fseek(f, -8, SEEK_CUR) != 0) { return -1; }
	// Parse the 0th IFD.
	offset = parse_ifd(f, offset);
	if(offset == -1) { return -1; }
	// If the offset is zero, it means we didn't find an Exif IFD ptr.
	if(offset != 0) {
		// Parse the Exif IFD.
		offset = parse_ifd(f, offset);
		if(offset == -1) { return -1; }
	}
	// Rewind to the beginning of the APP1 dection, before the APP1 header.
	if(fseek(f, -sizeof(TIFF_HEADER), SEEK_CUR) != 0) { return -1; }
	return 0;
}

/*
 * Parses a chunk. Returns 1 if a chunk is successfully parsed, 0 if it is the
 * last chunk in the file, and -1 if there is an error.
 */
int parse_jpg_chunk(FILE *f) {
	// Parse the chunk marker.
	int marker = parse_marker(f);
	if(marker == -1) { return -1; }
	// Check whether the chunk is a super chunk or a standard chunk.
	if(is_super_chunk(marker) != -1) {
		// Forward the stream to the next chunk.
		if(find_next_chunk(f) == -1) { return -1; }
	} else {
		// Parse the chunk data length.
		int length = parse_length(f);
		if(length == -1) { return -1; }
		// Subtract 2 because the length corresponds to the size of the data
		// section and the length field.
		length -= 2;
		// Test whether the chunk is an APP1 chunk or not.
		if(is_app1_chunk(marker) != -1) {
			// Parse the APP1 chunk. There is only 1 APP1 chunk in the files
			// relevant to this project, so if parsing succeeds, just quit,
			// otherwise error.
			return (parse_app1_chunk(f) == 0) ? 0 : -1;
		}
		// Ensure the length is nonnegative and forward the position to the end
		// of the chunk.
		if(length < 0 || fseek(f, length, SEEK_CUR) != 0) { return -1; }
	}
	if(fpeek(f) == EOF) { return 0; }
	return 1;
}

/*
 * Analyze a JPG file that contains Exif data.
 * If it is a JPG file, print out all relevant metadata and return 0.
 * If it isn't a JPG file, return -1 and print nothing.
 */
int analyze_jpg(FILE *f) {
	int c;
	while((c = parse_jpg_chunk(f))) {
		if(c < 0) { return -1; }
	}
    return 0;
}
