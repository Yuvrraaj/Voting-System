# Secure Distributed Voting Platform
### C++17 · Winsock2 TCP · OpenSSL · SQLite3 · AWS EC2

> A production-grade client–server voting system built from scratch in C++ — multithreaded, encrypted end-to-end, and deployed on AWS EC2 with zero race conditions.

---

## The Problem

Electronic voting systems fail in two predictable ways: they either don't scale under concurrent load (single-threaded servers that queue voters and time out), or they don't protect data at rest (plaintext ballots in a database that becomes a single point of compromise). This project addresses both simultaneously — a multithreaded TCP server handling concurrent voters with zero race conditions, and a layered cryptographic architecture where neither passwords nor ballots ever touch disk unprotected.

---

## System Architecture

```
VotingClient.exe
      │
      │  TCP socket (Winsock2)
      │  Raw byte protocol over Port 8080
      ▼
VotingServer.exe  ──▶  thread pool (std::thread per connection)
      │                recursive_mutex guards all shared state
      │
      ├──▶  OpenSSL EVP API
      │         ├── SHA-256  →  credential hashing
      │         └── AES-256-CBC  →  ballot encryption
      │
      └──▶  SQLite3 (embedded)
                ├── voters table      (id, username, SHA-256 hash, role, voted flag)
                ├── candidates table  (id, name, party, encrypted_vote_count)
                └── ballots table     (id, voter_id, encrypted_choice, timestamp)
```

The client is a pure communication and UI layer — it holds no business logic, no encryption keys, no database access. All authority is server-side.

---

## How It Works

### Networking — Native Winsock2 TCP Sockets

The server and client communicate over raw TCP using the **Winsock2 API** (`ws2_32.lib`) — no HTTP, no REST, no abstraction layer. The server binds to port 8080, listens, and accepts connections in a loop. Each accepted socket is handed off to a dedicated `std::thread` immediately so the accept loop is never blocked.

The protocol is a custom binary/text framing: each message is prefixed with a command identifier followed by a payload. This keeps bandwidth minimal and avoids the overhead of JSON parsing on every vote submission.

---

### Concurrency — Multithreaded with Recursive Mutex

Every incoming client connection spawns its own `std::thread`. The threading model is intentionally simple — no thread pool, no work queue — because the bottleneck is I/O wait (network reads) not CPU, so thread-per-connection is appropriate here.

The critical section is SQLite: a single database file accessed from N threads simultaneously. This is guarded with a `std::recursive_mutex` rather than a plain mutex because the authentication flow calls into database helpers that also acquire the same lock (a plain mutex would deadlock on reentrant calls).

Result: **zero race conditions** on vote submission, even under concurrent load.

---

### Security Layer 1 — SHA-256 Credential Hashing

Passwords are never stored or transmitted in plaintext. On registration:

```
user_password  →  SHA-256 (OpenSSL EVP_DigestInit / EVP_DigestUpdate / EVP_DigestFinal)
                       →  hex digest stored in voters table
```

On login, the entered password is hashed identically and compared against the stored digest. The server never sees the raw password after the initial hash — **zero-knowledge credential storage**.

Brute-force login protection is enforced at the server: repeated failed authentication attempts from a connection are detected and the session is terminated.

---

### Security Layer 2 — AES-256-CBC Ballot Encryption

Every vote is encrypted before being written to the database using **AES-256 in CBC mode** via the OpenSSL EVP API:

```
plaintext_ballot  →  AES-256-CBC encrypt (EVP_EncryptInit_ex / EVP_EncryptUpdate / EVP_EncryptFinal_ex)
                           →  ciphertext stored in ballots table
```

The encryption key and IV are managed server-side. Even if the SQLite database file is exfiltrated directly from disk, the ballot contents are unreadable without the server's key material. Vote tallying decrypts ballots in memory only at result computation time.

---

### Security Layer 3 — Prepared Statements (SQL Injection Prevention)

All database queries use **SQLite3 prepared statements** with bound parameters — no string interpolation, no format-string construction of SQL. This closes the SQL injection vector entirely regardless of what a client sends as a username or payload.

---

### Role-Based Access Control (RBAC)

The system enforces two roles at the server level:

| Role | Permissions |
|------|-------------|
| **Voter** | Register, authenticate, view candidates, cast one vote |
| **Administrator** | View election statistics, manage candidates, reset election data, monitor state |

Administrative endpoints require a **server-side secret master key** validated on every admin request. Role is stored in the voters table and checked on each authenticated session — a voter-role session cannot escalate to admin operations regardless of what it sends.

