#ifndef DPDK_WRITE_H
#define DPDK_WRITE_H

#include <stdint.h>

#include "handlers/handler.h"


int64_t dpdk_write_write(struct out_buffer_t* out_buffer);

#endif // DPDK_WRITE_H