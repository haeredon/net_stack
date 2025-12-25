<!-- .github/copilot-instructions.md - guidance for AI coding agents working on this repo -->

# Copilot instructions for net_stack

This repository implements a small network stack built around DPDK. The goal of
these notes is to help an AI coding agent be immediately productive by
highlighting the architecture, developer workflows, patterns, and concrete
examples from the codebase.

1) Big-picture architecture
- **DPDK-driven packet pipeline:** The process boots with `rte_eal_init` in
  `main.c` and uses DPDK rings and mbufs. `main.c` configures ports, creates a
  mempool (`pktmbuf_pool`), launches worker execution contexts, and runs an
  offloader loop that distributes received mbufs to worker rings.
- **Handlers as the protocol layer:** Protocol logic lives under `handlers/`.
  Handlers follow a plugin-like pattern: each handler exposes a `struct
  handler_t` with `init`, `close` and `operations` (see
  `handlers/handler.h`). Example: `handlers/ethernet/ethernet.c` creates an
  ethernet root handler; `handlers/tcp/*` implements TCP state machine logic.
- **Shared handler library:** Handlers are compiled into a shared object
  `build/libhandler.so` (see `Makefile`). The main binary links against this
  library at runtime.
- **I/O offloading and write queue:** Writes are queued through
  `dpdk_write_write_queue` and serviced in the offloader in `main.c` and
  `dpdk/write.c`.

2) Key directories and files (quick map)
- `main.c` — application lifecycle, port initialization, EAL integration,
  worker/offloader startup.
- `handlers/` — protocol implementations (ethernet, arp, ipv4, tcp, custom,
  etc.). Look at `handlers/handler.h` for the handler interface.
- `dpdk/` — DPDK helper code (`packet.c`, `write.c`). The ring `dpdk_write_write_queue`
  and `dpdk_write_tx_buffer[]` are defined in `dpdk/write.c`.
- `server.c` — exposes a simple TCP server API wired into the TCP handler
  (`server_start(...)` is invoked from `main.c`).
- `test/` — unit/integration tests and embedded pcapng packages used by tests.

3) Build, test and runtime workflows (concrete commands)
- Build app: `make app` → produces `build/net_stack`.
- Build tests: `make test` → produces `build/net_stack-test` and helper
  shared libs like `build/libtestoverrides.so`.
- Full test build: `make all` → builds the test binary by default.
- Clean: `make clean` removes `build/`.
- Requirements: The `Makefile` uses `pkg-config --exists libdpdk` and will
  error if DPDK is not found. Compilation uses `pkg-config --cflags/--libs
  libdpdk` and builds `libhandler.so` with `-fpic -shared`.
- Runtime: `main.c` calls `rte_eal_init(argc, argv)`. The application expects
  DPDK EAL arguments and therefore should be started with appropriate EAL
  flags for your environment (this repo does not hardcode EAL args).

4) Project-specific conventions and patterns
- **Handler interface:** Implementations return a `struct handler_t*` via
  `xxx_create_handler(...)`. The struct contains an `operations` field and
  `init/close` function pointers. See `handlers/handler.h` for the full
  contract and `handlers/ethernet/ethernet.c` for a concrete example.
- **Memory and logging helpers:** The repo uses `NET_STACK_MALLOC` and
  logging macros in `util/log.h`. Prefer these wrappers for consistency.
- **Shared library pattern:** Protocol handlers are built as position
  independent code and linked into `build/libhandler.so`. If you add new
  handler sources, update `SRCS-HANDLERS` in the `Makefile` so they're
  included in the shared library build.
- **RTE primitives for IPC inside process:** Rings (`rte_ring`) are used to
  exchange packets between offloader and worker contexts. Search for
  `rte_ring_create`, `rte_ring_enqueue`, and `rte_ring_dequeue_bulk` for
  producers/consumers.

5) Tests and test data
- Tests live under `test/`. Several tests embed pcapng files (e.g.
  `test/tcp/tests/download_1/download_1.pcapng`) and helper code in
  `test/utility.c`.
- The test binary links against `-lhandler` and `-ltestoverrides` at build
  time (see `Makefile`), so test runs exercise the same handler implementation
  as the app.

6) Integration points / external dependencies
- **DPDK** via `pkg-config libdpdk` (compile- and run-time EAL usage).
- The code relies on DPDK EAL APIs (`rte_eal_init`, `rte_mbuf`, `rte_ring`,
  `rte_eth_*`). If you change EAL usage, consider compatibility with the
  `Makefile`'s pkg-config flags.

7) Example fixes and common areas to inspect
- If adding a handler, follow `handlers/handler.h` contract and update
  `Makefile` `SRCS-HANDLERS` so it's included into `libhandler.so`.
- Networking concurrency issues often involve ring sizing and mbuf
  allocation — check uses of `pktmbuf_pool`, `rte_pktmbuf_alloc_bulk` and
  the offloader/worker ring code in `main.c`.
- Tests use embedded pcapng fixtures; if tests fail, inspect `test/*` and the
  corresponding fixture under `test/*/*.pcapng`.

8) How to ask me (the AI) for help in this repo
- Use exact filenames when requesting changes. Example: “Update
  `handlers/tcp/tcp_states.c` to add transition X.”
- For build issues, include the exact `make` command and the `pkg-config`
  output for `libdpdk` if available (`pkg-config --cflags --libs libdpdk`).
- For runtime bugs, provide the `rte_eal_init` args used and any DPDK
  environment (kernel driver, hugepages, sudo/root) details.

If anything here is unclear or you want more detail on a particular area
(handlers, build, or tests), tell me which files you want me to expand on and
I will update this document.
