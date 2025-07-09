# binary name
APP = net_stack

# all source are stored in SRCS-y
SRCS-OVERRIDES-TEST := test/tcp/overrides.c
SRCS-TEST := test/main.c test/ipv4/test.c test/common.c test/tcp/test.c  test/tcp/utility.c test/tcp/overrides.c test/tcp/tests/download_1/download_1.c
SRCS-DEV := main_dev.c
INCLUDES-TEST := -I ./ 
SRCS-y := main.c worker.c 
SRCS-HANDLERS := handlers/ethernet/ethernet.c handlers/handler.c handlers/protocol_map.c handlers/ipv4/ipv4.c handlers/custom/custom.c handlers/tcp/tcp.c handlers/tcp/tcp_block_buffer.c handlers/tcp/socket.c handlers/tcp/tcp_shared.c handlers/tcp/tcp_states.c
SRCS-UTILITY := util/array.c util/b_tree.c
SRCS-LOG := log.c
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

test: build/$(APP)-test

dev: build/$(APP)-dev
	
PC_FILE := $(shell $(PKGCONF) --path libdpdk 2>/dev/null)
CFLAGS += -g $(shell $(PKGCONF) --cflags libdpdk)
# Add flag to allow experimental API as l2fwd uses rte_ethdev_set_ptype API
CFLAGS += -DALLOW_EXPERIMENTAL_API
LDFLAGS_SHARED = $(shell $(PKGCONF) --libs libdpdk)

build/$(APP)-shared: build/libhandler.so $(SRCS-y) Makefile $(PC_FILE) | build
	$(CC) -L/home/skod/net_stack/build $(CFLAGS) $(SRCS-y) $(INCLUDES)  -o $@ $(LDFLAGS) $(LDFLAGS_SHARED) -lhandler

build/$(APP)-dev: build/libhandler.so $(SRCS-DEV) | build	
	$(CC) -L/home/skod/net_stack/build $(CFLAGS) $(SRCS-DEV) $(INCLUDES-TEST) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED) -lhandler

build/$(APP)-test: build/libtestoverrides.so build/libhandler.so $(SRCS-TEST) | build	
	$(CC) -L/home/skod/net_stack/build $(CFLAGS) $(SRCS-TEST) $(INCLUDES-TEST) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED) -lhandler -ltestoverrides

build/libtestoverrides.so: $(SRCS-OVERRIDES-TEST) | build
	$(CC) $(CFLAGS) -fpic -shared $(SRCS-OVERRIDES-TEST) $(INCLUDES-TEST) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)

build/libhandler.so: $(SRCS-HANDLERS) $(SRCS-LOG) $(SRCS-UTILITY) | build
	$(CC) $(CFLAGS) -fpic -shared $(SRCS-HANDLERS) $(SRCS-LOG) $(SRCS-UTILITY) $(INCLUDES) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)

build:
	@mkdir -p $@

.PHONY: clean test
clean:
	rm -f build/$(APP) build/$(APP)-shared build/$(APP)-test build/$(APP)-dev build/libhandler.so
	test -d build && rmdir -p build || true