The "voted" flag per voter is set atomically under mutex lock after a successful ballot write, preventing double-voting even if two requests from the same voter arrive simultaneously.

---

### Persistent Storage — Embedded SQLite3

SQLite3 is embedded directly into the server binary — no separate database process, no network connection to a database server, no installation required. The database file is initialised on first run with the schema and pre-loaded candidate data.

Three tables:
- `voters` — username, SHA-256 password hash, role, voted flag
- `candidates` — name, party affiliation
- `ballots` — voter ID, encrypted ballot bytes, timestamp

All writes are wrapped in mutex-protected transactions to prevent partial writes under concurrent access.

---

## Cloud Deployment — AWS EC2

The server was deployed on AWS EC2 to run as an actual distributed system with real remote clients connecting over the public internet.

**Environment:**
- Instance: `t2.micro`, Windows Server 2022
- Runtime dependencies: `libssl-3-x64.dll`, `libcrypto-3-x64.dll` (OpenSSL 3.x, bundled with release)

**Network security — two layers:**

1. **AWS Security Groups** — inbound rule allowing TCP on port 8080 only from trusted CIDR ranges. All other ports blocked at the cloud network level before traffic reaches the instance.

2. **Windows Defender Firewall** — host-level inbound rule permitting port 8080. Combined with Security Groups this creates a defence-in-depth perimeter: an attacker bypassing the cloud filter still hits the host firewall.

This dual-layer model means neither a misconfigured Security Group nor a misconfigured host firewall alone opens the system — both must allow traffic for a connection to reach the server process.

---

## Running the System

### Pre-built Release (Windows)

Download the release zip from the [Releases](https://github.com/Yuvrraaj/Voting-System/releases) page. It contains:

```
VotingServer.exe
VotingClient.exe
libssl-3-x64.dll
libcrypto-3-x64.dll
```

**Start the server:**
```
VotingServer.exe
```
The server initialises the SQLite database, loads candidates, and begins listening on **port 8080**.

**Run the client** (same machine or remote, update server IP in client config):
```
VotingClient.exe
```

Client menu options:
1. Register voter account
2. Login
3. View candidates
4. Cast vote
5. Administrator panel (requires master key)

---

### Build from Source

**Prerequisites:**
- CMake 3.15+
- C++17-compatible compiler (MSVC or MinGW-w64)
- OpenSSL 3.x (headers + libs)
- SQLite3 (amalgamation included in `include/`)

```bash
git clone https://github.com/Yuvrraaj/Voting-System.git
cd Voting-System

mkdir build && cd build
cmake ..
cmake --build . --config Release
```

Binaries output to `build/Release/`. Copy `libssl-3-x64.dll` and `libcrypto-3-x64.dll` alongside the executables before running.

---

## Project Structure

```
├── src/
│   ├── server.cpp          # TCP server, thread spawning, request routing
│   ├── client.cpp          # Winsock2 client, UI, protocol handling
│   ├── crypto.cpp          # OpenSSL EVP wrappers: SHA-256, AES-256-CBC
│   ├── database.cpp        # SQLite3 operations, prepared statements, schema init
│   └── auth.cpp            # RBAC, session management, brute-force protection
├── include/
│   ├── sqlite3.h / sqlite3.c   # SQLite3 amalgamation
│   └── *.h                     # Header declarations
├── CMakeLists.txt
├── libssl-3-x64.dll        # OpenSSL runtime (bundled)
├── libcrypto-3-x64.dll     # OpenSSL runtime (bundled)
└── README.md
```

---

## Tech Stack

| Component | Technology |
|-----------|------------|
| Language | C++17 |
| Networking | Winsock2 (`ws2_32.lib`) |
| Cryptography | OpenSSL 3.x EVP API |
| Database | SQLite3 (embedded amalgamation) |
| Concurrency | `std::thread`, `std::recursive_mutex` |
| Build System | CMake |
| Cloud | AWS EC2 (Windows Server 2022) |

---

## Security Properties Summary

| Property | Mechanism |
|----------|-----------|
| Password confidentiality | SHA-256 hashing, never stored plaintext |
| Ballot confidentiality | AES-256-CBC encryption at rest |
| SQL injection prevention | Prepared statements throughout |
| Double-vote prevention | Atomic mutex-locked voted flag |
| Privilege escalation prevention | Server-side RBAC + master key validation |
| Network perimeter | AWS Security Groups + Windows Firewall (dual layer) |
| Brute-force protection | Server-side failed-attempt detection + session termination |

---

## License

MIT License. See `LICENSE` for details.
