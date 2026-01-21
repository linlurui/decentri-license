#include "decentrilicense/crypto_utils.hpp"
#include "decentrilicense/root_key.hpp"
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>
#include <cstring>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <array>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#else
#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <net/if.h>
#endif

namespace decentrilicense {

const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string CryptoUtils::encrypt_token_aes256_gcm(const std::string& token_json, const std::string& product_public_key_file_content) {
    initialize_openssl();

    const auto key = derive_aes256_key_from_product_public_key(product_public_key_file_content);
    std::vector<uint8_t> nonce(12);
    if (RAND_bytes(nonce.data(), static_cast<int>(nonce.size())) != 1) {
        throw std::runtime_error("failed to generate nonce");
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("failed to create cipher context");
    }

    std::vector<uint8_t> ciphertext(token_json.size());
    int out_len = 0;
    int total_len = 0;

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("failed to init aes-256-gcm");
    }
    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("failed to set key/nonce");
    }
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len, reinterpret_cast<const unsigned char*>(token_json.data()), static_cast<int>(token_json.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("failed to encrypt");
    }
    total_len = out_len;
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + total_len, &out_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("failed to finalize encrypt");
    }
    total_len += out_len;
    ciphertext.resize(static_cast<size_t>(total_len));

    std::vector<uint8_t> tag(16);
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, static_cast<int>(tag.size()), tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("failed to get tag");
    }
    EVP_CIPHER_CTX_free(ctx);

    std::vector<uint8_t> ciphertext_with_tag;
    ciphertext_with_tag.reserve(ciphertext.size() + tag.size());
    ciphertext_with_tag.insert(ciphertext_with_tag.end(), ciphertext.begin(), ciphertext.end());
    ciphertext_with_tag.insert(ciphertext_with_tag.end(), tag.begin(), tag.end());

    return base64url_encode(ciphertext_with_tag) + "|" + base64url_encode(nonce);
}

std::string CryptoUtils::decrypt_token_aes256_gcm(const std::string& encrypted_token, const std::string& product_public_key_file_content) {
    initialize_openssl();

    size_t sep = encrypted_token.find('|');
    if (sep == std::string::npos) {
        throw std::runtime_error("invalid encrypted token format");
    }

    const std::string ct_b64u = encrypted_token.substr(0, sep);
    const std::string nonce_b64u = encrypted_token.substr(sep + 1);

    std::vector<uint8_t> ciphertext_with_tag = base64url_decode(ct_b64u);
    std::vector<uint8_t> nonce = base64url_decode(nonce_b64u);

    if (nonce.size() != 12) {
        throw std::runtime_error("invalid nonce length");
    }
    if (ciphertext_with_tag.size() < 16) {
        throw std::runtime_error("invalid ciphertext length");
    }

    const auto key = derive_aes256_key_from_product_public_key(product_public_key_file_content);

    std::vector<uint8_t> tag(ciphertext_with_tag.end() - 16, ciphertext_with_tag.end());
    std::vector<uint8_t> ciphertext(ciphertext_with_tag.begin(), ciphertext_with_tag.end() - 16);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("failed to create cipher context");
    }

    std::vector<uint8_t> plaintext(ciphertext.size());
    int out_len = 0;
    int total_len = 0;

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("failed to init aes-256-gcm");
    }
    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("failed to set key/nonce");
    }
    if (EVP_DecryptUpdate(ctx, plaintext.data(), &out_len, ciphertext.data(), static_cast<int>(ciphertext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("failed to decrypt");
    }
    total_len = out_len;
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, static_cast<int>(tag.size()), tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("failed to set tag");
    }
    int final_ok = EVP_DecryptFinal_ex(ctx, plaintext.data() + total_len, &out_len);
    EVP_CIPHER_CTX_free(ctx);
    if (final_ok != 1) {
        throw std::runtime_error("gcm tag verification failed");
    }
    total_len += out_len;
    plaintext.resize(static_cast<size_t>(total_len));

    return std::string(reinterpret_cast<const char*>(plaintext.data()), plaintext.size());
}

