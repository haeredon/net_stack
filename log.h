#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>

enum LOG_LEVEL { NETSTACK_INFO, NETSTACK_WARNING, NETSTACK_ERROR };

void _netstack_log(char* level, char* msg, ...);

#define NETSTACK_LOG(level, ...)					\
    _netstack_log(#level, __VA_ARGS__)

#endif // LOG_H