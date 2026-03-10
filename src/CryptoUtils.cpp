#include "CryptoUtils.h"
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>

// FIX: Removed strict array limits [32] and [16]. The compiler will now automatically allocate space for the hidden null terminators.
static const unsigned char AES_KEY[] = "01234567890123456789012345678901"; 
static const unsigned char AES_IV[]  = "0123456789012345";

std::string CryptoUtils::hashPassword(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    EVP_DigestInit_ex(context, EVP_sha256(), nullptr);
    EVP_DigestUpdate(context, password.c_str(), password.length());
    EVP_DigestFinal_ex(context, hash, nullptr);
    EVP_MD_CTX_free(context);

    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string CryptoUtils::encryptVote(const std::string& plaintext) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len;
    int ciphertext_len;
    unsigned char ciphertext[256];

    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, AES_KEY, AES_IV);
    // FIX: Added static_cast<int> to safely convert size_t
    EVP_EncryptUpdate(ctx, ciphertext, &len, (unsigned char*)plaintext.c_str(), static_cast<int>(plaintext.length()));
    ciphertext_len = len;
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    ciphertext_len += len;
    EVP_CIPHER_CTX_free(ctx);

    return std::string((char*)ciphertext, ciphertext_len);
}

std::string CryptoUtils::decryptVote(const std::string& ciphertext) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len;
    int plaintext_len;
    unsigned char plaintext[256];

    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, AES_KEY, AES_IV);
    // FIX: Added static_cast<int> to safely convert size_t
    EVP_DecryptUpdate(ctx, plaintext, &len, (unsigned char*)ciphertext.c_str(), static_cast<int>(ciphertext.length()));
    plaintext_len = len;
    EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);

    plaintext[plaintext_len] = '\0';
    return std::string((char*)plaintext);
}

std::string CryptoUtils::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    struct tm timeinfo;
    #ifdef _WIN32
        localtime_s(&timeinfo, &now_c);
    #else
        localtime_r(&now_c, &timeinfo);
    #endif
    ss << std::put_time(&timeinfo, "[%H:%M:%S]");
    return ss.str();
}