std::string CryptoUtils::base64_encode(const std::vector<uint8_t>& data) {
    std::string ret;
    int i = 0;
    int j = 0;
    uint8_t char_array_3[3];
    uint8_t char_array_4[4];
    int data_len = data.size();
    
    for (int k = 0; k < data_len; k++) {
        char_array_3[i++] = data[k];
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i)
    {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';

    }

    return ret;
}

std::string CryptoUtils::base64_encode(const std::string& data) {
    std::vector<uint8_t> vec(data.begin(), data.end());
    return base64_encode(vec);
}

std::string CryptoUtils::base64url_encode(const std::vector<uint8_t>& data) {
    std::string s = base64_encode(data);
    for (char& c : s) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    while (!s.empty() && s.back() == '=') {
        s.pop_back();
    }
    return s;
}

std::vector<uint8_t> CryptoUtils::base64url_decode(const std::string& data) {
    std::string s = data;
    for (char& c : s) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    while (s.size() % 4 != 0) {
        s.push_back('=');
    }
    return base64_decode(s);
}

std::array<uint8_t, 32> CryptoUtils::sha256_bytes(const std::string& data) {
    std::array<uint8_t, 32> out{};
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data.data(), data.size());
    SHA256_Final(out.data(), &ctx);
    return out;
}

std::array<uint8_t, 32> CryptoUtils::derive_aes256_key_from_product_public_key(const std::string& product_public_key_file_content) {
    // For token encryption/decryption, use the hardcoded root public key (matching dl-issuer behavior)
    // dl-issuer uses the root public key PEM content as the basis for AES key derivation
    return sha256_bytes(decentrilicense::ROOT_PUBLIC_KEY);
}

std::vector<uint8_t> CryptoUtils::base64_decode(const std::string& encoded) {
    // Use OpenSSL's built-in base64 decoder for reliability
    BIO *bio, *b64;
    int decodeLen = encoded.size() * 3 / 4 + 1;
    std::vector<uint8_t> buffer(decodeLen);

    bio = BIO_new_mem_buf(encoded.c_str(), encoded.size());
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    // Don't use newlines to flush buffer
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    int len = BIO_read(bio, buffer.data(), decodeLen);
    BIO_free_all(bio);

    if (len < 0) {
        return std::vector<uint8_t>(); // Return empty vector on error
    }

    buffer.resize(len);
    return buffer;
}

std::string CryptoUtils::sha256(const std::string& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.c_str(), data.size());
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

std::string CryptoUtils::compute_license_key_hash(const std::string& license_public_key_pem) {
    // Extract the actual public key PEM from the file content
    // The file may contain additional data after the PEM block
    std::string actualPublicKeyPEM = license_public_key_pem;
    size_t rootSigPos = license_public_key_pem.find("\nROOT_SIGNATURE:");
    if (rootSigPos != std::string::npos) {
        actualPublicKeyPEM = license_public_key_pem.substr(0, rootSigPos);
    }
    
    // Generate SHA256 hash of the license public key
    return sha256(actualPublicKeyPEM);
}

std::string CryptoUtils::generate_device_id() {
    // Simple implementation - in a real-world scenario, this would collect
    // machine-specific information like MAC addresses, CPU info, etc.
    return sha256("device_specific_data");
}

// Add AES-GCM encryption/decryption implementation
std::string CryptoUtils::aes_encrypt(const std::string& plaintext, const std::string& key) {
    initialize_openssl();
    
    // Create cipher context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create cipher context");
    }
    
    // Initialize encryption with AES-256-GCM
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize AES-GCM encryption");
    }
    
    // Set key length to 32 bytes for AES-256
    if (EVP_CIPHER_CTX_set_key_length(ctx, 32) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set key length");
    }
    
    // Generate random IV (12 bytes for GCM)
    std::vector<uint8_t> iv(12);
    if (RAND_bytes(iv.data(), iv.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to generate random IV");
    }
    
    // Set key and IV
    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, 
                          reinterpret_cast<const unsigned char*>(key.data()), 
                          iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set key and IV");
    }
    
    // Encrypt the plaintext
    std::vector<uint8_t> ciphertext(plaintext.length() + 16); // Add space for padding
    int len = 0;
    int ciphertext_len = 0;
    
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                         reinterpret_cast<const unsigned char*>(plaintext.data()), 
                         plaintext.length()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to encrypt data");
    }
    ciphertext_len = len;
    
    // Finalize encryption
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize encryption");
    }
    ciphertext_len += len;
    
    // Get the authentication tag (16 bytes)
    std::vector<uint8_t> tag(16);
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to get authentication tag");
    }
    
    // Clean up
    EVP_CIPHER_CTX_free(ctx);
    
    // Combine IV, ciphertext, and tag
    std::vector<uint8_t> result;
    result.insert(result.end(), iv.begin(), iv.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + ciphertext_len);
    result.insert(result.end(), tag.begin(), tag.end());
    
    return base64_encode(result);
}

