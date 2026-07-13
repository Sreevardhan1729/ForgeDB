# Redis Clone in Modern C++

A minimal Redis implementation built from scratch to learn systems programming, event-driven architecture, and network protocols. This project implements core Redis functionality using POSIX syscalls, kqueue for I/O multiplexing, the RESP2 protocol, disk persistence (RDB + AOF), and Pub/Sub.

## 📋 Current Implementation Status

✅ **Milestone 1**: File Descriptors & POSIX I/O
✅ **Milestone 2**: TCP Socket Server (Single Client)
✅ **Milestone 3**: RESP Protocol Parser & Serializer
✅ **Milestone 4**: Non-Blocking I/O (Multiple Clients)
✅ **Milestone 5**: Event Loop with kqueue
✅ **Milestone 6**: In-Memory Key-Value Store (GET/SET/DEL/PING)
✅ **Milestone 7**: TTL & Key Expiration (EXPIRE/TTL, lazy + active)
✅ **Milestone 8**: Lists (LPUSH/RPUSH/LPOP/RPOP/LRANGE)
✅ **Milestone 9**: Hashes & Sets (HSET/HGET/…, SADD/SINTER/…)
✅ **Milestone 10**: RDB Snapshots (fork + copy-on-write)
✅ **Milestone 11**: Append-Only File (AOF) persistence
✅ **Milestone 12**: Pub/Sub (SUBSCRIBE/UNSUBSCRIBE/PUBLISH)

## 🏗️ Architecture

### Main Server (`server.cpp`)
- **Event-driven**: Uses kqueue on macOS for efficient I/O multiplexing
- **Single-threaded**: Handles multiple clients concurrently without threads
- **Non-blocking sockets**: All I/O operations use `O_NONBLOCK` to prevent stalls
- **Per-client buffering**: Maintains partial message buffers for each connection
- **Periodic maintenance**: On a 1-second `kevent` timeout the loop runs active key
  expiration and `fsync`s the AOF; every 60 seconds it forks a background RDB snapshot
- **Port**: Listens on `127.0.0.1:6379` (standard Redis port)

### RESP Protocol Implementation
The server implements Redis's RESP2 (REdis Serialization Protocol):
- **Parser** (`resp_parser.cpp`): Handles bulk strings, arrays, simple strings, errors, nulls, integers
- **Serializer** (`resp_serializer.cpp`): Encodes responses back to RESP format
- **Partial message handling**: Returns `std::optional<RespMessage>` — `std::nullopt`
  buffers incomplete messages until the full command arrives

### Data Store (`datastore.cpp`)
A single `std::unordered_map<std::string, Store>` backs every key. Each `Store` holds:
- A `std::variant` value — one of `std::string`, `std::deque<std::string>` (lists),
  `std::unordered_set<std::string>` (sets), or `std::unordered_map<std::string, std::string>` (hashes)
- An optional `std::chrono::steady_clock::time_point` expiry

Type safety is enforced with a `getTypedData<T>()` helper over `std::get_if`, returning
a `WRONGTYPE` error when a command is used against the wrong value type.

### Command Handler & Dispatch (`command_handler.cpp`)
Commands are dispatched through two tables keyed by (case-insensitive) command name:
- **`dispatchTable`** — standard commands `(const RespMessage&) → RespMessage`
- **`pubsubDispatchTable`** — client-aware commands `(const RespMessage&, int clientFd) → RespMessage`
  (Pub/Sub needs the client fd to register subscriptions and push messages)

Write commands (SET, DEL, EXPIRE, L/RPUSH, L/RPOP, HSET, HDEL, SADD, SREM) are appended
to the AOF after successful execution.

Command implementations are grouped by type under `src/commands/`:
- `string_commands` — SET, GET
- `list_commands` — LPUSH, RPUSH, LPOP, RPOP, LRANGE
- `hash_commands` — HSET, HGET, HDEL, HGETALL
- `set_commands` — SADD, SREM, SMEMBERS, SISMEMBER, SINTER
- `general_commands` — PING, DEL, TTL, EXPIRE

### Persistence

**RDB Snapshots** (`rdb.cpp`)
- A custom, type-tagged binary format: each entry is `[E<abs-ms>]? <key> <type-byte> <value>`,
  where the type byte is `S` (string), `L` (list), `H` (hash), or `T` (set); TTLs are stored
  as **absolute** `system_clock` milliseconds so they survive a reboot
