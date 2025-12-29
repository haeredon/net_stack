#ifndef HANDLER_HANDLERS_DHCP_H
#define HANDLER_HANDLERS_DHCP_H

#include "handlers/handler.h"

#define DHCP_OPCODE_BOOTREQUEST 1
#define DHCP_OPCODE_BOOTREPLY 2

#define DHCP_HTYPE_ETHERNET 1
#define DHCP_HLEN_ETHERNET 6

#define DHCP_FLAGS_BROADCAST 0x8000 // big-endian

#define DHCP_OPTION_MESSAGE_TYPE 53
#define DHCP_OPTION_REQUESTED_IP 50
#define DHCP_OPTION_SERVER_IDENTIFIER 54
#define DHCP_OPTION_PARAMETER_REQUEST_LIST 55
#define DHCP_OPTION_END 255

#define DHCP_MESSAGE_TYPE_DISCOVER 1
#define DHCP_MESSAGE_TYPE_OFFER 2
#define DHCP_MESSAGE_TYPE_REQUEST 3
#define DHCP_MESSAGE_TYPE_DECLINE 4
#define DHCP_MESSAGE_TYPE_ACK 5
#define DHCP_MESSAGE_TYPE_NAK 6
#define DHCP_MESSAGE_TYPE_RELEASE 7
#define DHCP_MESSAGE_TYPE_INFORM 8

#define DHCP_MAGIC_COOKIE 0x63825363 // big-endian
#define DHCP_OPTIONS_OFFSET 240
#define DHCP_MAX_OPTIONS_SIZE 312
#define DHCP_MIN_MESSAGE_SIZE 300
#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

struct dhcp_priv_t {
    int dummy;
};

struct dhcp_write_args_t {
    int dummy;
};

struct dhcp_header_t {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128];
    uint8_t options[312]; // variable length
} __attribute__((packed));

struct handler_t* dhcp_create_handler(struct handler_config_t *handler_config);

#endif // HANDLER_HANDLERS_DHCP_H

