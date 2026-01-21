#include "decentrilicense/environment_checker.hpp"
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <unistd.h>
#include <limits.h>

// Define HOST_NAME_MAX if not available
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/utsname.h>
#endif

namespace decentrilicense {

std::string EnvironmentChecker::generate_environment_hash() {
    std::ostringstream oss;
    
    // Get username
    const char* username = getenv("USER");
    if (!username) {
        username = getenv("USERNAME");
    }
    if (username) {
        oss << username;
    }
    
    // Get hostname
    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        oss << "|" << hostname;
    }
    
    // Create SHA256 hash
    std::string data = oss.str();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.c_str(), data.length());
    SHA256_Final(hash, &sha256);
    
    // Convert to hex string
    std::stringstream hexStream;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        hexStream << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return hexStream.str();
}

bool EnvironmentChecker::verify_environment_hash(const std::string& stored_hash) {
    if (stored_hash.empty()) {
        // No environment hash to verify
        return true;
    }
    
    std::string current_hash = generate_environment_hash();
    return stored_hash == current_hash;
}

std::string EnvironmentChecker::get_warning_message() {
    return "许可证环境已变更！当前环境与签发时不一致。";
}

} // namespace decentrilicense