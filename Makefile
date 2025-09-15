# binary name
APP = net_stack

SRCS-OVERRIDES-TEST := test/tcp/overrides.c
SRCS-TEST := test/main.c test/utility.c test/ipv4/test.c test/common.c test/tcp/test.c  test/tcp/utility.c test/tcp/overrides.c test/tcp/tests/download_1/download_1.c test/tcp/tests/active_mode/active_mode.c test/arp/test.c
OBJS-TEST = $(SRCS-TEST:.c=.o)
INCLUDES-TEST := -I ./ 

#SRCS-DEV := main_dev.c worker.c execution_pool.c

SRCS-HANDLERS := handlers/ethernet/ethernet.c handlers/handler.c handlers/protocol_map.c handlers/ipv4/ipv4.c handlers/custom/custom.c handlers/tcp/tcp.c handlers/tcp/tcp_block_buffer.c handlers/tcp/socket.c handlers/tcp/tcp_shared.c handlers/tcp/tcp_states.c handlers/arp/arp.c
SRCS-UTILITY := util/array.c util/b_tree.c util/log.c
OBJS = $(SRCS-HANDLERS:.c=.o) $(SRCS-UTILITY:.c=.o) 
INCLUDES := -I ./

BUILD-DIR := build
OBJ-DIR = $(BUILD-DIR)/obj

PKGCONF ?= pkg-config

# Build using pkg-config variables if possible
ifneq ($(shell $(PKGCONF) --exists libdpdk && echo 0),0)
$(error "no installation of DPDK found")
endif

# all: shared

# .PHONY: shared
# shared: build/$(APP)-shared
# 	ln -sf $(APP)-shared build/$(APP)

test: build/$(APP)-test

# dev: build/$(APP)-dev
	
PC_FILE := $(shell $(PKGCONF) --path libdpdk 2>/dev/null)
CFLAGS += -g $(shell $(PKGCONF) --cflags libdpdk)
# Add flag to allow experimental API as l2fwd uses rte_ethdev_set_ptype API
CFLAGS += -DALLOW_EXPERIMENTAL_API
LDFLAGS_SHARED = $(shell $(PKGCONF) --libs libdpdk)



############################### RUNTIME ######################################
# build/$(APP)-shared: build/libhandler.so $(SRCS-y) Makefile $(PC_FILE) | build
# 	$(CC) -L/home/skod/net_stack/build $(CFLAGS) $(SRCS-y) $(INCLUDES)  -o $@ $(LDFLAGS) $(LDFLAGS_SHARED) -lhandler

# build/$(APP)-dev: build/libhandler.so $(SRCS-DEV) | build	
# 	$(CC) -L/home/skod/net_stack/build $(CFLAGS) $(SRCS-DEV) $(INCLUDES-TEST) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED) -lhandler

############################### NET STACK HANDLERS ###########################
$(BUILD-DIR)/libhandler.so: $(addprefix $(OBJ-DIR)/, $(OBJS))
	mkdir -p $(BUILD-DIR)
	$(CC) $(CFLAGS) -fpic -shared $(addprefix $(OBJ-DIR)/,$(OBJS)) $(INCLUDES) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)

$(OBJ-DIR)/handlers/%.o: handlers/%.c 
	mkdir -p $(dir $@)
	$(CC) -L$(BUILD-DIR) $(CFLAGS) -fpic $(INCLUDES) -c $< -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)

$(OBJ-DIR)/util/%.o: util/%.c 
	mkdir -p $(dir $@)
	$(CC) -L$(BUILD-DIR) $(CFLAGS) -fpic $(INCLUDES) -c $< -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)

############################### TEST ##########################################
$(BUILD-DIR)/$(APP)-test: $(addprefix $(OBJ-DIR)/, $(OBJS-TEST)) $(BUILD-DIR)/libhandler.so $(BUILD-DIR)/libtestoverrides.so | $(BUILD-DIR)
	mkdir -p $(BUILD-DIR)
	$(CC) -L$(BUILD-DIR) $(CFLAGS) $(addprefix $(OBJ-DIR)/,$(OBJS-TEST)) $(INCLUDES-TEST) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED) -lhandler -ltestoverrides

$(OBJ-DIR)/test/%.o: test/%.c $(BUILD-DIR)/libhandler.so $(BUILD-DIR)/libtestoverrides.so | $(BUILD-DIR)
	mkdir -p $(dir $@)
	$(CC) -L$(BUILD-DIR) $(CFLAGS) $(INCLUDES-TEST) -c $< -o $@ -lhandler -ltestoverrides

$(BUILD-DIR)/libtestoverrides.so: $(SRCS-OVERRIDES-TEST) | $(BUILD-DIR)
	mkdir -p $(BUILD-DIR)
	$(CC) $(CFLAGS) -fpic -shared $(SRCS-OVERRIDES-TEST) $(INCLUDES-TEST) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)


############################### STANDARD TARGETS ###############################
all: $(BUILD-DIR)/$(APP)-test 

.PHONY: clean test
clean:
	test -d build && rm -rf build


