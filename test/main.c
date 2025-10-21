#include "handlers/handler.h"
#include "handlers/protocol_map.h"
#include "handlers/ipv4/ipv4.h"
#include "test/ipv4/test.h"
#include "test/tcp/test.h"
#include "test/arp/test.h"

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


#include "handlers/tcp/tcp_shared.h"

int main(int argc, char **argv) {     
    printf("Starting tests\n\n");     
    
    tcp_tests_start();

    arp_tests_start();
}



