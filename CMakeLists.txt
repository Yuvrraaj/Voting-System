cmake_minimum_required(VERSION 3.10)
project(OnlineVotingSystem)

set(CMAKE_CXX_STANDARD 17)

# Threading
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# --- NEW: Tell CMake exactly where OpenSSL is installed ---
set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")
set(OPENSSL_USE_STATIC_LIBS TRUE)
# ----------------------------------------------------------

# OpenSSL
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

include_directories(include)

# --- BUILD THE SERVER ---
add_executable(VotingServer 
    src/Server.cpp 
    src/CryptoUtils.cpp 
    src/sqlite3.c 
)
target_link_libraries(VotingServer Threads::Threads OpenSSL::SSL OpenSSL::Crypto ws2_32)

# --- BUILD THE CLIENT ---
add_executable(VotingClient 
    src/Client.cpp 
    src/CryptoUtils.cpp
)
target_link_libraries(VotingClient Threads::Threads OpenSSL::SSL OpenSSL::Crypto ws2_32)