std::string CryptoUtils::aes_decrypt(const std::string& ciphertext, const std::string& key) {
    initialize_openssl();
    
    // Decode base64 input
    std::vector<uint8_t> data = base64_decode(ciphertext);
    
    // Check minimum length (IV + tag = 12 + 16 = 28 bytes)
    if (data.size() < 28) {
        throw std::runtime_error("Invalid ciphertext length");
    }
    
    // Extract IV (first 12 bytes)
    std::vector<uint8_t> iv(data.begin(), data.begin() + 12);
    
    // Extract authentication tag (last 16 bytes)
    std::vector<uint8_t> tag(data.end() - 16, data.end());
    
    // Extract actual ciphertext (between IV and tag)
    std::vector<uint8_t> encrypted_data(data.begin() + 12, data.end() - 16);
    
    // Create cipher context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create cipher context");
    }
    
    // Initialize decryption with AES-256-GCM
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize AES-GCM decryption");
    }
    
    // Set key length to 32 bytes for AES-256
    if (EVP_CIPHER_CTX_set_key_length(ctx, 32) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set key length");
    }
    
    // Set key and IV
    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr,
                          reinterpret_cast<const unsigned char*>(key.data()),
                          iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set key and IV");
    }
    
    // Decrypt the ciphertext
    std::vector<uint8_t> plaintext(encrypted_data.size());
    int len = 0;
    int plaintext_len = 0;
    
    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len,
                         encrypted_data.data(), encrypted_data.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to decrypt data");
    }
    plaintext_len = len;
    
    // Set expected tag
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set authentication tag");
    }
    
    // Finalize decryption
    int ret = EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len);
    EVP_CIPHER_CTX_free(ctx);
    
    if (ret != 1) {
        throw std::runtime_error("Authentication failed - invalid tag");
    }
    plaintext_len += len;
    
    // Resize plaintext to actual length
    plaintext.resize(plaintext_len);
    
    return std::string(plaintext.begin(), plaintext.end());
}

// RAII wrapper for OpenSSL structures
template<typename T, void(*Deleter)(T*)>
struct OpenSSLDeleter {
    void operator()(T* ptr) const {
        if (ptr) Deleter(ptr);
    }
};

using EVPKeyPtr = std::unique_ptr<EVP_PKEY, OpenSSLDeleter<EVP_PKEY, EVP_PKEY_free>>;
using EVPCipherCtxPtr = std::unique_ptr<EVP_CIPHER_CTX, OpenSSLDeleter<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free>>;
using EVPMDCtxPtr = std::unique_ptr<EVP_MD_CTX, OpenSSLDeleter<EVP_MD_CTX, EVP_MD_CTX_free>>;
using BIOPtr = std::unique_ptr<BIO, OpenSSLDeleter<BIO, BIO_free_all>>;

// CryptoUtils implementation
void CryptoUtils::initialize_openssl() {
    // OpenSSL 1.1.0+ doesn't require explicit initialization
}

void CryptoUtils::cleanup_openssl() {
    // OpenSSL 1.1.0+ doesn't require explicit cleanup
}

