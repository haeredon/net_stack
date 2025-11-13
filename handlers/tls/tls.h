#ifndef HANDLER_HANDLERS_TLS_H
#define HANDLER_HANDLERS_TLS_H

#include "handlers/handler.h"

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

#define TLS_RECORD_CONTENT_TYPE_HANDSHAKE           22
#define TLS_RECORD_CONTENT_TYPE_ALERT               21
#define TLS_RECORD_CONTENT_TYPE_APPLICATION_DATA    23
#define TLS_RECORD_CONTENT_TYPE_CHANGE_CIPHER_SPEC  20




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

struct tls_control_block_t {
    void (*handshake)();
    enum TLS_SERVER_STATE state;
    uint8_t out_header[1024];
};

struct tls_priv_t {
    struct tls_control_block_t control_block;
};

struct tls_write_args_t {
  
};

struct tls_server_hello_t {
    //   struct {
    //       ProtocolVersion legacy_version = 0x0303;    /* TLS v1.2 */
    //       Random random;
    //       opaque legacy_session_id_echo<0..32>;
    //       CipherSuite cipher_suite;
    //       uint8 legacy_compression_method = 0;
    //       Extension extensions<6..2^16-1>;
    //   } ServerHello;
} __attribute__((packed, aligned(2)));

struct tls_client_hello_t {
    uint16_t protocol_version;
    uint32_t random;
} __attribute__((packed, aligned(2)));

struct tls_legacy_compression_methods_t {
    uint8_t num_bytes;
    void* methods;
} __attribute__((packed, aligned(2)));

struct tls_legacy_session_t {
    uint8_t num_bytes;
    uint16_t* suites;
} __attribute__((packed, aligned(2)));

struct tls_ciphersuites_t {
    uint16_t num_bytes;
    uint16_t* suites;
} __attribute__((packed, aligned(2)));

struct tls_extensions_t {
    uint16_t num_bytes;
    uint16_t* extensions;
} __attribute__((packed, aligned(2)));


struct tls_record_t {
    uint8_t content_type;
    uint16_t protocol_version;
    uint16_t length;
    void* data;
} __attribute__((packed, aligned(2)));

struct tls_handshake_t {
    uint8_t message_type;
    uint8_t length[3];
    void* data;
} __attribute__((packed, aligned(2)));

struct tls_header_t {
    struct tls_record_t record;
    void* data;   
} __attribute__((packed, aligned(2)));

struct handler_t* tls_create_handler(struct handler_config_t *handler_config);




#endif // HANDLER_HANDLERS_tls_H