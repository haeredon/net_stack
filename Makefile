# binary name
APP = net_stack

# all source are stored in SRCS-y
SRCS-TEST := test/main.c test/pcapng.c test/test_suite.c
INCLUDES-TEST := -I ./ -I ./test
SRCS-y := main.c worker.c 
SRCS-HANDLERS := handlers/pcapng.c handlers/ethernet.c handlers/handler.c handlers/arp.c handlers/protocol_map.c
SRCS-UTILITY := util/array.c
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

test: build/libhandler.so build/$(APP)-test
	
PC_FILE := $(shell $(PKGCONF) --path libdpdk 2>/dev/null)
CFLAGS += -g $(shell $(PKGCONF) --cflags libdpdk)
# Add flag to allow experimental API as l2fwd uses rte_ethdev_set_ptype API
CFLAGS += -DALLOW_EXPERIMENTAL_API
LDFLAGS_SHARED = $(shell $(PKGCONF) --libs libdpdk)

build/$(APP)-shared: build/libhandler.so $(SRCS-y) Makefile $(PC_FILE) | build
	$(CC) -L/home/skod/net_stack/build $(CFLAGS) $(SRCS-y) $(INCLUDES)  -o $@ $(LDFLAGS) $(LDFLAGS_SHARED) -lhandler

build/$(APP)-test: build/libhandler.so $(SRCS-TEST) | build
	$(CC) -L/home/skod/net_stack/build $(CFLAGS) $(SRCS-TEST) $(INCLUDES-TEST) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED) -lhandler

build/libhandler.so: $(SRCS-HANDLERS) $(SRCS-LOG) $(SRCS-UTILITY) | build
	$(CC) $(CFLAGS) -fpic -shared $(SRCS-HANDLERS) $(SRCS-LOG) $(SRCS-UTILITY) $(INCLUDES) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)

build:
	@mkdir -p $@

.PHONY: clean test
clean:
	rm -f build/$(APP) build/$(APP)-shared build/$(APP)-test build/libhandler.so
	test -d build && rmdir -p build || true

