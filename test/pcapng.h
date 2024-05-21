#ifndef TEST_PCAPNG_H
#define TEST_PCAPNG_H

#include <stdio.h>
#include <stdint.h>

struct packet_block_t {
    uint32_t block_type;
    uint32_t block_length;
    uint32_t interface_id;
    uint32_t timestamp_upper;
    uint32_t timestamp_lower;
    uint32_t captured_package_length;
    uint32_t original_packet_length;
    uint8_t packet_data;
};

struct pcapng_block_type_length_t {
    uint32_t block_type;
    uint32_t block_length;
};

struct pcapng_reader_t {
    uint8_t (*init)(struct pcapng_reader_t* reader, char pcapng_file[]);
    uint8_t (*close)(struct pcapng_reader_t* reader);
    int (*read_block)(struct pcapng_reader_t* reader, void* buffer, uint32_t max_size);
    uint8_t (*has_more_headers)(struct pcapng_reader_t* reader);
    FILE* file;
};


struct pcapng_reader_t* create_pcapng_reader();

 #endif // TEST_PCAPNG_H