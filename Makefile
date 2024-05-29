# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2010-2014 Intel Corporation

# binary name
APP = l2fwd

# all source are stored in SRCS-y
TEST_SRCS-y := test/
SRCS-y := main.c worker.c 
SRCS-HANDLERS := handlers/pcapng.c handlers/ethernet.c handlers/handler.c handlers/arp.c handlers/protocol_map.c
INCLUDES := -I ./

PKGCONF ?= pkg-config

# Build using pkg-config variables if possible
ifneq ($(shell $(PKGCONF) --exists libdpdk && echo 0),0)
$(error "no installation of DPDK found")
endif

all: shared
.PHONY: shared 
shared: build/$(APP)-shared
	ln -sf $(APP)-shared build/$(APP)

PC_FILE := $(shell $(PKGCONF) --path libdpdk 2>/dev/null)
CFLAGS += -g $(shell $(PKGCONF) --cflags libdpdk)
# Add flag to allow experimental API as l2fwd uses rte_ethdev_set_ptype API
CFLAGS += -DALLOW_EXPERIMENTAL_API
LDFLAGS_SHARED = $(shell $(PKGCONF) --libs libdpdk)

build/$(APP)-shared: build/libhandler.so $(SRCS-y) Makefile $(PC_FILE) | build
	$(CC) -L/home/skod/net_stack/build $(CFLAGS) $(SRCS-y) $(INCLUDES)  -o $@ $(LDFLAGS) $(LDFLAGS_SHARED) -lhandler

build/libhandler.so: $(SRCS-HANDLERS) | build
	$(CC) $(CFLAGS) -fpic -shared $(SRCS-HANDLERS) $(INCLUDES) -o build/libhandler.so $(LDFLAGS) $(LDFLAGS_SHARED)




# build/$(APP)-test: build/$(APP)-shared 
# 	$(CC) $(CFLAGS) $(SRCS-y) $(INCLUDES)  -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)


# test: main.o pcapng.o
# 	gcc -o test -g  main.o pcapng.o 

# main.o : main.c
# 	gcc -g -c main.c  

# pcapng.o : pcapng.c pcapng.h
# 	gcc -g -c pcapng.c -g 


build:
	@mkdir -p $@



.PHONY: clean test
clean:
	rm -f build/$(APP) build/$(APP)-static build/$(APP)-shared
	test -d build && rmdir -p build || true

test: 
	$(MAKE) -C test