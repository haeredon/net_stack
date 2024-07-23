#include "log.h"

void _netstack_log(char* level, char* msg, ...) {
    va_list variadic_arguments;
	va_start(variadic_arguments, msg);
	printf(msg, variadic_arguments);
	va_end(variadic_arguments);    
}