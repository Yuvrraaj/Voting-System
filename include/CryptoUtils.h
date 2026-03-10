#pragma once
#include <string>

class CryptoUtils {
public:
    // Real SHA-256 Hashing
    static std::string hashPassword(const std::string& password);
    
    // Real AES-256 Encryption
    static std::string encryptVote(const std::string& plaintext);
    static std::string decryptVote(const std::string& ciphertext);
    
    static std::string getCurrentTimestamp();
};