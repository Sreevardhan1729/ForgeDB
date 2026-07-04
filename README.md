# Redis Clone in Modern C++

A minimal Redis implementation built from scratch to learn systems programming, event-driven architecture, and network protocols. This project implements core Redis functionality using POSIX syscalls, kqueue for IO multiplexing, and the RESP2 protocol.

## 📋 Current Implementation Status

✅ **Milestone 1**: File Descriptors & POSIX I/O
✅ **Milestone 2**: TCP Socket Server (Single Client)
✅ **Milestone 3**: RESP Protocol Parser & Serializer
✅ **Milestone 4**: Non-Blocking I/O (Multiple Clients)
✅ **Milestone 5**: Event Loop with kqueue
✅ **Milestone 6**: In-Memory Key-Value Store (GET/SET/DEL/PING)
⏳ **Milestone 7+**: TTL, Lists, Sets, Persistence, Pub/Sub (Planned)

## 🏗️ Architecture

### Main Server (`server.cpp`)
- **Event-driven**: Uses kqueue on macOS for efficient I/O multiplexing
- **Single-threaded**: Handles multiple clients concurrently without threads
- **Non-blocking sockets**: All I/O operations use `O_NONBLOCK` to prevent stalls
- **Per-client buffering**: Maintains partial message buffers for each connection
- **Port**: Listens on `127.0.0.1:6379` (standard Redis port)

### RESP Protocol Implementation
The server implements Redis's RESP2 (REdis Serialization Protocol):
- **Parser** (`resp_parser.cpp`): Handles bulk strings, arrays, simple strings, errors, nulls, integers
- **Serializer** (`resp_serializer.cpp`): Encodes responses back to RESP format
- **Partial message handling**: Buffers incomplete messages until full command arrives

### Command Handler
Command dispatch system with support for:
- **PING** - Health check (returns "PONG" or echoes argument)
- **SET key value** - Store a string value
- **GET key** - Retrieve a value (returns null if not found)
- **DEL key [key ...]** - Delete one or more keys (returns count deleted)

All commands are case-insensitive and stored in-memory using `std::unordered_map`.

## 🚀 Getting Started

### Prerequisites
- **C++17** compatible compiler (GCC 7+, Clang 5+, or MSVC 2017+)
- **CMake 3.16+**
- **macOS/Linux** (uses POSIX syscalls and kqueue/epoll)

### Building the Project

```bash
# Clone and navigate to the project
cd redis-clone

# Create build directory
mkdir -p build && cd build

# Configure with CMake
cmake ..

# Build all targets
make

# Alternatively, build specific targets:
make server        # Main Redis server
make test_resp     # Unit tests
make milestone1    # File descriptor demo
make milestone2    # Single-client TCP echo
make milestone4    # Multi-client non-blocking echo
make milestone5    # kqueue event loop echo
```

### Running the Server

```bash
# From the build directory
./server

# Server will start on 127.0.0.1:6379
# No output means it's running successfully
```

### Testing with redis-cli

```bash
# In another terminal, connect with official Redis CLI
redis-cli -p 6379

# Try some commands
127.0.0.1:6379> PING
PONG

127.0.0.1:6379> SET mykey "Hello Redis"
OK

127.0.0.1:6379> GET mykey
"Hello Redis"

127.0.0.1:6379> DEL mykey
(integer) 1

127.0.0.1:6379> GET mykey
(nil)
```

### Testing with telnet (Raw RESP)

```bash
telnet localhost 6379

# Type RESP commands manually:
*1\r\n$4\r\nPING\r\n
# Response: +PONG\r\n

*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n
# Response: +OK\r\n

*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n
# Response: $3\r\nbar\r\n
```

## 🧪 Running Tests

```bash
# From build directory
./test_resp

# Or use CTest
ctest --verbose

# Tests cover:
# - Bulk string parsing
# - Array parsing
# - Partial message handling
# - Round-trip serialization
```

## 📁 Project Structure

```
redis-clone/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── src/
│   ├── server.cpp          # Main event-driven server
│   ├── milestone1.cpp      # File descriptor basics
│   ├── milestone2.cpp      # Single-client TCP echo
│   ├── milestone4.cpp      # Non-blocking multi-client echo
│   ├── milestone5.cpp      # kqueue event loop echo
│   ├── resp_parser.cpp     # RESP protocol parser
│   ├── resp_parser.h
│   ├── resp_serializer.cpp # RESP protocol serializer
│   ├── resp_serializer.h
│   ├── resp_message.h      # RESP message types
│   ├── command_handler.cpp # Redis command implementations
│   └── command_handler.h
├── tests/
│   └── test_resp.cpp       # Google Test suite
└── build/                  # Build artifacts (generated)
```

