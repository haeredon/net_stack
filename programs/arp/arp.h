#ifndef PROGRAMS_ARP_ARP_H
#define PROGRAMS_ARP_ARP_H

#include <stdint.h>

#include "handlers/handler.h"
#include "net_stack.h"

pthread_t arp_program_start(const struct net_stack_app* net_stack);


#endif // PROGRAMS_ARP_ARP_H