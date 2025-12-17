#include "handlers/tls/handshake/certificate_verify.h"


uint32_t tls_write_certificate_verify(uint16_t signature_scheme, uint16_t signature_length, 
        void* signature, void* to_write) {
    struct tls_certificate_verify_msg_t* cert_verify = (struct tls_certificate_verify_msg_t*) to_write;
    cert_verify->signature_scheme = htons(signature_scheme);
    cert_verify->signature_length = htons(signature_length);
    memcpy((uint8_t*) to_write + sizeof(struct tls_certificate_verify_msg_t), signature, signature_length);

    return sizeof(struct tls_certificate_verify_msg_t) + signature_length;
}