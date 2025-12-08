#ifndef HANDLER_HANDLERS_TLS_CERTIFICATE_H
#define HANDLER_HANDLERS_TLS_CERTIFICATE_H


#define TLS_CERTIFICATE_TYPE_X509         0
#define TLS_CERTIFICATE_TYPE_RAW_PUBLIC_KEY 2


struct certificate_request_context {
    uint8_t length;
    uint8_t data[];
};

struct tls_certificate_msg_t {
    uint16_t type;
    uint16_t num_bytes;
} __attribute__((packed, aligned(2)));


void tls_write_certificate(struct tls_certificate_t certificate, void* buffer);


#endif // HANDLER_HANDLERS_TLS_CERTIFICATE_H
