#ifndef HANDLERS_TCP_SOCKET_H
#define HANDLERS_TCP_SOCKET_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#include "handlers/handler.h"
#include "handlers/socket.h"
#include "handlers/ipv4/ipv4.h"
#include "handlers/tcp/tcp_shared.h"
#include "tcp_block_buffer.h"

#define TCP_SOCKET_NUM_TCB 32


struct transmission_control_block_t {
    uint32_t id;

    uint32_t remote_ipv4;
    uint32_t own_ipv4;

    uint16_t remote_port;
    uint16_t own_port;

    uint32_t send_unacknowledged; // oldest unacknowledged sequence number
    uint32_t send_next;
    uint16_t send_window; // how much storage is left remotely
    uint16_t send_urgent_pointer;
    uint32_t send_initial_sequence_num;
    uint32_t send_last_update_sequence_num; // called WL1 in RFC
    uint32_t send_last_update_acknowledgement_num; // called WL2 in RFC

    uint32_t receive_next;
    uint16_t receive_window; // how much storage is left locally
    uint16_t receive_urgent_pointer;
    uint32_t receive_initial_sequence_num;

    uint32_t fin_num; // if a fin has been sent, then this is the sequence number of it

    enum TCP_STATE state;

    bool active_mode;

    struct tcp_block_buffer_t* out_buffer;
    struct tcp_block_buffer_t* in_buffer;
    uint8_t* out_header[TCP_HEADER_MAX_SIZE];

    pthread_mutex_t lock;

    uint16_t (*state_function)(struct handler_t* handler,
        struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface);
};

// IF WE GET MORE THAN ONE SOCKET. THIS MUST BE AUTO GENERATED WITH A MACRO
struct socket_operations_t {
    uint32_t (*connect)(struct handler_t* handler, struct tcp_socket_t* socket, uint32_t remote_ip, uint16_t port);
    bool (*send)(struct tcp_socket_t* socket, uint32_t connection_id, void* buffer, uint64_t size);
    void (*on_receive)(uint8_t* data, uint64_t size); // provided by user of socket. This is a callback function. it should not hold the thread since that will uphold the entire tcp stack
    void (*on_connect)();
    void (*on_close)();
    void (*close)(struct tcp_socket_t* socket, uint32_t connection_id);
    void (*abort)(struct tcp_socket_t* socket, uint32_t connection_id);
    void (*status)(struct tcp_socket_t* socket, uint32_t connection_id);
};

struct tcp_socket_t {
    uint16_t port; // incoming port
    uint32_t ipv4; // host ip: right now tcp is tightly bound to ipv4. This field should be more generic to support both ipv4 and ipv6
    struct transmission_control_block_t* trans_control_block[TCP_SOCKET_NUM_TCB];

    pthread_mutex_t tcb_list_lock;

    struct socket_operations_t operations;

    struct socket_t socket;
    
    // the read function of the handler is not allowed to block
    // because it will block ACKs from the tcp protocol. It should 
    // return fast to acknowledge that it takes ownership of the data
    struct handler_t* next_handler;
};

struct transmission_control_block_t* tcp_create_transmission_control_block(struct handler_t* handler, struct tcp_socket_t* socket, 
        uint32_t connection_id, const struct tcp_header_t* initial_header, 
        uint32_t source_ip, enum TCP_STATE state);

struct transmission_control_block_t* tcp_get_transmission_control_block(struct tcp_socket_t* socket, uint32_t connection_id);

void tcp_delete_socket(struct handler_t* handler, struct tcp_socket_t* socket) ;

void tcp_delete_transmission_control_block(struct handler_t* handler, struct tcp_socket_t* socket, uint32_t connection_id);

void tcp_add_transmission_control_block(struct tcp_socket_t* socket, struct transmission_control_block_t* transmission_control_block);

bool tcp_add_socket(struct handler_t* handler, struct tcp_socket_t* socket);

struct tcp_socket_t* tcp_get_socket(const struct handler_t* handler, uint32_t ipv4, uint16_t port);

struct tcp_socket_t* tcp_create_socket(struct handler_t* next_handler, uint16_t port, uint32_t ipv4, 
    void (*on_receive)(uint8_t* data, uint64_t size), void (*on_connect)(), void (*on_close)());


#endif // HANDLERS_TCP_SOCKET_H