# Secure C++ Online Voting System

An enterprise grade multi threaded client server voting application built in C++17.  
The system demonstrates low level network programming cryptographic security concurrent database handling and cloud deployment.

The project simulates a real world electronic election environment where voters interact with a client application that communicates securely with a centralized voting server.

The system has been deployed on AWS EC2 to demonstrate distributed infrastructure and real world networking.


# System Architecture

The application follows a client server architecture.

Client Application  
Handles user interaction registration authentication and vote casting.

Server Application  
Acts as the central election authority responsible for authentication encryption and database operations.

Client communicates with the server through TCP socket communication.


# Client Responsibilities

User registration

User authentication

Secure vote submission

Fetching candidate list from the server

Communication with server using Winsock2 TCP sockets


# Server Responsibilities

Accepting client connections

Handling multiple clients using multi threading

Processing authentication requests

Encrypting ballots before storage

Managing election database and statistics


# Key Features

# Multi Threaded Server

The server spawns dedicated threads for every incoming TCP connection.

This ensures that multiple voters can interact with the system simultaneously without blocking other users.

Technologies used

C++ threads  
recursive mutex for thread safe operations


# End To End Cryptography

The system integrates OpenSSL to provide cryptographic security.

Passwords are never stored in plain text.

Password flow

Password entered by user  
Converted to SHA 256 hash  
Stored securely in the database

This ensures zero knowledge password storage.


# Vote Encryption

Ballots are encrypted using AES 256 encryption before being stored in the database.

This guarantees

Vote confidentiality  
Protection against database compromise  
Secure ballot storage


# Role Based Access Control

The system separates privileges between voters and administrators.

Voter capabilities

Register account  
Login  
Cast vote

Administrator capabilities

View election statistics  
Add or remove candidates  
Reset election

Administrative operations require a server validated master secret key.


# Persistent Database Storage

The system uses SQLite3 as the database engine.

The database stores

Registered voters  
Candidate records  
Encrypted ballots

Thread safety is ensured using recursive mutex locks to prevent race conditions during concurrent access.


# Cloud Deployment

The project is deployed on Amazon Web Services to simulate a real world distributed election environment.

# Compute Infrastructure

Hosted on AWS EC2

Instance type  
t2.micro

Operating system  
Windows Server 2022


# Network Security

AWS Security Groups configured to allow TCP traffic only on the required port.

Server port

8080

Traffic filtering ensures controlled access to the voting server.


# Firewall Protection

Additional protection implemented using Windows Defender Firewall rules.

This creates a dual layer security model consisting of

Cloud level firewall filtering  
Host machine firewall filtering


# Dependency Management

OpenSSL cryptographic libraries are statically linked for reliable deployment on cloud infrastructure.

Libraries used

libcrypto  
libssl


# Technology Stack

Programming Language  
C++17

Networking  
Winsock2 API

Cryptography  
OpenSSL

Database  
SQLite3

Concurrency  
C++ Threads

Build System  
CMake

Deployment Platform  
AWS EC2


# Build Instructions

Clone the repository

git clone https://github.com/YOUR_USERNAME/Secure-Voting-System.git

Navigate into project directory

cd Secure-Voting-System


Generate CMake build files

mkdir build

cd build

cmake ..


Compile the project

cmake --build . --config Release


# Runtime Requirement

Ensure the following OpenSSL runtime libraries are present in the executable directory

libcrypto-3-x64.dll

libssl-3-x64.dll


# Usage

# Start Server

Run VotingServer.exe on the host machine or AWS instance.

The server will initialize the enterprise election database and begin listening for incoming connections on port 8080.


# Run Client

Run VotingClient.exe.

The client interface allows users to perform the following actions.

Register a voter account  
Login using voter credentials  
Cast vote securely  
Fetch candidate list from the server


# Administrator Services

Administrative functions include

Viewing live election statistics  
Managing candidate records  
Resetting election database

These actions require the master secret key verified by the server.


# Learning Outcomes

This project demonstrates practical experience with

Systems programming in C++

Low level socket networking

Concurrent server architecture

Cryptographic security implementation

Database integration

Cloud infrastructure deployment


# Author

Yuvraj Jha

Computer Science Student  
Vellore Institute of Technology

This project was developed as a systems engineering demonstration of secure distributed application design.
