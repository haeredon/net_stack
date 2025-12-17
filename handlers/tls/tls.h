#ifndef HANDLER_HANDLERS_TLS_H
#define HANDLER_HANDLERS_TLS_H

#include "handlers/handler.h"
#include "handlers/tls/handshake/handshake.h"

#include <stdbool.h>



#define TLS_SOCKET_BUFFER_SIZE          32

#define TLS_MSG_CLIENT_HELLO            1
#define TLS_MSG_SERVER_HELLO            2
#define TLS_MSG_NEW_SESSION_TICKET      4
#define TLS_MSG_END_OF_EARLY_DATA       5
#define TLS_MSG_ENCRYPTED_EXTENSIONS    8
#define TLS_MSG_CERTIFICATE             11
#define TLS_MSG_CERTIFICATE_REQUEST     13
#define TLS_MSG_CERTIFICATE_VERIFY      15
#define TLS_MSG_FINISHED                20
#define TLS_MSG_KEY_UPDATE              24
#define TLS_MSG_MESSAGE_HASH            254


#define TLS_OUT_BUFFER_LENGTH 2048

#define TLS_PROTOCOL_VERSION_1_2 0x0303
#define TLS_PROTOCOL_VERSION_1_3 0x0304

enum TLS_STATES {
    TLS_SERVER_START,
    TLS_SERVER_RECEIVED_CLIENT_HELLO,
    TLS_SERVER_NEGOTIATED,
    TLS_SERVER_WAIT_END_OF_EARLY_DATA,
    TLS_SERVER_WAIT_FLIGHT_2,
    TLS_SERVER_WAIT_CERTIFICATE,
    TLS_SERVER_WAIT_CERTIFICATE_VERIFY,
    TLS_SERVER_WAIT_FINISHED,
    TLS_CLIENT_START,
    TLS_CLIENT_WAITING_SERVER_HELLO,
    TLS_CLIENT_WAIT_ENCRYPTED_EXTENSIONS,
    TLS_CLIENT_WAIT_CERTIFICATE_REQUEST,
    TLS_CLIENT_WAIT_CERTIFICATE,
    TLS_CLIENT_WAIT_CERTIFICATE_VERIFY,
    TLS_CLIENT_WAIT_FINISHED,
    TLS_CONNECTED
};

struct tls_out_buffer {
    uint8_t buffer[TLS_OUT_BUFFER_LENGTH];
    uint16_t length;
    uint8_t offset;
};

struct tls_control_block_t {
    enum TLS_STATES state;
    struct tls_out_buffer out_buffer;
};

struct tls_certificate_t {
    uint8_t* certificate;
    uint32_t length;
};

struct tls_priv_t {
    struct tls_control_block_t control_block;

    struct tls_certificate_t* certificate;
    struct tls_cipher_suites_t* supported_cipher_suites;
    struct tls_supported_groups_t* supported_groups;
};

struct tls_write_args_t {
  
};

struct tls_server_hello_t {
    uint16_t protocol_version;
    uint32_t random;
    uint8_t data;
} __attribute__((packed, aligned(2)));

struct tls_legacy_session_t {
    uint8_t num_bytes;
    uint16_t* suites;
} __attribute__((packed, aligned(2)));







struct tls_header_t {
    struct tls_record_t record;
} __attribute__((packed, aligned(2)));

struct handler_t* tls_create_handler(struct handler_config_t *handler_config);




#endif // HANDLER_HANDLERS_tls_H