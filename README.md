# Secure C++ Online Voting System

A secure client-server voting application built in C++ that demonstrates low level networking, cryptographic security, concurrent processing and database management.

The system simulates a real election environment where voters interact with a client application that communicates with a centralized voting server.

The project has also been deployed on AWS EC2 to simulate a distributed production environment.


# System Architecture

The application follows a client server architecture.

Client Application  
Handles user registration, authentication and vote casting.

Server Application  
Handles authentication, encryption, database operations and election management.

Communication between the client and server is performed using TCP sockets.


# Key Features

Multi Threaded Server  
Handles multiple client connections simultaneously using C++ threads.

Secure Authentication  
User passwords are hashed using SHA 256 through OpenSSL before storage.

Encrypted Ballots  
Votes are encrypted using AES 256 before being stored in the database.

Role Based Access Control  
Separates voter and administrator privileges for secure election management.

Persistent Database  
SQLite3 is used for secure storage of voters, candidates and ballots.

Concurrent Data Safety  
Database access is protected using recursive mutex locks.


# Technology Stack

Language: C++17  
Networking: Winsock2 TCP Sockets  
Cryptography: OpenSSL  
Database: SQLite3  
Concurrency: C++ Threads  
Build System: CMake  
Deployment: AWS EC2


# Build Instructions

Clone the repository

git clone https://github.com/YOUR_USERNAME/Secure-Voting-System.git

Navigate to the project

cd Secure-Voting-System

Generate build files

mkdir build
cd build
cmake ..

Compile the project

cmake --build . --config Release


# Usage

Start the server

VotingServer.exe

The server initializes the election database and begins listening for connections.

Run the client

VotingClient.exe

The client allows users to

Register a voter account  
Login securely  
Cast votes  
Access admin services with the secret key


# Author

Yuvraj Jha  
Computer Science Student  
Vellore Institute of Technology
