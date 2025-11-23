#ifndef HANDLER_HANDLERS_TLS_HANDSHAKE_H
#define HANDLER_HANDLERS_TLS_HANDSHAKE_H

#include <stdint.h>

struct tls_ciphersuites_t {
    uint16_t num_bytes;
    uint16_t* suites;
} __attribute__((packed, aligned(2)));

uint16_t tls_select_cipher_suite(struct tls_ciphersuites_t client_suites, struct tls_ciphersuites_t server_suites);





#endif // HANDLER_HANDLERS_TLS_HANDSHAKE_H