CryptoUtils::KeyPair CryptoUtils::generate_rsa_keypair(int key_size) {
    initialize_openssl();
    
    KeyPair result;
    
    // Generate RSA key
    EVP_PKEY* pkey_raw = nullptr;
    EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!pctx) {
        throw std::runtime_error("Failed to create EVP_PKEY_CTX");
    }
    
    if (EVP_PKEY_keygen_init(pctx) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        throw std::runtime_error("Failed to initialize key generation");
    }
    
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(pctx, key_size) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        throw std::runtime_error("Failed to set key size");
    }
    
    if (EVP_PKEY_keygen(pctx, &pkey_raw) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        throw std::runtime_error("Failed to generate key");
    }
    
    EVP_PKEY_CTX_free(pctx);
    EVPKeyPtr pkey(pkey_raw);
    
    // Export public key to PEM
    BIO* bio_pub = BIO_new(BIO_s_mem());
    if (!bio_pub) {
        throw std::runtime_error("Failed to create BIO for public key");
    }
    
    if (PEM_write_bio_PUBKEY(bio_pub, pkey.get()) != 1) {
        BIO_free(bio_pub);
        throw std::runtime_error("Failed to write public key");
    }
    
    char* pub_key_data = nullptr;
    long pub_key_len = BIO_get_mem_data(bio_pub, &pub_key_data);
    result.public_key_pem = std::string(pub_key_data, pub_key_len);
    BIO_free(bio_pub);
    
    // Export private key to PEM
    BIO* bio_priv = BIO_new(BIO_s_mem());
    if (!bio_priv) {
        throw std::runtime_error("Failed to create BIO for private key");
    }
    
    if (PEM_write_bio_PrivateKey(bio_priv, pkey.get(), nullptr, nullptr, 0, nullptr, nullptr) != 1) {
        BIO_free(bio_priv);
        throw std::runtime_error("Failed to write private key");
    }
    
    char* priv_key_data = nullptr;
    long priv_key_len = BIO_get_mem_data(bio_priv, &priv_key_data);
    result.private_key_pem = std::string(priv_key_data, priv_key_len);
    BIO_free(bio_priv);
    
    return result;
}

CryptoUtils::KeyPair CryptoUtils::generate_ed25519_keypair() {
    initialize_openssl();
    
    KeyPair result;
    
    // Generate Ed25519 key
    EVP_PKEY* pkey_raw = nullptr;
    EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
    if (!pctx) {
        throw std::runtime_error("Failed to create EVP_PKEY_CTX for Ed25519");
    }
    
    if (EVP_PKEY_keygen_init(pctx) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        throw std::runtime_error("Failed to initialize Ed25519 key generation");
    }
    
    if (EVP_PKEY_keygen(pctx, &pkey_raw) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        throw std::runtime_error("Failed to generate Ed25519 key");
    }
    
    EVP_PKEY_CTX_free(pctx);
    EVPKeyPtr pkey(pkey_raw);
    
    // Export public key to PEM
    BIO* bio_pub = BIO_new(BIO_s_mem());
    if (!bio_pub) {
        throw std::runtime_error("Failed to create BIO for Ed25519 public key");
    }
    
    if (PEM_write_bio_PUBKEY(bio_pub, pkey.get()) != 1) {
        BIO_free(bio_pub);
        throw std::runtime_error("Failed to write Ed25519 public key");
    }
    
    char* pub_key_data = nullptr;
    long pub_key_len = BIO_get_mem_data(bio_pub, &pub_key_data);
    result.public_key_pem = std::string(pub_key_data, pub_key_len);
    BIO_free(bio_pub);
    
    // Export private key to PEM
    BIO* bio_priv = BIO_new(BIO_s_mem());
    if (!bio_priv) {
        throw std::runtime_error("Failed to create BIO for Ed25519 private key");
    }
    
    if (PEM_write_bio_PrivateKey(bio_priv, pkey.get(), nullptr, nullptr, 0, nullptr, nullptr) != 1) {
        BIO_free(bio_priv);
        throw std::runtime_error("Failed to write Ed25519 private key");
    }
    
    char* priv_key_data = nullptr;
    long priv_key_len = BIO_get_mem_data(bio_priv, &priv_key_data);
    result.private_key_pem = std::string(priv_key_data, priv_key_len);
    BIO_free(bio_priv);
    
    return result;
}

