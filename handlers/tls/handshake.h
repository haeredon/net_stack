#ifndef HANDLER_HANDLERS_TLS_HANDSHAKE_H
#define HANDLER_HANDLERS_TLS_HANDSHAKE_H

#include "handlers/tls/extensions.h"

#include <stdint.h>



/**
 * Cipher Suites structures and functions
 */

struct tls_ciphersuites_t {
    uint16_t num_bytes;
    uint16_t* suites;
} __attribute__((packed, aligned(2)));

uint16_t tls_select_cipher_suite(struct tls_ciphersuites_t* client_suites, struct tls_ciphersuites_t* server_suites);


struct tls_key_share_entry_t* tls_select_key_share(struct tls_key_share_client_hello_t* , struct tls_supported_groups_t* server_suites);
struct tls_key_share_entry_t* tls_generate_key_share(void* to_write);











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