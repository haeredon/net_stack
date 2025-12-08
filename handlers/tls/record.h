#ifndef HANDLER_HANDLERS_TLS_RECORD_H
#define HANDLER_HANDLERS_TLS_RECORD_H

#include <stdint.h>


#define TLS_RECORD_CONTENT_TYPE_HANDSHAKE           22
#define TLS_RECORD_CONTENT_TYPE_ALERT               21
#define TLS_RECORD_CONTENT_TYPE_APPLICATION_DATA    23
#define TLS_RECORD_CONTENT_TYPE_CHANGE_CIPHER_SPEC  20


struct tls_record_t {
    uint8_t content_type;
    uint16_t protocol_version;
    uint16_t length;
} __attribute__((packed, aligned(2)));



#endif // HANDLER_HANDLERS_TLS_RECORD_H