#include "handlers/tls/tls.h"
#include "handlers/tls/handshake.h"

#include "util/log.h"

#include <string.h>


/*
 * Transport Layer Security (TLS).
 * Specification: RFC 8446
 * 
 */



void tls_close_handler(struct handler_t* handler) {
    struct tls_priv_t* private = (struct tls_priv_t*) handler->priv;    
    NET_STACK_FREE(private);
}

void tls_init_handler(struct handler_t* handler, void* priv_config) {
    struct tls_priv_t* ipv4_priv = (struct tls_priv_t*) NET_STACK_MALLOC("ipv4 handler private data", sizeof(struct tls_priv_t)); 
    handler->priv = (void*) ipv4_priv;
}


bool tls_write(struct out_packet_stack_t* packet_stack, struct interface_t* interface, const struct handler_t* handler) {

}


void tls_write_alert() {

}

void tls_write_server_hello(struct tls_priv_t* priv, struct tls_client_hello_t* client_hello) {
    struct tls_out_buffer* buffer = &priv->control_block.out_buffer;
    struct tls_server_hello_t* out = buffer->buffer[buffer->offset];

    out->protocol_version = TLS_PROTOCOL_VERSION_1_2;
    out->random = 0; // TODO: Create random bytes

    uint16_t server_hello_offset = 0;

    // set session echo
    struct tls_legacy_session_t* client_legacy_session = (struct tls_legacy_session_t*) client_hello->legacy_session;
    server_hello_offset += sizeof(client_legacy_session->num_bytes) + client_legacy_session->num_bytes;
    memcpy(&out->data, client_legacy_session, server_hello_offset);

    // set cipher suites
    struct tls_ciphersuites_t* client_ciphersuites = (struct tls_ciphersuites_t*) ((uint8_t*) client_hello->ciphersuites);
    uint16_t selected_cipher_suite = tls_select_cipher_suite(client_ciphersuites, priv->supported_cipher_suites);
    struct tls_ciphersuites_t* server_cipher_suite = (struct tls_ciphersuites_t*) ((uint8_t*) &out->data + server_hello_offset);
    server_cipher_suite->num_bytes = htons(2);
    server_cipher_suite->suites[0] = selected_cipher_suite;

    // set compression methods
    *((uint8_t*) out->data + server_hello_offset) = 0;
    server_hello_offset += 1;

    // set supported versions extension. No need to check client hello as the server only supports TLS 1.3
    struct tls_supported_versions_server_hello_t version = {
        .version = TLS_PROTOCOL_VERSION_1_3
    };
    server_hello_offset += tls_set_server_hello_supported_version(&version, &out->data + server_hello_offset);
    
    // set key share extension
    struct tls_key_share_entry_t* selected_key_share = tls_select_key_share(client_hello->key_shares, priv->supported_groups,);
    server_hello_offset += tls_set_server_hello_key_share(selected_key_share, tls_generate_key_share, &out->data + server_hello_offset);
    
}

void tls_handle_handshake(struct tls_control_block_t* control_block, struct tls_handshake_t* handshake) {
    struct tls_server_hello_t* out;

    switch (control_block->state) {
        case TLS_SERVER_START:
            if(handshake->message_type != TLS_MSG_CLIENT_HELLO) {
                tls_write_alert();
                return;
            }

            tls_write_server_hello(control_block, (struct tls_client_hello_t*) handshake->data);

            control_block->state = TLS_SERVER_RECEIVED_CLIENT_HELLO;
            break;    
        default:
            break;
    }
}

uint8_t tls_handle_record(struct tls_record_t* record, struct tls_control_block_t* control_block) {
    switch(record->content_type) {
        case TLS_RECORD_CONTENT_TYPE_HANDSHAKE:
            tls_handle_handshake(control_block, record);
            return 0;
    }

    return 1;    
}

uint16_t tls_read(struct in_packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) {
    uint8_t packet_idx = packet_stack->stack_idx++;
    packet_stack->handlers[packet_idx] = handler;  
    struct tls_header_t* header = (struct tls_header_t*) packet_stack->in_buffer.packet_pointers[packet_idx];

    struct handler_t* handler = (struct handler_t*) packet_stack->handlers[packet_idx];
    struct tls_priv_t* priv = (struct tls_priv_t*) handler->priv;

    uint16_t bytes_left = 11111 - 2222 /* SHOULD BE total_in_buffer_length - total_handled_bytes */;
    
    while(bytes_left >= sizeof(struct tls_record_t)) {
        struct tls_record_t* record = &header->record;
        uint16_t record_length = ntohs(record->length);
        uint16_t full_record_length = sizeof(struct tls_record_t) + record_length;

        if(bytes_left < full_record_length) {
            NETSTACK_LOG(NETSTACK_WARNING, "TLS got an partial record.\n");   
            return 1;
        }

        // handle record        
        // IF NEEDED, DECRYPT RECORD HERE
        uint8_t fail = tls_handle_record(record, &priv->control_block);

        if(fail) {
            NETSTACK_LOG(NETSTACK_ERROR, "TLS could not handle record.\n");   
            return 2;
        }

        // we have a full record
        bytes_left -= full_record_length;

        // move header pointer to next record
        header = (struct tls_header_t*) ((uint8_t*) header + full_record_length);
    }




    return 0;
}


struct handler_t* tls_create_handler(struct handler_config_t *handler_config) {
    struct handler_t* handler = (struct handler_t*) NET_STACK_MALLOC("tls handler", sizeof(struct handler_t));	
    handler->handler_config = handler_config;

    handler->init = tls_init_handler;
    handler->close = tls_close_handler;

    handler->operations.read = tls_read;
    handler->operations.write = tls_write;

    return handler;
}


/*
    TLS skal have en socket fordi vi skal predefinere hvilken protocol der er dens next_handler. TCP 
    skal også have en socket for at kunne modtage eller sende pakker.

    Eftersom TLS gives som en next_handler til TCP, så kan vi ligeså godt lave en handler per connection. 
    Det vil sige at hver TCP connection på en socket får en next_handler som den kan anvende. Det betyder 
    at TCP socket skal have en factory method til at lave TLS sockets. TLS er ikke thread-safe, 
    da det forventes at det underliggende lag (TCP) allerede har en lås på eksekveringen

*/