- Saving forks a child process; **copy-on-write** gives the child a frozen snapshot while the
  parent keeps serving. The child writes to `dump.rdb.tmp`, `fsync`s, then atomically
  `rename`s it over `dump.rdb`
- The parent reaps the child with `waitpid(..., WNOHANG)` (non-blocking) to avoid zombies

**Append-Only File** (`aof.cpp`)
- Every write command is appended to `appendonly.aof` as its raw RESP bytes
- `fsync`ed once per second (`everysec` policy) — at most ~1 second of data loss on crash
- On startup the log is replayed by feeding each command back through `handleCommand`
  (a `replaying` guard prevents re-logging during replay)

**Recovery order on startup**: load `dump.rdb` (base state) → replay `appendonly.aof`
(commands since the last snapshot).

### Pub/Sub (`pubsub.cpp`)
Two reverse-indexed maps enable O(1) lookup in both directions:
- `channel → set<clientFd>` (deliver a PUBLISH to all subscribers)
- `clientFd → set<channel>` (clean up all subscriptions when a client disconnects)

SUBSCRIBE/UNSUBSCRIBE write per-channel confirmations directly to the client;
PUBLISH pushes `["message", channel, content]` to every subscriber and returns the
delivered count to the publisher.

## 🚀 Getting Started

### Prerequisites
- **C++17** compatible compiler (GCC 7+, Clang 5+)
- **CMake 3.16+**
- **macOS** (uses POSIX syscalls and kqueue)

### Building the Project

```bash
cd redis-clone

# Configure + build into ./build
cmake -B build -S .
cmake --build build

# Or build specific targets
cmake --build build --target server     # Main Redis server
cmake --build build --target test_resp  # Unit tests
```

### Running the Server

```bash
./build/server
# Server starts on 127.0.0.1:6379
# On startup it loads dump.rdb and replays appendonly.aof (if present)
```

> **Tip:** If `./build/server` exits immediately with code 255, a previous server is
> probably still holding the port. Find and kill it:
> ```bash
> kill -9 $(lsof -ti :6379)
> ```

### Testing with redis-cli

```bash
redis-cli -p 6379

127.0.0.1:6379> PING
PONG
127.0.0.1:6379> SET mykey "Hello Redis" EX 60
OK
127.0.0.1:6379> TTL mykey
(integer) 60
127.0.0.1:6379> GET mykey
"Hello Redis"
127.0.0.1:6379> RPUSH mylist a b c
(integer) 3
127.0.0.1:6379> LRANGE mylist 0 -1
1) "a"
2) "b"
3) "c"
127.0.0.1:6379> HSET user name alice age 30
(integer) 2
127.0.0.1:6379> HGETALL user
1) "name"
2) "alice"
3) "age"
4) "30"
127.0.0.1:6379> SADD tags x y z
(integer) 3
```

### Testing Pub/Sub

```bash
# Terminal 1 — subscriber
redis-cli -p 6379 SUBSCRIBE news

# Terminal 2 — publisher
redis-cli -p 6379 PUBLISH news "hello subscribers"
# Terminal 1 receives: ["message", "news", "hello subscribers"]
```

## 🧪 Running Tests

```bash
./build/test_resp

# Or use CTest
cd build && ctest --verbose
```

Tests cover RESP parsing (bulk strings, arrays, partial data, round-trip) and
RDB serialization/deserialization round-trips for strings, lists, and TTL entries
(TTLs are compared with a tolerance since exact time points shift between calls).

## 📁 Project Structure

```
redis-clone/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── src/
│   ├── server.cpp              # Main event-driven server (kqueue loop)
│   ├── resp_parser.{cpp,h}     # RESP protocol parser
│   ├── resp_serializer.{cpp,h} # RESP protocol serializer
│   ├── resp_message.h          # RESP message type (RespMessage / RespType)
│   ├── datastore.{cpp,h}       # Global store, variant value type, expiry cleanup
│   ├── command_handler.{cpp,h} # Command dispatch tables + AOF hook
│   ├── rdb.{cpp,h}             # RDB snapshot serialize/deserialize + save/load
│   ├── aof.{cpp,h}            # Append-only file: append, fsync, replay
│   ├── pubsub.{cpp,h}         # SUBSCRIBE/UNSUBSCRIBE/PUBLISH + disconnect cleanup
│   ├── utils.{cpp,h}          # Shared helpers (errorMessage, number parsing)
│   ├── commands/              # Command implementations grouped by type
│   │   ├── string_commands.{cpp,h}
│   │   ├── list_commands.{cpp,h}
│   │   ├── hash_commands.{cpp,h}
│   │   ├── set_commands.{cpp,h}
│   │   └── general_commands.{cpp,h}
│   └── milestones/            # Standalone learning programs
│       ├── milestone1.cpp     # File descriptor basics
│       ├── milestone2.cpp     # Single-client TCP echo
│       ├── milestone4.cpp     # Non-blocking multi-client echo
│       └── milestone5.cpp     # kqueue event loop echo
├── tests/
│   └── test_resp.cpp          # Google Test suite
└── build/                     # Build artifacts (generated)
```