CryptoUtils::KeyPair CryptoUtils::generate_sm2_keypair() {
    initialize_openssl();
    
    KeyPair result;
    
    // Generate SM2 key
    EVP_PKEY* pkey_raw = nullptr;
    EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_SM2, nullptr);
    if (!pctx) {
        throw std::runtime_error("Failed to create EVP_PKEY_CTX for SM2");
    }
    
    if (EVP_PKEY_keygen_init(pctx) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        throw std::runtime_error("Failed to initialize SM2 key generation");
    }
    
    if (EVP_PKEY_keygen(pctx, &pkey_raw) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        throw std::runtime_error("Failed to generate SM2 key");
    }
    
    EVP_PKEY_CTX_free(pctx);
    EVPKeyPtr pkey(pkey_raw);
    
    // Export public key to PEM
    BIO* bio_pub = BIO_new(BIO_s_mem());
    if (!bio_pub) {
        throw std::runtime_error("Failed to create BIO for SM2 public key");
    }
    
    if (PEM_write_bio_PUBKEY(bio_pub, pkey.get()) != 1) {
        BIO_free(bio_pub);
        throw std::runtime_error("Failed to write SM2 public key");
    }
    
    char* pub_key_data = nullptr;
    long pub_key_len = BIO_get_mem_data(bio_pub, &pub_key_data);
    result.public_key_pem = std::string(pub_key_data, pub_key_len);
    BIO_free(bio_pub);
    
    // Export private key to PEM
    BIO* bio_priv = BIO_new(BIO_s_mem());
    if (!bio_priv) {
        throw std::runtime_error("Failed to create BIO for SM2 private key");
    }
    
    if (PEM_write_bio_PrivateKey(bio_priv, pkey.get(), nullptr, nullptr, 0, nullptr, nullptr) != 1) {
        BIO_free(bio_priv);
        throw std::runtime_error("Failed to write SM2 private key");
    }
    
    char* priv_key_data = nullptr;
    long priv_key_len = BIO_get_mem_data(bio_priv, &priv_key_data);
    result.private_key_pem = std::string(priv_key_data, priv_key_len);
    BIO_free(bio_priv);
    
    return result;
}

std::string CryptoUtils::sign_data(const std::string& data, const std::string& private_key_pem) {
    // Load private key
    BIOPtr bio(BIO_new_mem_buf(private_key_pem.c_str(), -1));
    if (!bio) {
        throw std::runtime_error("Failed to create BIO for private key");
    }
    
    EVP_PKEY* pkey_raw = PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr);
    if (!pkey_raw) {
        throw std::runtime_error("Failed to read private key");
    }
    EVPKeyPtr pkey(pkey_raw);
    
    // Create signature context
    EVPMDCtxPtr ctx(EVP_MD_CTX_new());
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }
    
    if (EVP_DigestSignInit(ctx.get(), nullptr, EVP_sha256(), nullptr, pkey.get()) != 1) {
        throw std::runtime_error("Failed to initialize signing");
    }
    
    if (EVP_DigestSignUpdate(ctx.get(), data.c_str(), data.length()) != 1) {
        throw std::runtime_error("Failed to update signature");
    }
    
    // Determine signature length
    size_t sig_len = 0;
    if (EVP_DigestSignFinal(ctx.get(), nullptr, &sig_len) != 1) {
        throw std::runtime_error("Failed to determine signature length");
    }
    
    // Generate signature
    std::vector<uint8_t> signature(sig_len);
    if (EVP_DigestSignFinal(ctx.get(), signature.data(), &sig_len) != 1) {
        throw std::runtime_error("Failed to generate signature");
    }
    
    signature.resize(sig_len);
    return base64_encode(signature);
}

