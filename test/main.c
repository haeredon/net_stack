#include "pcapng.h"
 
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_packet.h>

// struct test_socket_t {
//     int (*send)(struct test_socket_t* test_socket, const void* buffer, uint32_t size);
//     int (*receive)(struct test_socket_t* test_socket);
//     int (*close)(struct test_socket_t* test_socket);

//     int fd;
//     struct sockaddr_ll socket_address;
// };

// int test_socket_send(struct test_socket_t* test_socket, const void* buffer, uint32_t size) {
//     int ret = sendto(test_socket->fd, 
//         buffer, 
//         size, 
//         0, 
//         (struct sockaddr*) &test_socket->socket_address, 
//         sizeof(struct sockaddr_ll)
//     );

//     if (ret) {
//         printf("Could not write packet to socket. errno: %d\n", errno);        
//     }
//     return ret;
// }

// int test_socket_receive(struct test_socket_t* test_socket) {

// }

// int test_socket_close(struct test_socket_t* test_socket) {
//     int ret = close(test_socket->fd);
//     if(ret) {
//         printf("Failed to close socket. errno: %d\n", errno);		
//     }    
//     return ret;
// }



// struct test_socket_t* create_test_socket(char interface_name[]) {
//     int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

//     if(fd == -1) {
//         printf("Failed to open socket. errno: %d\n", errno);		
//         return 0;
//     }

//     struct ifreq interface_request;
//     memset(&interface_request, 0, sizeof(struct ifreq));
//     strncpy(interface_request.ifr_name, interface_name, IFNAMSIZ);


//     if(ioctl(fd, SIOCGIFINDEX, &interface_request) < 0) {
//         printf("Could not get interface id for %s with request value %d", interface_name, SIOCGIFINDEX);
//         return 0;
//     }

//     struct test_socket_t* test_socket = (struct test_socket_t*) malloc(sizeof(struct test_socket_t));
    
//     test_socket->fd = fd;
 
//     memset(&test_socket->socket_address, 0, sizeof(struct sockaddr_ll));
//     test_socket->socket_address.sll_ifindex = interface_request.ifr_ifindex;
//     test_socket->socket_address.sll_halen = ETH_ALEN;
//     test_socket->socket_address.sll_protocol = 0; // at the moment we don't want to receive. Later make it htons(ETH_P_ALL) to receive; 
//     test_socket->socket_address.sll_family = AF_PACKET;

//     test_socket->send = test_socket_send;
//     test_socket->receive = test_socket_receive;
//     test_socket->close = test_socket_close;

//     return test_socket;
// }



/***********************************************/

struct test_t {
    uint8_t (*test)(struct test_t* test);
    int (*init)(struct test_t* test);
    void (*end)(struct test_t* test);

    struct pcapng_reader_t* reader;
    struct test_socket_t* socket;
};


uint8_t arp_test(struct test_t* test) {
    static const int MAX_BUFFER_SIZE = 0xFFFF;
    uint8_t* req_buffer = (uint8_t*) malloc(MAX_BUFFER_SIZE);
    uint8_t* res_buffer = (uint8_t*) malloc(MAX_BUFFER_SIZE);
    struct pcapng_reader_t* reader = test->reader;

    do {
        if(reader->read_block(reader, req_buffer, MAX_BUFFER_SIZE) == -1)  {
            printf("FAIL.\n");	
        }
        
        if(packet_is_type(req_buffer, PCAPNG_ENHANCED_BLOCK)) {
            printf("Enhanced Block!\n");    

            struct packet_enchanced_block_t* block = (struct packet_enchanced_block_t*) req_buffer;

            
        }
        printf("Do test!\n");
        // send request

        // wait for response
        // check response 

        // loop
    } while(reader->has_more_headers(reader));
}

int arp_init(struct test_t* test) {
    test->reader = create_pcapng_reader();        

    if(test->reader->init(test->reader, "test.pcapng")) {
        test->reader->close(test->reader);
        return -1;
    };    

    return 0;
}

void arp_end(struct test_t* test) {
    test->reader->close(test->reader);
}

struct test_t* create_arp_test_suite() {
    struct test_t* test = (struct test_t*) malloc(sizeof(struct test_t));

    test->init = arp_init;
    test->end = arp_end;
    test->test = arp_test;    
}

/**********************************************/

int main(int argc, char **argv) {
    struct test_t* arp_test_suite = create_arp_test_suite();

    if(arp_test_suite->init(arp_test_suite)) {
        return -1;
    }

    arp_test_suite->test(arp_test_suite);


    arp_test_suite->end(arp_test_suite);
}
