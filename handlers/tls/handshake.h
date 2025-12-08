#ifndef HANDLER_HANDLERS_TLS_HANDSHAKE_H
#define HANDLER_HANDLERS_TLS_HANDSHAKE_H

#include "handlers/tls/extensions.h"

#include <stdint.h>


#define TLS_HANDSHAKE_CLIENT_HELLO          1
#define TLS_HANDSHAKE_SERVER_HELLO          2
#define TLS_HANDSHAKE_NEW_SESSION_TICKET    4
#define TLS_HANDSHAKE_END_OF_EARLY_DATA     5
#define TLS_HANDSHAKE_ENCRYPTED_EXTENSIONS  8
#define TLS_HANDSHAKE_CERTIFICATE           11
#define TLS_HANDSHAKE_CERTIFICATE_REQUEST   13
#define TLS_HANDSHAKE_CERTIFICATE_VERIFY    15
#define TLS_HANDSHAKE_FINISHED              20
#define TLS_HANDSHAKE_KEY_UPDATE            24
#define TLS_HANDSHAKE_MESSAGE_HASH          254

struct tls_handshake_msg_t {
    uint8_t message_type;
    uint8_t length[3];
} __attribute__((packed, aligned(2)));

uint16_t tls_write_handshake_header(uint8_t handshake_type, uint8_t length[3], void* to_write);

void* start_encrypted_extensions_handshake(void* to_write);
void* end_encrypted_extensions_handshake(void* to_write, uint16_t extension_length, uint8_t handshake_length[3]);






/**
 * Cipher Suites structures and functions
 */

struct tls_ciphersuites_t {
    uint16_t num_bytes;
    uint16_t* suites;
} __attribute__((packed, aligned(2)));

uint16_t tls_select_cipher_suite(struct tls_ciphersuites_t* client_suites, struct tls_ciphersuites_t* server_suites);


struct tls_key_share_entry_t* tls_select_key_share(struct tls_key_share_client_hello_t* , struct tls_supported_groups_t* server_suites);
uint16_t tls_generate_key_share(void* to_write);











struct tls_legacy_compression_methods_t {
    uint8_t num_bytes;
    uint8_t method;
} __attribute__((packed, aligned(2)));

struct tls_legacy_session_t {
    uint8_t num_bytes;
    uint8_t session_id;
} __attribute__((packed, aligned(2)));

struct tls_key_share_t {
    uint8_t num_bytes;
    uint8_t session_id;
} __attribute__((packed, aligned(2)));

struct tls_client_hello_t {
    uint16_t protocol_version;
    uint32_t random;
    struct tls_legacy_session_t* legacy_session;
    struct tls_ciphersuites_t* ciphersuites;
    struct tls_legacy_compression_methods_t* compression_methods;
    
    struct tls_supported_groups_t* supported_groups;
    struct tls_key_share_client_hello_t* key_shares;
} __attribute__((packed, aligned(2)));












#endif // HANDLER_HANDLERS_TLS_HANDSHAKE_H