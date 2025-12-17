#ifndef HANDLER_HANDLERS_TLS_HANDSHAKE_FINISHED_H
#define HANDLER_HANDLERS_TLS_HANDSHAKE_FINISHED_H

#include <stdint.h>

uint32_t tls_write_finished(void* verify_data, uint16_t length, void* to_write);

#endif // HANDLER_HANDLERS_TLS_HANDSHAKE_FINISHED_H
