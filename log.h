#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>

enum LOG_LEVEL { NETSTACK_INFO, NETSTACK_WARNING, NETSTACK_ERROR };

void _netstack_log(char* level, char* msg, ...) {
    va_list variadic_arguments;
	va_start(variadic_arguments, msg);
	printf(msg, variadic_arguments);
	va_end(variadic_arguments);    
}

#define NETSTACK_LOG(level, ...)					\
    _netstack_log(#level, __VA_ARGS__)

#endif // LOG_H