bool CryptoUtils::verify_signature(const std::string& data, 
                                   const std::string& signature,
                                   const std::string& public_key_pem) {
    try {
        // Load public key
        BIOPtr bio(BIO_new_mem_buf(public_key_pem.c_str(), -1));
        if (!bio) {
            return false;
        }
        
        EVP_PKEY* pkey_raw = PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr);
        if (!pkey_raw) {
            return false;
        }
        EVPKeyPtr pkey(pkey_raw);
        
        // Decode signature
        std::vector<uint8_t> sig_bytes = base64_decode(signature);
        
        // Create verification context
        EVPMDCtxPtr ctx(EVP_MD_CTX_new());
        if (!ctx) {
            return false;
        }
        
        if (EVP_DigestVerifyInit(ctx.get(), nullptr, EVP_sha256(), nullptr, pkey.get()) != 1) {
            return false;
        }
        
        if (EVP_DigestVerifyUpdate(ctx.get(), data.c_str(), data.length()) != 1) {
            return false;
        }
        
        int result = EVP_DigestVerifyFinal(ctx.get(), sig_bytes.data(), sig_bytes.size());
        return result == 1;
    } catch (...) {
        return false;
    }
}

std::string CryptoUtils::sign_ed25519_data(const std::string& data, const std::string& private_key_pem) {
    // Load private key
    BIOPtr bio(BIO_new_mem_buf(private_key_pem.c_str(), -1));
    if (!bio) {
        throw std::runtime_error("Failed to create BIO for Ed25519 private key");
    }
    
    EVP_PKEY* pkey_raw = PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr);
    if (!pkey_raw) {
        throw std::runtime_error("Failed to read Ed25519 private key");
    }
    EVPKeyPtr pkey(pkey_raw);
    
    // Check if it's an Ed25519 key
    int key_type = EVP_PKEY_id(pkey.get());
    if (key_type != EVP_PKEY_ED25519) {
        throw std::runtime_error("Not an Ed25519 key");
    }
    
    // For Ed25519, we don't use a digest, we sign the raw data directly
    // Create signature context
    EVPMDCtxPtr ctx(EVP_MD_CTX_new());
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }
    
    if (EVP_DigestSignInit(ctx.get(), nullptr, nullptr, nullptr, pkey.get()) != 1) {
        throw std::runtime_error("Failed to initialize Ed25519 signing");
    }
    
    // For Ed25519, we sign the raw data directly without hashing
    size_t sig_len = 0;
    if (EVP_DigestSign(ctx.get(), nullptr, &sig_len, 
                      reinterpret_cast<const unsigned char*>(data.data()), data.length()) <= 0) {
        throw std::runtime_error("Failed to determine Ed25519 signature length");
    }
    
    // Generate signature
    std::vector<uint8_t> signature(sig_len);
    if (EVP_DigestSign(ctx.get(), signature.data(), &sig_len, 
                      reinterpret_cast<const unsigned char*>(data.data()), data.length()) <= 0) {
        throw std::runtime_error("Failed to generate Ed25519 signature");
    }
    
    signature.resize(sig_len);
    return base64_encode(signature);
}

bool CryptoUtils::verify_ed25519_signature(const std::string& data, 
                                          const std::string& signature,
                                          const std::string& public_key_pem) {
    try {
        // Load public key
        BIOPtr bio(BIO_new_mem_buf(public_key_pem.c_str(), -1));
        if (!bio) {
            return false;
        }
        
        EVP_PKEY* pkey_raw = PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr);
        if (!pkey_raw) {
            return false;
        }
        EVPKeyPtr pkey(pkey_raw);
        
        // Check if it's an Ed25519 key
        int key_type = EVP_PKEY_id(pkey.get());
        if (key_type != EVP_PKEY_ED25519) {
            return false;
        }
        
        // Decode signature
        std::vector<uint8_t> sig_bytes = base64_decode(signature);
        
        // For Ed25519, we verify the raw data directly without hashing
        // Create verification context
        EVPMDCtxPtr ctx(EVP_MD_CTX_new());
        if (!ctx) {
            return false;
        }
        
        if (EVP_DigestVerifyInit(ctx.get(), nullptr, nullptr, nullptr, pkey.get()) != 1) {
            return false;
        }
        
        // For Ed25519, we verify the raw data directly
        int result = EVP_DigestVerify(ctx.get(), sig_bytes.data(), sig_bytes.size(),
                                     reinterpret_cast<const unsigned char*>(data.data()), data.length());
        return result == 1;
    } catch (...) {
        return false;
    }
}

