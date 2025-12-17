#ifndef HANDLER_HANDLERS_TLS_CERTIFICATE_H
#define HANDLER_HANDLERS_TLS_CERTIFICATE_H


#include <stdint.h>

#define TLS_CERTIFICATE_TYPE_X509           0
#define TLS_CERTIFICATE_TYPE_RAW_PUBLIC_KEY 2


struct certificate_request_context {
    uint8_t length;
} __attribute__((packed, aligned(2)));

struct tls_certificate_list_t {
    uint8_t length[3];
} __attribute__((packed, aligned(2)));

struct tls_certificate_entry_t {
    uint8_t length[3];
} __attribute__((packed, aligned(2)));


uint32_t tls_write_certificates(struct tls_certificate_t* certificate, uint8_t num_certificates, void* to_write);


#endif // HANDLER_HANDLERS_TLS_CERTIFICATE_H
