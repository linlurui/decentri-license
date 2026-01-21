#ifndef DECENTRILICENSE_ENVIRONMENT_CHECKER_HPP
#define DECENTRILICENSE_ENVIRONMENT_CHECKER_HPP

#include <string>

namespace decentrilicense {

/**
 * EnvironmentChecker - Provides environment-based anti-copy protection
 * 
 * This class generates a hash based on system environment information
 * (username, hostname) to help prevent simple copying of license tokens
 * between different systems.
 * 
 * Note: This is an enhancement feature, not an absolute security measure.
 * It can effectively prevent casual copying but cannot defend against
 * intentional hardware information spoofing.
 */
class EnvironmentChecker {
public:
    /**
     * Generate environment hash based on system information
     * @return SHA256 hash of environment information
     */
    static std::string generate_environment_hash();
    
    /**
     * Verify if the current environment matches the stored hash
     * @param stored_hash The environment hash stored in the token
     * @return true if environment matches or no verification needed
     */
    static bool verify_environment_hash(const std::string& stored_hash);
    
    /**
     * Get warning message for environment mismatch
     * @return Warning message string
     */
    static std::string get_warning_message();
};

} // namespace decentrilicense

#endif // DECENTRILICENSE_ENVIRONMENT_CHECKER_HPP