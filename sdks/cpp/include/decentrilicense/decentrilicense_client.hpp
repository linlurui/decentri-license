#ifndef DECENTRILICENSE_CLIENT_HPP
#define DECENTRILICENSE_CLIENT_HPP

#include "network_manager.hpp"
#include "election_manager.hpp"
#include "token_manager.hpp"
#include "crypto_utils.hpp"
#include <string>
#include <memory>
#include <future>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <random>

namespace decentrilicense {

// Forward declarations
class NetworkManager;
class ElectionManager;
class TokenManager;

// Activation result
struct ActivationResult {
    bool success;
    std::string message;
    std::optional<Token> token;
};

// Connection mode enum
enum class ConnectionMode {
    WAN_REGISTRY,    // 广域网注册中心优先
    LAN_P2P,        // 局域网P2P
    OFFLINE         // 离线模式
};

// State chain comparison result
enum class StateChainComparisonResult {
    NO_CONFLICT,        // No conflict, can activate
    CONFLICT_WIN,       // Conflict but our token wins
    CONFLICT_LOSE,      // Conflict and our token loses
    CONFLICT_RANDOM     // Conflict with equal precedence, random decision needed
};

// Discovered device information
struct DiscoveredDevice {
    std::string token_id;
    std::string address;
    uint64_t last_seen;
};

// Client configuration
struct ClientConfig {
    std::string license_code;
    ConnectionMode preferred_mode = ConnectionMode::WAN_REGISTRY; // 首选连接模式
    uint16_t udp_port = 13325;  // Fixed UDP port for P2P discovery (uncommon port)
    uint16_t tcp_port = 23325;  // Fixed TCP port for P2P communication (uncommon port)
    std::string registry_server_url;          // Optional, for WAN coordination

    // Key generation options
    bool generate_keys_automatically = true;  // If true, generate keys automatically
    std::string private_key_file = "";        // Path to private key file (PEM format)
    std::string public_key_file = "";         // Path to public key file (PEM format)
};

class DecentriLicenseClient {
public:
    explicit DecentriLicenseClient(const ClientConfig& config);
    ~DecentriLicenseClient();

    // Non-copyable
    DecentriLicenseClient(const DecentriLicenseClient&) = delete;
    DecentriLicenseClient& operator=(const DecentriLicenseClient&) = delete;

    /**
     * Set product public key for token verification
     */
    void set_product_public_key(const std::string& product_public_key_pem);

    /**
     * Start the client with smart degradation
     */
    void start();

    /**
     * Stop the client
     */
    void stop();

    /**
     * Get current connection mode
     */
    ConnectionMode get_connection_mode() const { return current_mode_; }

    /**
     * Check if token conflicts with other devices
     * @param token_id The token ID to check
     * @return true if conflict detected
     */
    bool check_token_conflict(const std::string& token_id);

    /**
     * Activate a license with smart degradation
     * @param license_code The license code to activate
     * @return Activation result
     */
    ActivationResult activate_license(const std::string& license_code);

    /**
     * Compare state chains to resolve conflicts
     * @param new_token The new token to compare
     * @param current_token The current active token
     * @return Comparison result indicating precedence
     */
    StateChainComparisonResult compare_state_chains(const Token& new_token, const Token& current_token);

    /**
     * Activate a license with offline token
     * @param token The offline token to activate
     * @return Activation result
     */
    ActivationResult activate_with_token(const Token& token);
    
    /**
     * Verify token with environment check
     * @param token The token to verify
     * @return true if token is valid and environment matches (if applicable)
     */
    bool verify_token_with_environment_check(const Token& token);

    /**
     * Verify token trust chain
     * @param token The token to verify
     * @return true if token trust chain is valid
     */
    bool verify_token_trust_chain(const Token& token);
    
private:
    // Network initialization
    void initialize_network_components();

    // Discovery and messaging
    void broadcast_discovery_message();
    void handle_discovery_message(const NetworkMessage& msg, const std::string& from_address);
    void handle_discovery_response(const NetworkMessage& msg, const std::string& from_address);
    void send_discovery_response(const std::string& to_address, const DiscoveryMessage& original_discovery);
    void handle_token_transfer(const NetworkMessage& msg, const std::string& from_address);
    void send_token_ack(const std::string& to_address, const std::string& token_id);

    // Smart degradation methods
    void perform_smart_degradation();
    bool try_wan_connection();
    bool try_lan_p2p_connection();
    void fallback_to_offline();

    // Conflict detection
    bool detect_wan_conflicts(const std::string& token_id);
    bool detect_lan_conflicts(const std::string& token_id);
    void resolve_conflicts(const std::string& token_id);

    // Event handlers
    void handle_discovery(const std::string& device_id, const std::string& address);
    void handle_message(const NetworkMessage& msg, const std::string& from_address);
    void handle_token_change(TokenStatus status, const std::optional<Token>& token);
    void handle_election_result(bool is_coordinator);
    void handle_coordinator_change(const std::string& coordinator_id);

    ClientConfig config_;
    ConnectionMode current_mode_;
    std::unique_ptr<NetworkManager> network_manager_;
    std::unique_ptr<ElectionManager> election_manager_;
    std::unique_ptr<TokenManager> token_manager_;

    // Product public key for token verification
    std::string product_public_key_pem_;

    // Discovered devices on the network
    std::unordered_map<std::string, DiscoveredDevice> discovered_devices_;
    std::mutex devices_mutex_;

    // Used license codes (archived after activation)
    std::unordered_set<std::string> used_license_codes_;
    std::mutex license_mutex_;

    std::atomic<bool> running_{false};
    std::thread periodic_thread_;
    std::thread degradation_thread_;
};

} // namespace decentrilicense

#endif // DECENTRILICENSE_CLIENT_HPP