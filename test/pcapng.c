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


// void send_test_data(struct pcapng_reader_t* reader) {
// 	uint8_t block_buffer[0xFFFF]; // 65535 bytes

// 	/* Create the mbuf pool. 8< */
// 	struct rte_mempool* pool = rte_pktmbuf_pool_create("mbuf_pool_2", 8192U,
// 		MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
// 		rte_socket_id());
// 	if (pool == NULL)
// 		rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");

// 	if(reader->init(reader)) {
// 			exit(1);
// 	}

// 	int i = 0;

// 	while(reader->has_more_headers(reader)) {
// 		const static uint8_t HEADER_SIZE = 32;

// 		memset(block_buffer, 0, 0xFFFF);
				
// 		if(reader->read(reader, block_buffer, HEADER_SIZE)) {
// 			return;
// 			//exit(1);
// 		}

// 		struct packet_block_t* packet_block = (struct packet_block_t*) block_buffer;

// 		if(reader->read(reader, block_buffer + HEADER_SIZE, packet_block->block_length - HEADER_SIZE)) {
// 			return;
// 			//exit(1);
// 		}
		
// 		if(packet_block->block_type == PACKET_BLOCK) {
// 			uint32_t packet_length = packet_block->captured_package_length;

// 			struct rte_mbuf* packet = rte_pktmbuf_alloc(pool);

// 			packet->data_len = packet_length;
// 			packet->pkt_len = packet_length;

// 			uint8_t* new_packet_block = rte_pktmbuf_mtod(packet, uint8_t*);

// 			memcpy(new_packet_block, &packet_block->packet_data, packet_length);

// 			printf("DONE: %d\n", ++i);

// 			l2fwd_simple_forward(packet);
// 		}

// 		int kage = reader->has_more_headers(reader);
// 		int e = 3;
// 	}
// }