## 🔧 Technical Deep Dive

### Event Loop (kqueue on macOS)

1. **Setup**: Create kqueue descriptor and register the listening socket
2. **Accept**: New connections trigger `EVFILT_READ` on the listen socket
3. **Register**: Each client socket is added to kqueue with `EV_ADD`
4. **Wait**: `kevent` blocks up to a 1-second timeout for any socket to become readable
5. **Maintenance**: On timeout — expire keys, `fsync` the AOF, reap finished snapshot children,
   and start a new RDB fork if 60s have elapsed
6. **Read → Parse → Execute**: Partial reads append to a per-client buffer; complete RESP
   commands are dispatched and their responses written back
7. **Cleanup**: On disconnect (`read() == 0`), remove the client's buffer and subscriptions

### fork() + Copy-on-Write for Snapshots

`fork()` returns `0` to the child and the child's PID to the parent — the one asymmetric bit
that lets identical code split into two roles. The child inherits a logical copy of the entire
store, but the OS shares the physical pages read-only and only duplicates a page when the parent
later writes to it (**copy-on-write**). This gives the child a consistent, frozen snapshot to
serialize while the parent keeps mutating — without pausing the event loop or copying gigabytes.

### Durability: RDB vs AOF

| | RDB | AOF |
|---|---|---|
| Stores | Current state (snapshot) | Log of write commands |
| Data-loss window | Up to 60s | Up to ~1s (`everysec`) |
| File size | Small (state only) | Grows with writes |
| Recovery speed | Fast (direct load) | Slower (replay all commands) |
| Write cost | Heavy but rare (fork) | Light but constant (append) |

Running both gives fast recovery *and* a small data-loss window: RDB provides the base state,
AOF replays everything since the last snapshot.

## 📚 Learning Milestones (Standalone Programs)

The `src/milestones/` programs each isolate one building block:

```bash
./build/milestone1   # File descriptors: open/read/write/close
./build/milestone2   # Single TCP client echo, then exit
./build/milestone4   # Busy-loop polling all clients (inefficient but works)
./build/milestone5   # Efficient event-driven echo with kqueue
```

### Key Syscalls Used
- `socket()`, `bind()`, `listen()`, `accept()` — TCP server setup
- `read()` / `write()` — I/O on sockets and files (with partial-write loops)
- `fcntl()` — set non-blocking mode
- `kqueue()` / `kevent()` — event registration and waiting (macOS)
- `fork()` / `waitpid()` / `_exit()` — background snapshot process
- `open()` / `fsync()` / `rename()` / `ftruncate()` / `lseek()` — durable file persistence

## 🐛 Known Limitations

- **macOS-only**: Uses kqueue; a Linux port would need epoll
- **No write buffering**: Assumes `write()` to clients doesn't block (should use `EVFILT_WRITE`)
- **AOF grows unbounded**: No AOF rewrite/compaction yet
- **No replication**, **no AUTH**, **no maxmemory/eviction**
- **Inline commands unsupported**: Only RESP arrays are accepted (so `redis-benchmark PING_INLINE` hangs)

## 📖 Why This Project?

A **learning project** designed to teach:
1. **POSIX syscalls** — the foundation of all network programming
2. **Event-driven architecture** — how Redis serves many clients in one thread
3. **Protocol design** — parsing, framing, serialization
4. **Persistence & durability** — snapshots, write-ahead logging, atomic file swaps, fork/CoW
5. **Systems thinking** — file descriptors, kernel interfaces, performance trade-offs

**Not a production Redis replacement** — use the real Redis for anything serious!

## 📄 License

MIT License — See LICENSE file for details

---

**Built with**: Modern C++17, CMake, kqueue, Google Test, and lots of ☕
