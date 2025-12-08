#include "handlers/tls/handshake.h"

struct tls_key_share_entry_t* tls_select_key_share(
    struct tls_key_share_client_hello_t* , 
    struct tls_supported_groups_t* server_suites) {

}

uint16_t tls_generate_key_share(void* to_write) {

}



uint16_t tls_write_handshake_header(uint8_t handshake_type, uint8_t length[3], void* to_write) {
    struct tls_handshake_msg_t* header = (struct tls_handshake_msg_t*) to_write;
    header->message_type = handshake_type;
    header->length[0] = length[0];
    header->length[1] = length[1];
    header->length[2] = length[2];

    return sizeof(struct tls_handshake_msg_t);
}

void* start_encrypted_extensions_handshake(void* to_write) {
    return (uint8_t*) to_write + sizeof(struct tls_handshake_msg_t);
}

void* end_encrypted_extensions_handshake(void* to_write, uint16_t extension_length, uint8_t handshake_length[3]) {
    to_write = (uint8_t*) to_write - sizeof(struct tls_handshake_msg_t);

    tls_write_handshake_header(
        TLS_HANDSHAKE_ENCRYPTED_EXTENSIONS,
        handshake_length,
        to_write
    );

    struct tls_extensions_msg_t* extension = (struct tls_extensions_msg_t*) ((uint8_t*) to_write + sizeof(struct tls_handshake_msg_t));
    extension->extension_length = htons(extension_length);

    return to_write;
}