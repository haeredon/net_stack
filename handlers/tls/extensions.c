#include "handlers/tls/extensions.h"
#include "handlers/tls/handshake.h"

#include <string.h>

void* start_extensions(void* to_write) {
    return (uint8_t*) to_write + sizeof(struct tls_extensions_msg_t);
}

void* end_extensions(void* to_write, uint16_t extension_length) {
    to_write = (uint8_t*) to_write - sizeof(struct tls_extensions_msg_t);

    struct tls_extensions_msg_t* extension = (struct tls_extensions_msg_t*) to_write;
    extension->extension_length = htons(extension_length);

    return to_write;
}

uint16_t tls_write_extension(const uint16_t extension_type, const uint16_t extension_length, const void* extension_data, void* to_write) {
    struct tls_extensions_t* extension = (struct tls_extensions_t*) to_write;
    extension->type = extension_type;
    extension->num_bytes = htons(extension_length);
    memcpy((uint8_t*) to_write + sizeof(struct tls_extensions_t), extension_data, extension_length);

    return sizeof(struct tls_extensions_t) + extension_length;
}

uint16_t tls_write_server_hello_key_share(struct tls_key_share_entry_t* server_key_share, uint16_t (*generator)(void* to_write), void* to_write) {
    uint16_t num_written = tls_write_extension(TLS_EXTENSION_KEY_SHARE,
                      sizeof(struct tls_key_share_entry_t),
                      server_key_share,
                      to_write);
    return num_written + generator((uint8_t*) to_write + num_written);
    
}

uint16_t tls_write_server_hello_supported_version(struct tls_supported_versions_server_hello_t* version, void* to_write) {
    return tls_write_extension(TLS_EXTENSION_SUPPORTED_VERSIONS,
                      sizeof(struct tls_supported_versions_server_hello_t),
                      version,
                      to_write);
}