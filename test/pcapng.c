#include "pcapng.h"

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

uint8_t pcapng_init(struct pcapng_reader_t* reader, char pcapng_file[]) {
    reader->file = fopen(pcapng_file, "rb");

	if(!reader->file) {
		printf("Could not open pcapng file. errno: %d\n", errno);
		return 1;
	}


	if(fseek(reader->file, 0, SEEK_END)) {
		printf("Could not seek in pcapng file. errno: %d\n", errno);
		return 1;
	}

	if((reader->file_length = ftell(reader->file)) == -1) {
		printf("Could not get pcapng file length. errno: %d\n", errno);
		return 1;
	}

	rewind(reader->file);

	return 0;
}

int i = 0;

int pcapng_read_block(struct pcapng_reader_t* reader, void* buffer, uint32_t max_size) {
    static const uint8_t HEADER_TYPE_LENGTH = 8;

    if(fread(buffer, 1, HEADER_TYPE_LENGTH, reader->file) != HEADER_TYPE_LENGTH) {		
        printf("Failed to read pcapng block header. errno %d\n", errno);			
        return -1;
    }

    struct pcapng_block_type_length_t* type_length = (struct pcapng_block_type_length_t*) buffer;
    uint32_t num_to_read = type_length->block_length;

	if(num_to_read + HEADER_TYPE_LENGTH > max_size) {
		printf("Packet too big.\n");			
		return -1;
	}

	uint32_t header_bytes_read = HEADER_TYPE_LENGTH;
	while(!feof(reader->file) && header_bytes_read != num_to_read) {
		header_bytes_read += fread(buffer + header_bytes_read, 1, num_to_read - header_bytes_read, reader->file);

		if(ferror(reader->file)) {
			printf("Failed to read pcapng file. errno %d\n", errno);			
			return -1;
		}
	}
	reader->bytes_read += header_bytes_read;
	return header_bytes_read;
}

uint8_t pcapng_has_more_headers(struct pcapng_reader_t* reader) {
	return reader->bytes_read != reader->file_length;
}

uint8_t pcapng_close(struct pcapng_reader_t* reader) {
    return fclose(reader->file);    
}

struct pcapng_reader_t* create_pcapng_reader() {
	struct pcapng_reader_t* reader = (struct pcapng_reader_t*) malloc(sizeof(struct pcapng_reader_t));

	reader->init = pcapng_init;
	reader->close = pcapng_close;
	reader->read_block = pcapng_read_block;
	reader->has_more_headers = pcapng_has_more_headers;

	reader->bytes_read = 0;

	return reader;
}