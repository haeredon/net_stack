#ifndef TEST_PCAPNG_H
#define TEST_PCAPNG_H

#include <stdio.h>
#include <stdint.h>


#define PCAPNG_SECTION_BLOCK        0x0A0D0D0A
#define PCAPNG_INTERFACE_DESC_BLOCK 0x00000001
#define PCAPNG_INTERFACE_STAT_BLOCK 0x00000005
#define PCAPNG_ENHANCED_BLOCK       0x00000006
#define PCAPNG_SIMPLE_BLOCK         0x00000003
#define PCAPNG_NAME_BLOCK           0x00000004
#define PCAPNG_CUSTOM_BLOCK         0x00000BAD
#define PCAPNG_DECRYPTION_BLOCK     0x0000000A

struct pcapng_block_type_length_t {
    uint32_t block_type;
    uint32_t block_length;
};

struct packet_enchanced_block_t {
    struct pcapng_block_type_length_t type_length;
    uint32_t interface_id;
    uint32_t timestamp_upper;
    uint32_t timestamp_lower;
    uint32_t captured_package_length;
    uint32_t original_packet_length;
    uint8_t packet_data;
};



#define packet_is_type(PACKET, TYPE)  (((struct pcapng_block_type_length_t*) PACKET)->block_type == TYPE)


struct pcapng_reader_t {
    uint8_t (*init)(struct pcapng_reader_t* reader, char pcapng_file[]);
    uint8_t (*close)(struct pcapng_reader_t* reader);
    int (*read_block)(struct pcapng_reader_t* reader, void* buffer, uint32_t max_size);
    uint8_t (*has_more_headers)(struct pcapng_reader_t* reader);
    FILE* file;
    uint64_t file_length;
    uint64_t bytes_read;
};

struct pcapng_reader_t* create_pcapng_reader();

 #endif // TEST_PCAPNG_H