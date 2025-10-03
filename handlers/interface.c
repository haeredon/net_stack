#include "interface.h"
#include "util/memory.h"


struct interface_t* interface_create_interface(struct interface_operations_t interface_operations) {
	struct interface_t* interface = (struct interface_t*) NET_STACK_MALLOC("Interface", 
        sizeof(struct interface_t));

    interface->operations = interface_operations;

	return interface;
}