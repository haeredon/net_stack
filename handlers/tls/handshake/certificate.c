#include "handlers/tls/handshake/certificate.h"
#include "handlers/tls/tls.h"

#include <string.h>

uint32_t tls_write_certificates(struct tls_certificate_t* certificate, uint8_t num_certificates, void* to_write) {
    // for now, request context is empty (only for server authentication)
    struct certificate_request_context* request_context = (struct certificate_request_context*) to_write;
    request_context->length = 0;

    struct tls_certificate_list_t* cert_list = (struct tls_certificate_list_t*) ((uint8_t*) to_write + sizeof(struct certificate_request_context));

    struct tls_certificate_entry_t* cert_entry = (struct tls_certificate_entry_t*) ((uint8_t*) cert_list + sizeof(struct tls_certificate_list_t));
    uint32_t total_size = 0;
    for(uint8_t i = 0; i < num_certificates; i++) {                        
        cert_entry->length[0] += (certificate[i].length >> 16) & 0xFF;
        cert_entry->length[1] += (certificate[i].length >> 8) & 0xFF;
        cert_entry->length[2] += certificate[i].length & 0xFF;

        memcpy((uint8_t*) cert_entry + sizeof(struct tls_certificate_entry_t) + total_size, certificate[i].certificate, certificate[i].length);
        
        uint16_t* num_extensions = (uint16_t*) ((uint8_t*) cert_entry + sizeof(struct tls_certificate_entry_t) + certificate[i].length);
        *num_extensions = 0; // no extensions for now                                

        total_size += certificate[i].length + sizeof(struct tls_certificate_list_t) + sizeof(uint16_t) /* num_extensions */;
    }  

    cert_list->length[0] = (total_size >> 16) & 0xFF;
    cert_list->length[1] = (total_size >> 8) & 0xFF;
    cert_list->length[2] = total_size & 0xFF;

    return sizeof(struct certificate_request_context) + sizeof(struct tls_certificate_list_t) + total_size;
}