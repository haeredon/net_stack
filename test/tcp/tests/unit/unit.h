#ifndef TEST_TCP_TEST_UNIT_H
#define TEST_TCP_TEST_UNIT_H

#include "test/common.h"
#include "handlers/handler.h"
#include "test/tcp/tests/unit/unit_packages.h"

bool tcp_test_checksum(struct handler_t* handler, struct test_config_t* config);

#endif // TEST_TCP_TEST_UNIT_H