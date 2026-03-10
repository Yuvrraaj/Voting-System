# Secure Distributed Online Voting System

A highly secure, multithreaded client-server voting application built natively in C++17. This project demonstrates enterprise-grade systems engineering by integrating network socket programming, embedded relational databases, and cryptographic data protection.



## 🚀 Key Features

* **Client-Server Architecture:** Decoupled architecture using native Windows Sockets (Winsock2) over TCP/IP for real-time remote voting.
* **Multithreaded Backend:** Utilizes `std::thread` to handle multiple concurrent client connections, preventing network bottlenecks.
* **Thread-Safe Database Engine:** Integrates the SQLite amalgamation engine. Uses `std::recursive_mutex` to create critical sections, completely eliminating race conditions and data corruption during highly concurrent voting loads.
* **Advanced Cryptography (OpenSSL):**
    * **Authentication:** Passwords are mathematically hashed using SHA-256 before storage.
    * **Payload Security:** Cast votes are symmetrically encrypted using AES-256-CBC before being written to the database.
* **Cybersecurity Defenses:** * **SQL Injection Prevention:** Utilizes strictly bound Prepared Statements for all database insertions.
    * **Brute-Force Protection:** Intelligent account lockout logic triggers after 3 failed authentication attempts.
* **Remote Admin API:** Administrators can securely request live election statistics, ASCII graphical visual charts, add/remove candidates, and execute database resets over the network socket.

## 🛠️ Technology Stack
* **Language:** C++17
* **Networking:** Winsock2 (`ws2_32`)
* **Database:** SQLite 3 (Embedded Amalgamation)
* **Cryptography:** OpenSSL (EVP API)
* **Build System:** CMake

## ⚙️ Prerequisites
To compile and run this project on a Windows machine, you need:
1.  **C++17 Compiler:** MSVC (Visual Studio) or MinGW.
2.  **CMake:** Version 3.10 or higher.
3.  **OpenSSL for Windows:** Installed in `C:\Program Files\OpenSSL-Win64` (Standard pre-compiled Win64 binaries).

## 🏗️ Build Instructions

1. **Clone the repository:**
   ```bash
   git clone [https://github.com/yourusername/OnlineVotingSystem.git](https://github.com/yourusername/OnlineVotingSystem.git)
   cd OnlineVotingSystem
