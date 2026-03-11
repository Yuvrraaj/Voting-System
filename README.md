# 🗳️ Secure C++ Online Voting System

An **enterprise-grade client–server voting application built in C++17** designed to demonstrate secure systems engineering principles such as **network programming, cryptographic data protection, concurrent server architecture, and persistent database management**.

This project simulates a **real electronic election environment** where voters interact with a lightweight client application that communicates with a centralized voting server over TCP sockets.

The system has also been **deployed on AWS EC2** to demonstrate how such an architecture can operate in a distributed cloud environment.


---

# 🏗 System Architecture

The application is designed using a **client–server architecture** where responsibilities are clearly separated.
Client Application
│
│ TCP Socket Communication
▼
Voting Server
│
▼
SQLite Database


### Client Application

The client acts as the **user interface and communication layer**.  
It allows users to interact with the voting system securely.

Client responsibilities include:

- Registering a voter account
- Authenticating users during login
- Submitting votes securely to the server
- Fetching the current candidate list
- Displaying voting options and responses

The client communicates with the server using **low-level TCP sockets implemented through the Winsock2 API**.

---

### Server Application

The server acts as the **central election authority** and manages all system logic.

Server responsibilities include:

- Accepting and managing incoming client connections
- Authenticating voters securely
- Encrypting vote data before storage
- Managing candidate lists and election state
- Storing election data in a persistent database
- Providing administrative services such as statistics and election resets

To handle multiple voters simultaneously, the server uses a **multi-threaded design** where each incoming connection is processed in an isolated thread.

---

# 🔐 Key Features

## Multi-Threaded Server

The server spawns **dedicated threads for every incoming TCP connection**, ensuring that the system can support multiple voters concurrently.

This architecture prevents blocking and ensures the system remains responsive during high-traffic voting periods.

Concurrency is implemented using:

- `std::thread`
- `std::recursive_mutex`

These ensure **thread safe database access and concurrent request handling**.

---

## End-to-End Cryptographic Security

Security is a core design goal of this project.

The system integrates **OpenSSL cryptographic libraries** to protect user credentials and ballot data.

### Password Protection

User passwords are **never stored in plaintext**.

Instead, passwords are processed using the **SHA-256 hashing algorithm** before being stored.

Authentication flow:
User enters password
↓
Password hashed using SHA-256
↓
Hash stored in database


During login, the entered password is hashed again and compared with the stored hash.

This ensures **zero-knowledge password storage**.

---

### Encrypted Ballot Storage

All votes are **encrypted using AES-256 encryption** before they are written to the database.

This ensures:

- Confidentiality of voter choices
- Protection against database compromise
- Secure ballot storage

Even if the database is accessed directly, the stored ballots remain encrypted.

---

## Role Based Access Control

The system implements **role based access control** to separate voter and administrator privileges.

### Voter Permissions

- Register a voter account
- Authenticate using voter credentials
- Cast a vote
- View available candidates

### Administrator Permissions

- View election statistics
- Manage candidate lists
- Reset election data
- Monitor election state

Administrative actions require a **secret master key validated at the server level**.

---

## Persistent Database Storage

The system uses **SQLite3** for lightweight yet reliable persistent storage.

The database stores:

- Registered voters
- Candidate information
- Encrypted ballot records

To prevent corruption during concurrent access, database operations are protected using **recursive mutex locks**.

---

# ☁️ Cloud Deployment

To simulate a real world distributed system, the project was deployed on **Amazon Web Services (AWS)**.

### Compute Environment
Platform: Amazon EC2
Instance Type: t2.micro
Operating System: Windows Server 2022


The voting server runs on the cloud instance and accepts client connections through the public network.

---

### Network Security

AWS **Security Groups** were configured to allow controlled access to the server.

Example configuration:
Protocol : TCP
Port : 8080
Access : Allowed only from trusted sources


This ensures that only authorized network traffic can reach the voting server.

---

### Host Firewall Protection

In addition to AWS network filtering, **Windows Defender Firewall rules** were configured on the server instance.

This creates a **dual layer security model**:

1. Cloud level filtering through AWS Security Groups
2. Host level filtering through Windows Firewall

---

### Dependency Management

OpenSSL cryptographic libraries were included as runtime dependencies.

Required libraries:
libcrypto
libssl


These provide the cryptographic primitives used for hashing and encryption.

---

# 🛠 Technology Stack

| Component | Technology |
|----------|-----------|
| Programming Language | C++17 |
| Networking | Winsock2 TCP Sockets |
| Cryptography | OpenSSL |
| Database | SQLite3 |
| Concurrency | C++ Threads |
| Build System | CMake |
| Cloud Deployment | AWS EC2 |

---

# ⚙️ Build Instructions

### Clone the Repository

```bash
git clone https://github.com/YOUR_USERNAME/Secure-Voting-System.git
cd Secure-Voting-System
```

Generate Build Files
```bash
mkdir build
cd build
cmake ..
```

Compile the Project
```bash
cmake --build . --config Release
```

▶️ Running the System
Starting the Server

Run the server executable:
``` VotingServer.exe ```
The server will automatically:

Initialize the election database

Load candidate data

Begin listening for incoming connections on port 8080

Running the Client

Run the client executable:

``` VotingClient.exe ```

The client interface allows users to perform the following actions:

Register a voter account

Login using voter credentials

View available candidates

Cast votes securely

Access administrator services (with secret key)

🎯 Learning Objectives

This project demonstrates practical experience with:

Systems programming using C++

Low level socket networking

Multi threaded server architecture

Cryptographic security implementation

Database integration

Cloud infrastructure deployment

👨‍💻 Author

Yuvraj Jha
Computer Science Student
Vellore Institute of Technology

This project was developed as a systems engineering demonstration of a secure distributed voting platform built using C++.
