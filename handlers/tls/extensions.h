#ifndef HANDLER_HANDLERS_TLS_EXTENSIONS_H
#define HANDLER_HANDLERS_TLS_EXTENSIONS_H

#include <stdint.h>

/*
* General extensions structure and functions
*/

#define TLS_EXTENSION_SERVER_NAME            0
#define TLS_EXTENSION_SUPPORTED_GROUPS       10
#define TLS_EXTENSION_SIGNATURE_ALGORITHMS   13
#define TLS_EXTENSION_KEY_SHARE              51
#define TLS_EXTENSION_SUPPORTED_VERSIONS     43

struct tls_extensions_t {
    uint16_t type;
    uint16_t num_bytes;
} __attribute__((packed, aligned(2)));


/*
* Key Share structures and functions
*/

struct tls_key_share_entry_t {
    uint16_t group;
    uint16_t key_share_length;
} __attribute__((packed, aligned(2)));

struct tls_key_share_client_hello_t {
    uint16_t num_bytes;
    struct tls_key_share_entry_t key_share_entry;
} __attribute__((packed, aligned(2)));

struct tls_key_share_server_hello_t {
    struct tls_key_share_entry_t key_share_entry;
} __attribute__((packed, aligned(2)));

uint16_t tls_set_server_hello_key_share(
    struct tls_key_share_entry_t* server_key_share, 
    uint16_t (*generator)(void* to_write),
    void* to_write);

/*
* Supported Groups structures and functions
*/

struct tls_supported_groups_t {
    uint16_t num_bytes;
    uint16_t* groups;
} __attribute__((packed, aligned(2)));

/*
* Supported Versions structures and functions
*/

struct tls_supported_versions_client_hello_t {
    uint16_t num_bytes;
    uint16_t* versions;
} __attribute__((packed, aligned(2)));

struct tls_supported_versions_server_hello_t {
    uint16_t version;
} __attribute__((packed, aligned(2)));

uint16_t tls_set_server_hello_supported_version(
    struct tls_supported_versions_server_hello_t* version, 
    void* to_write);

#endif // HANDLER_HANDLERS_TLS_EXTENSIONS_H