## 🔧 Technical Deep Dive

### Event Loop (kqueue on macOS)

The server uses kqueue for IO multiplexing:

1. **Setup**: Create kqueue descriptor and register listening socket
2. **Accept**: New connections trigger `EVFILT_READ` on listen socket
3. **Register**: Each client socket gets added to kqueue with `EV_ADD`
4. **Process**: Event loop waits for any socket to become readable
5. **Read**: Partial reads append to per-client buffer
6. **Parse**: Attempt to parse complete RESP messages
7. **Execute**: Dispatch command and send response
8. **Cleanup**: Remove closed connections from kqueue

### Non-Blocking I/O

All sockets use `fcntl(fd, F_SETFL, O_NONBLOCK)`:
- **Accept**: May return `-1` with `errno == EAGAIN` if no clients pending
- **Read**: Returns immediately even if no data available
- **Write**: May partially write (not handled yet - TODO)

### RESP Protocol

The parser handles streaming data:
```
*3\r\n           # Array of 3 elements
$3\r\n           # Bulk string of length 3
SET\r\n          # String data
$5\r\n           # Bulk string of length 5
mykey\r\n        # String data
$7\r\n           # Bulk string of length 7
myvalue\r\n      # String data
```

Returns `std::optional<RespMessage>`:
- `std::nullopt` if incomplete (need more bytes)
- `RespMessage` if successfully parsed

## 📚 Learning Resources

### Milestones (Standalone Programs)
Run these to understand each building block:

```bash
# Milestone 1: File descriptors
./milestone1
# Creates test.txt, writes "hello file descriptors", reads and writes to test1.txt

# Milestone 2: Single TCP client
./milestone2
# Echoes one client's messages, then exits

# Milestone 4: Non-blocking multi-client
./milestone4
# Busy-loop polling all clients (inefficient but works)

# Milestone 5: kqueue event loop
./milestone5
# Efficient event-driven echo server
```

### Key Syscalls Used
- `socket()` - Create network endpoint
- `bind()` - Assign address to socket
- `listen()` - Mark socket as passive
- `accept()` - Extract pending connection
- `read()`/`write()` - I/O on file descriptors
- `fcntl()` - Set non-blocking mode
- `kqueue()` - Create event queue (macOS)
- `kevent()` - Register events and wait

## 🎯 Next Steps (Roadmap)

### Phase 3: Advanced Features
- [ ] **TTL/EXPIRE**: Time-based key expiration
- [ ] **EXISTS**: Check if key exists
- [ ] **KEYS pattern**: Pattern matching (simple glob)

### Phase 4: Data Structures
- [ ] **Lists**: LPUSH, RPUSH, LPOP, RPOP, LRANGE
- [ ] **Sets**: SADD, SREM, SMEMBERS, SINTER
- [ ] **Hashes**: HSET, HGET, HGETALL

### Phase 5: Persistence
- [ ] **RDB**: Snapshot to disk (fork + serialize)
- [ ] **AOF**: Append-only log file
- [ ] **SAVE/BGSAVE**: Manual snapshot commands

### Phase 6: Advanced Networking
- [ ] **Pub/Sub**: SUBSCRIBE, PUBLISH, UNSUBSCRIBE
- [ ] **Transactions**: MULTI, EXEC, DISCARD
- [ ] **Pipelining**: Multiple commands without waiting

## 🐛 Known Limitations

- **No epoll support**: Currently macOS-only (kqueue). Linux port needs epoll.
- **No write buffering**: Assumes `write()` never blocks (should use `EVFILT_WRITE`)
- **No persistence**: All data lost on restart
- **No replication**: Single instance only
- **No authentication**: No AUTH command
- **Basic error handling**: Some edge cases not covered
- **Memory unbounded**: No maxmemory limits or eviction

## 📖 Why This Project?

This is a **learning project** designed to teach:
1. **POSIX syscalls** - The foundation of all network programming
2. **Event-driven architecture** - How Redis serves 100k+ clients in one thread
3. **Protocol design** - Parsing, framing, serialization
4. **Systems thinking** - File descriptors, kernel interfaces, performance

**Not a production Redis replacement** - Use the real Redis for anything serious!

## 🤝 Contributing

This is a personal learning project, but feel free to:
- Open issues for bugs or questions
- Submit PRs for missing features
- Use as a reference for your own implementation

## 📄 License

MIT License - See LICENSE file for details

---

**Built with**: Modern C++17, CMake, kqueue, Google Test, and lots of ☕
