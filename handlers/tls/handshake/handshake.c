#include "handlers/tls/handshake/handshake.h"
#include "util/log.h"


struct tls_key_share_entry_t* tls_select_key_share(
    struct tls_key_share_client_hello_t* client_key_shares, 
    struct tls_supported_groups_t* server_suites) {

}

uint16_t tls_generate_key_share(void* to_write) {

}



void* start_handshake(void* to_write, uint8_t handshake_type) {
    struct tls_handshake_msg_t* header = (struct tls_handshake_msg_t*) to_write;
    header->message_type = handshake_type;

    return (uint8_t*) to_write + sizeof(struct tls_handshake_msg_t);
}

uint32_t end_handshake(void* to_write, uint32_t handshake_length) {
    to_write = (uint8_t*) to_write - sizeof(struct tls_handshake_msg_t);

    if(handshake_length > 0xFFFFFF) {
        LOG_WARNING("Handshake length too large");
    }

    struct tls_handshake_msg_t* header = (uint8_t*) to_write - sizeof(struct tls_handshake_msg_t);    
    header->length[0] = (handshake_length >> 16) & 0xFF;
    header->length[1] = (handshake_length >> 8) & 0xFF;
    header->length[2] = handshake_length & 0xFF;

    return sizeof(struct tls_handshake_msg_t) + handshake_length;
}