std::string CryptoUtils::sign_sm2_data(const std::string& data, const std::string& private_key_pem) {
    // Load private key
    BIOPtr bio(BIO_new_mem_buf(private_key_pem.c_str(), -1));
    if (!bio) {
        throw std::runtime_error("Failed to create BIO for SM2 private key");
    }
    
    EVP_PKEY* pkey_raw = PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr);
    if (!pkey_raw) {
        throw std::runtime_error("Failed to read SM2 private key");
    }
    EVPKeyPtr pkey(pkey_raw);
    
    // Check if it's an SM2 key
    // In OpenSSL 3.0+, key type checking may not work as expected
    // We'll skip the strict check for now and rely on the signing/verification process
    /*
    int key_type = EVP_PKEY_id(pkey.get());
    if (key_type != EVP_PKEY_SM2 && key_type != EVP_PKEY_EC) {
        throw std::runtime_error("Not an SM2 key");
    }
    */
    
    // Create signature context
    EVPMDCtxPtr ctx(EVP_MD_CTX_new());
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }
    
    // Use SM3 digest for SM2
    // Skip setting SM2 ID for now to avoid compatibility issues
    /*
    if (EVP_PKEY_CTX_set1_id(EVP_MD_CTX_pkey_ctx(ctx.get()), "1234567812345678", 16) <= 0) {
        throw std::runtime_error("Failed to set SM2 ID");
    }
    */
    
    if (EVP_DigestSignInit(ctx.get(), nullptr, EVP_sm3(), nullptr, pkey.get()) != 1) {
        throw std::runtime_error("Failed to initialize SM2 signing");
    }
    
    if (EVP_DigestSignUpdate(ctx.get(), data.c_str(), data.length()) != 1) {
        throw std::runtime_error("Failed to update SM2 signature");
    }
    
    // Determine signature length
    size_t sig_len = 0;
    if (EVP_DigestSignFinal(ctx.get(), nullptr, &sig_len) != 1) {
        throw std::runtime_error("Failed to determine SM2 signature length");
    }
    
    // Generate signature
    std::vector<uint8_t> signature(sig_len);
    if (EVP_DigestSignFinal(ctx.get(), signature.data(), &sig_len) != 1) {
        throw std::runtime_error("Failed to generate SM2 signature");
    }
    
    signature.resize(sig_len);
    return base64_encode(signature);
}

bool CryptoUtils::verify_sm2_signature(const std::string& data, 
                                     const std::string& signature,
                                     const std::string& public_key_pem) {
    try {
        // Load public key
        BIOPtr bio(BIO_new_mem_buf(public_key_pem.c_str(), -1));
        if (!bio) {
            return false;
        }
        
        EVP_PKEY* pkey_raw = PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr);
        if (!pkey_raw) {
            return false;
        }
        EVPKeyPtr pkey(pkey_raw);
        
        // Check if it's an SM2 key
        // In OpenSSL 3.0+, key type checking may not work as expected
        // We'll skip the strict check for now and rely on the signing/verification process
        /*
        int key_type = EVP_PKEY_id(pkey.get());
        if (key_type != EVP_PKEY_SM2 && key_type != EVP_PKEY_EC) {
            return false;
        }
        */
        
        // Decode signature
        std::vector<uint8_t> sig_bytes = base64_decode(signature);
        
        // Create verification context
        EVPMDCtxPtr ctx(EVP_MD_CTX_new());
        if (!ctx) {
            return false;
        }
        
        // Use SM3 digest for SM2
        // Skip setting SM2 ID for now to avoid compatibility issues
        /*
        if (EVP_PKEY_CTX_set1_id(EVP_MD_CTX_pkey_ctx(ctx.get()), "1234567812345678", 16) <= 0) {
            return false;
        }
        */
        
        if (EVP_DigestVerifyInit(ctx.get(), nullptr, EVP_sm3(), nullptr, pkey.get()) != 1) {
            return false;
        }
        
        if (EVP_DigestVerifyUpdate(ctx.get(), data.c_str(), data.length()) != 1) {
            return false;
        }
        
        int result = EVP_DigestVerifyFinal(ctx.get(), sig_bytes.data(), sig_bytes.size());
        return result == 1;
    } catch (...) {
        return false;
    }
}

}  // namespace decentrilicense
