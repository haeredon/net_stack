#include "handlers/dhcp/dhcp.h"

#include "util/log.h"


/*
 * Dynamic Host Configuration Protocol (DHCP).
 * Specification: RFC 2131
 * 
 */

struct dhcp_options* parse_options(const uint8_t* options, struct dhcp_options* dhcp_options) {
    uint16_t idx = 4; // skip magic cookie

    while(idx < DHCP_MAX_OPTIONS_SIZE) {
        uint8_t option_type = options[idx];
        struct tlv_t* tlv_option = (struct tlv_t*) &options[idx];

        if(option_type == DHCP_OPTION_END) {
            dhcp_options->end = (struct tlv_t*) &options[idx];
            break;
        }

        switch (option_type) {
            case DHCP_OPTION_MESSAGE_TYPE:
                dhcp_options->dhcp_message_type = tlv_option;
                break;
            case DHCP_OPTION_REQUESTED_IP:
                dhcp_options->requested_ip = tlv_option;
                break;
            case DHCP_OPTION_SERVER_IDENTIFIER:
                dhcp_options->server_identifier = tlv_option;
                break;
            case DHCP_OPTION_PARAMETER_REQUEST_LIST:
                dhcp_options->parameter_request_list = tlv_option;
                break;
            default:
                NETSTACK_LOG(NETSTACK_WARNING, "DHCP option type %d not specifically handled.\n", option_type);          
                break;
        }

        if(tlv_option->length == 0) {
            NETSTACK_LOG(NETSTACK_ERROR, "DHCP option with length 0 found. Stopping option parsing to avoid infinite loop.\n");          
            break;
        }

        idx += 2 /* type and length */ + tlv_option->length;
    }

    return dhcp_options;    
}


uint16_t handle_reply(struct dhcp_header_t* packet) {    
    switch (packet->)
    {
    case constant expression:
        /* code */
        break;
    
    default:
        break;
    }

    return 0;
}

      
void dhcp_close_handler(struct handler_t* handler) {
    struct dhcp_priv_t* private = (struct dhcp_priv_t*) handler->priv;    
    NET_STACK_FREE(private);
}

void dhcp_init_handler(struct handler_t* handler, void* priv_config) {
    struct dhcp_priv_t* dhcp_priv = (struct dhcp_priv_t*) NET_STACK_MALLOC("dhcp handler private data", sizeof(struct dhcp_priv_t)); 
    handler->priv = (void*) dhcp_priv;
}

bool dhcp_write(struct out_packet_stack_t* packet_stack, struct interface_t* interface, const struct handler_t* handler) {
}

uint16_t dhcp_read(struct in_packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) {
    uint8_t packet_idx = packet_stack->stack_idx++;
    packet_stack->handlers[packet_idx] = handler;  
    struct dhcp_header_t* packet = (struct dhcp_header_t*) packet_stack->in_buffer.packet_pointers[packet_idx];

    if(packet->htype != DHCP_HTYPE_ETHERNET) {
        NETSTACK_LOG(NETSTACK_INFO, "DHCP with unsupported hardware type: Dropping package.\n");          
        return 1;
    }

    if(packet->hlen != DHCP_HLEN_ETHERNET) {
        NETSTACK_LOG(NETSTACK_INFO, "DHCP with unsupported hardware address length: Dropping package.\n");          
        return 2;
    }

    if(packet->op != DHCP_OPCODE_BOOTREQUEST) {        
        // TODO: implement DHCP server functionality
    } else if(packet->op != DHCP_OPCODE_BOOTREPLY) {        
        return handle_reply(packet);
    } else {
        return 3;
    }

    return 0;
}


struct handler_t* dhcp_create_handler(struct handler_config_t *handler_config) {
    struct handler_t* handler = (struct handler_t*) NET_STACK_MALLOC("dhcp handler", sizeof(struct handler_t));	
    handler->handler_config = handler_config;

    handler->init = dhcp_init_handler;
    handler->close = dhcp_close_handler;

    handler->operations.read = dhcp_read;
    handler->operations.write = dhcp_write;

    return handler;
}