#ifndef HANDLER_HANDLERS_TLS_HANDSHAKE_CERTIFICATE_VERIFY_H
#define HANDLER_HANDLERS_TLS_HANDSHAKE_CERTIFICATE_VERIFY_H

#include <stdint.h>

struct tls_certificate_verify_msg_t {
    uint16_t signature_scheme;
    uint16_t signature_length;
} __attribute__((packed, aligned(2)));


uint32_t tls_write_certificate_verify(uint16_t signature_scheme, uint16_t signature_length, 
    void* signature, void* to_write);

#endif // HANDLER_HANDLERS_TLS_HANDSHAKE_CERTIFICATE_VERIFY_H
