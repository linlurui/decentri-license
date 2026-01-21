#include "decentrilicense/decentrilicense_client.hpp"
#include "decentrilicense/environment_checker.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <array>
#include <cstring>
#include <curl/curl.h>

// Debug macro - set to 1 to enable debug output, 0 to disable
#define DECENTRILICENSE_DEBUG 0

namespace decentrilicense {

// HTTP response write callback for libcurl
static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), realsize);
    return realsize;
}

// Get device ID using system commands
std::string get_device_id() {
    // Try multiple methods to get a unique device identifier

    // Method 1: Try to get hardware UUID on macOS
    std::array<char, 128> buffer;
    std::string result;

    // Use ioreg to get hardware UUID
    FILE* pipe = popen("ioreg -rd1 -c IOPlatformExpertDevice | awk '/IOPlatformUUID/ { split($0, line, \"\\\"\"); printf(\"%s\\n\", line[4]); }'", "r");
    if (pipe) {
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
        pclose(pipe);

        // Clean up the result
        result.erase(result.find_last_not_of(" \n\r\t") + 1);
        if (!result.empty()) {
            return result;
        }
    }

    // Method 2: Fallback to hostname + process ID
    pipe = popen("hostname", "r");
    if (pipe) {
        std::string hostname;
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            hostname += buffer.data();
        }
        pclose(pipe);

        hostname.erase(hostname.find_last_not_of(" \n\r\t") + 1);
        if (!hostname.empty()) {
            // Append process ID to make it more unique
            return hostname + "-" + std::to_string(getpid());
        }
    }

    // Method 3: Ultimate fallback
    return "device-" + std::to_string(getpid()) + "-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
}

DecentriLicenseClient::DecentriLicenseClient(const ClientConfig& config)
    : config_(config), current_mode_(ConnectionMode::OFFLINE), token_manager_(std::make_unique<TokenManager>()) {

    // Always initialize network components for potential degradation
    initialize_network_components();
}

void DecentriLicenseClient::set_product_public_key(const std::string& product_public_key_pem) {
    product_public_key_pem_ = product_public_key_pem;
}

DecentriLicenseClient::~DecentriLicenseClient() {
    stop();
}

void DecentriLicenseClient::start() {
    if (running_) {
        return;
    }

    running_ = true;

    // Start smart degradation process in background
    degradation_thread_ = std::thread([this]() {
        perform_smart_degradation();
    });

    // Start periodic tasks
    periodic_thread_ = std::thread([this]() {
        while (running_) {
            // Broadcast discovery message (every 30 seconds)
            static auto last_broadcast = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_broadcast).count() >= 30) {
                broadcast_discovery_message();
                last_broadcast = now;
            }

            // Check token expiration
            token_manager_->check_expiration();

            // Perform other periodic tasks
            std::this_thread::sleep_for(std::chrono::seconds(10)); // Check every 10 seconds
        }
    });
}

void DecentriLicenseClient::initialize_network_components() {
    // Use fixed ports for P2P discovery - all clients must use the same ports
    // This ensures broadcast discovery works across all instances
    try {
        network_manager_ = std::make_unique<NetworkManager>(config_.udp_port, config_.tcp_port);
        election_manager_ = std::make_unique<ElectionManager>("device_id", config_.license_code);

        // Set up network manager callbacks
        network_manager_->set_message_callback([this](const NetworkMessage& msg, const std::string& from_address) {
            handle_message(msg, from_address);
        });

        // Set up token manager callback
        token_manager_->set_token_callback([this](TokenStatus status, const std::optional<Token>& token) {
            handle_token_change(status, token);
        });

        // Set up election manager callbacks
        election_manager_->set_election_callback([this](DeviceState new_state, const std::string& coordinator_id) {
            handle_election_result(new_state == DeviceState::COORDINATOR);
        });

        std::cout << "DecentriLicense: Network components initialized on fixed ports UDP:" << config_.udp_port << " TCP:" << config_.tcp_port << std::endl;
        std::cout << "DecentriLicense: P2P discovery will use UDP broadcast on port " << config_.udp_port << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "DecentriLicense: Failed to initialize network components: " << e.what() << std::endl;
        std::cerr << "DecentriLicense: This may be due to port conflict. Ports UDP:" << config_.udp_port << " TCP:" << config_.tcp_port << " may be in use by another application." << std::endl;
        std::cerr << "DecentriLicense: Falling back to offline mode." << std::endl;
        // Continue without network components - will operate in offline mode
    }
}

void DecentriLicenseClient::broadcast_discovery_message() {
    if (!network_manager_) return;

    // Get current token's ID for discovery
    auto current_token = token_manager_->get_current_token();
    if (!current_token.has_value()) return;

    DiscoveryMessage discovery;
    discovery.device_id = get_device_id(); // Use actual device ID
    discovery.token_id = current_token->token_id; // Use token_id for security
    discovery.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    network_manager_->broadcast_discovery(discovery);
}

void DecentriLicenseClient::handle_discovery_message(const NetworkMessage& msg, const std::string& from_address) {
    try {
        DiscoveryMessage discovery = DiscoveryMessage::from_json(msg.payload);

        // Ignore our own discovery messages
        if (discovery.device_id == get_device_id()) {
            return;
        }

        std::cout << "DecentriLicense: Discovered device " << discovery.device_id
                  << " with token " << discovery.token_id << " at " << from_address << std::endl;

        // Store discovered device info
        {
            std::lock_guard<std::mutex> lock(devices_mutex_);
            discovered_devices_[discovery.device_id] = {
                discovery.token_id,
                from_address,
                discovery.timestamp
            };
        }

        // Check for conflicts using token_id
        if (check_token_conflict(discovery.token_id)) {
            std::cout << "DecentriLicense: Conflict detected with device " << discovery.device_id << std::endl;

            // Register peer for election
            PeerDevice peer;
            peer.device_id = discovery.device_id;
            peer.token_id = discovery.token_id;
            peer.ip_address = from_address;
            peer.tcp_port = 23325; // Fixed TCP port
            peer.timestamp = discovery.timestamp;
            peer.last_seen = std::chrono::system_clock::now();

            if (election_manager_) {
                election_manager_->register_peer(peer);
                resolve_conflicts(discovery.token_id);
            }
        }

        // Send discovery response
        send_discovery_response(from_address, discovery);

    } catch (const std::exception& e) {
        std::cerr << "DecentriLicense: Failed to parse discovery message: " << e.what() << std::endl;
    }
}

void DecentriLicenseClient::handle_discovery_response(const NetworkMessage& msg, const std::string& from_address) {
    try {
        DiscoveryMessage response = DiscoveryMessage::from_json(msg.payload);

        std::cout << "DecentriLicense: Received discovery response from " << response.device_id
                  << " at " << from_address << std::endl;

        // Update device info
        {
            std::lock_guard<std::mutex> lock(devices_mutex_);
            discovered_devices_[response.device_id] = {
                response.token_id,
                from_address,
                response.timestamp
            };
        }

    } catch (const std::exception& e) {
        std::cerr << "DecentriLicense: Failed to parse discovery response: " << e.what() << std::endl;
    }
}

void DecentriLicenseClient::send_discovery_response(const std::string& to_address, const DiscoveryMessage& original_discovery) {
    if (!network_manager_) return;

    DiscoveryMessage response;
    response.device_id = ""; // Don't expose device ID in response for security
    response.token_id = ""; // We don't expose our token_id in response for security

    // Only respond if we have a token
    auto current_token = token_manager_->get_current_token();
    if (current_token.has_value()) {
        response.token_id = current_token->token_id;
    }

    response.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    NetworkMessage msg;
    msg.type = MessageType::DISCOVERY_RESPONSE;
    msg.payload = response.to_json();

    network_manager_->send_message(msg, to_address);
}

void DecentriLicenseClient::send_token_ack(const std::string& to_address, const std::string& token_id) {
    if (!network_manager_) return;

    // Create acknowledgment message
    std::string ack_payload = "{\"token_id\":\"" + token_id + "\",\"status\":\"accepted\"}";

    NetworkMessage msg;
    msg.type = MessageType::TOKEN_ACK;
    msg.payload = ack_payload;

    network_manager_->send_message(msg, to_address);
}

void DecentriLicenseClient::handle_token_transfer(const NetworkMessage& msg, const std::string& from_address) {
    try {
        // Parse token from message payload (assuming it's JSON serialized token)
        Token transferred_token;
        try {
            transferred_token = Token::from_json(msg.payload);
        } catch (const std::exception& e) {
            std::cerr << "DecentriLicense: Failed to parse transferred token: " << e.what() << std::endl;
            return;
        }

        std::cout << "DecentriLicense: Received token transfer from " << from_address
                  << " for token " << transferred_token.token_id << std::endl;

        // Verify token authenticity and integrity
        if (!verify_token_with_environment_check(transferred_token)) {
            std::cerr << "DecentriLicense: Token transfer verification failed" << std::endl;
            return;
        }

        // Accept the transferred token
        if (token_manager_->set_token(transferred_token, "")) {
            std::cout << "DecentriLicense: Token transfer accepted and activated" << std::endl;

            // Send acknowledgment
            send_token_ack(from_address, transferred_token.token_id);
        } else {
            std::cerr << "DecentriLicense: Failed to accept transferred token" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "DecentriLicense: Token transfer handling error: " << e.what() << std::endl;
    }
}

StateChainComparisonResult DecentriLicenseClient::compare_state_chains(const Token& new_token, const Token& current_token) {
    // Compare state indices first - higher index wins
    if (new_token.state_index > current_token.state_index) {
        return StateChainComparisonResult::CONFLICT_WIN;
    } else if (new_token.state_index < current_token.state_index) {
        return StateChainComparisonResult::CONFLICT_LOSE;
    }

    // State indices are equal, compare timestamps
    // Newer timestamp (more recent updates) wins
    if (new_token.issue_time > current_token.issue_time) {
        return StateChainComparisonResult::CONFLICT_WIN;
    } else if (new_token.issue_time < current_token.issue_time) {
        return StateChainComparisonResult::CONFLICT_LOSE;
    }

    // Both state index and timestamp are equal
    // This is a true conflict - use random decision
    return StateChainComparisonResult::CONFLICT_RANDOM;
}

void DecentriLicenseClient::perform_smart_degradation() {
    std::cout << "DecentriLicense: Starting smart degradation process..." << std::endl;

    // Step 1: Try WAN registry first (if configured)
    if (!config_.registry_server_url.empty()) {
        std::cout << "DecentriLicense: Attempting WAN registry connection..." << std::endl;
        if (try_wan_connection()) {
            current_mode_ = ConnectionMode::WAN_REGISTRY;
            std::cout << "DecentriLicense: Successfully connected to WAN registry" << std::endl;
            return;
        }
        std::cout << "DecentriLicense: WAN registry connection failed, degrading to LAN P2P" << std::endl;
    }

    // Step 2: Try LAN P2P
    std::cout << "DecentriLicense: Attempting LAN P2P connection..." << std::endl;
    if (try_lan_p2p_connection()) {
        current_mode_ = ConnectionMode::LAN_P2P;
        std::cout << "DecentriLicense: Successfully connected via LAN P2P" << std::endl;
        return;
    }
    std::cout << "DecentriLicense: LAN P2P connection failed, falling back to offline mode" << std::endl;

    // Step 3: Fallback to offline mode
    fallback_to_offline();
    current_mode_ = ConnectionMode::OFFLINE;
    std::cout << "DecentriLicense: Operating in offline mode" << std::endl;
}

bool DecentriLicenseClient::try_wan_connection() {
    if (config_.registry_server_url.empty()) {
        return false;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    std::string url = config_.registry_server_url + "/api/health";
    std::string response_data;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK && http_code == 200) {
        std::cout << "DecentriLicense: WAN registry server is healthy" << std::endl;
        return true;
    }

    std::cout << "DecentriLicense: WAN registry server connection failed" << std::endl;
    return false;
}

bool DecentriLicenseClient::try_lan_p2p_connection() {
    try {
        if (network_manager_) {
            network_manager_->start();
        }

        if (election_manager_) {
            election_manager_->start_election();
        }

        // Wait for network discovery to complete
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // Check if we have discovered any peers or have network connectivity
        // A successful LAN connection means we can discover and communicate with peers
        std::lock_guard<std::mutex> lock(devices_mutex_);
        bool has_peers = !discovered_devices_.empty();

        if (has_peers) {
            std::cout << "DecentriLicense: LAN P2P connected - discovered " << discovered_devices_.size() << " peer(s)" << std::endl;
        } else {
            std::cout << "DecentriLicense: LAN P2P available but no peers discovered" << std::endl;
        }

        return true;
    } catch (...) {
        return false;
    }
}

void DecentriLicenseClient::fallback_to_offline() {
    // Stop network components
    if (network_manager_) {
        network_manager_->stop();
    }

    // Offline mode - no network coordination
    std::cout << "DecentriLicense: Offline mode - manual token input required" << std::endl;
}

void DecentriLicenseClient::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    // Stop network components if they exist
    if (network_manager_) {
        network_manager_->stop();
    }

    // Join threads
    if (degradation_thread_.joinable()) {
        degradation_thread_.join();
    }

    if (periodic_thread_.joinable()) {
        periodic_thread_.join();
    }
}

bool DecentriLicenseClient::check_token_conflict(const std::string& token_id) {
    switch (current_mode_) {
        case ConnectionMode::WAN_REGISTRY:
            return detect_wan_conflicts(token_id);
        case ConnectionMode::LAN_P2P:
            return detect_lan_conflicts(token_id);
        case ConnectionMode::OFFLINE:
            // In offline mode, we can't detect conflicts, so assume no conflict
            // The conflict will be detected later when trying to use the same token
            return false;
        default:
            return false;
    }
}

bool DecentriLicenseClient::detect_wan_conflicts(const std::string& token_id) {
    if (config_.registry_server_url.empty()) {
        return false;
    }

    auto current_token = token_manager_->get_current_token();
    if (!current_token.has_value()) {
        return false;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    std::string url = config_.registry_server_url + "/api/licenses/" +
                      current_token->license_code + "/holder";
    std::string response_data;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    // If query fails or no holder found, assume no conflict
    if (res != CURLE_OK || http_code != 200) {
        return false;
    }

    // Parse JSON response to check device_id
    // Simple check: if response contains "device_id" and it's not our device
    std::string my_device_id = get_device_id();
    size_t device_id_pos = response_data.find("\"device_id\"");
    if (device_id_pos != std::string::npos) {
        // Check if the device_id in response is different from ours
        if (response_data.find(my_device_id) == std::string::npos) {
            std::cout << "DecentriLicense: WAN conflict detected - license held by another device" << std::endl;
            return true;
        }
    }

    return false;
}

bool DecentriLicenseClient::detect_lan_conflicts(const std::string& token_id) {
    if (!election_manager_) {
        return false;
    }

    // Check discovered_devices_ for conflicts with the same token_id
    std::lock_guard<std::mutex> lock(devices_mutex_);

    std::string my_device_id = get_device_id();
    for (const auto& [device_id, device_info] : discovered_devices_) {
        if (device_id != my_device_id && device_info.token_id == token_id) {
            std::cout << "DecentriLicense: LAN conflict detected with device " << device_id << std::endl;
            return true;
        }
    }

    return false;
}

void DecentriLicenseClient::resolve_conflicts(const std::string& token_id) {
    // Resolution depends on current mode
    switch (current_mode_) {
        case ConnectionMode::WAN_REGISTRY:
            // WAN: Report to registry, let registry decide
            break;
        case ConnectionMode::LAN_P2P:
            // LAN: Participate in election
            if (election_manager_) {
                election_manager_->start_election();
            }
            break;
        case ConnectionMode::OFFLINE:
            // Offline: No resolution possible, rely on state chain versioning
            break;
    }
}

ActivationResult DecentriLicenseClient::activate_license(const std::string& license_code) {
    ActivationResult result;

    // Get current token for conflict checking
    auto current_token = token_manager_->get_current_token();
    if (!current_token.has_value()) {
        result.success = false;
        result.message = "No token available for activation";
        return result;
    }

    // First, check for conflicts using token_id
    if (check_token_conflict(current_token->token_id)) {
        resolve_conflicts(current_token->token_id);

        // After conflict resolution, check again
        if (check_token_conflict(current_token->token_id)) {
            result.success = false;
            result.message = "License conflict detected - another device is using this license";
            return result;
        }
    }

    // Proceed with activation based on current mode
    switch (current_mode_) {
        case ConnectionMode::WAN_REGISTRY:
        case ConnectionMode::LAN_P2P:
            // In network modes, use election-based activation
            if (election_manager_ && election_manager_->get_state() == DeviceState::COORDINATOR) {
                // Generate token locally
                Token token = token_manager_->generate_token(
                    "device_id",
                    license_code,
                    24 * 30, // 30 days default
                    "", // Private key would be loaded from config in a real implementation
                    SigningAlgorithm::RSA
                );

                // Set token
                if (token_manager_->set_token(token, "")) {
                    result.success = true;
                    result.message = "License activated successfully via network coordination";
                    result.token = token;
                } else {
                    result.success = false;
                    result.message = "Failed to set token";
                }
            } else {
                result.success = false;
                result.message = "Not authorized - lost election or conflict resolution";
            }
            break;

        case ConnectionMode::OFFLINE:
            result.success = false;
            result.message = "Cannot activate via election in offline mode. Use activate_with_token instead.";
            break;
    }

    return result;
}

ActivationResult DecentriLicenseClient::activate_with_token(const Token& token) {
    ActivationResult result;

#if DECENTRILICENSE_DEBUG
    std::cerr << "dl-core debug: activate_with_token called for token: " << token.token_id << std::endl;
#endif

    // Check if token is valid for our license code
    // Allow "AUTO" or "TEMP" as special values for automatic license code detection
#if DECENTRILICENSE_DEBUG
    std::cerr << "dl-core debug: checking license code - token: '" << token.license_code << "', config: '" << config_.license_code << "'" << std::endl;
#endif
    if (token.license_code != config_.license_code &&
        config_.license_code != "AUTO" &&
        config_.license_code != "TEMP") {
        result.success = false;
        result.message = "Token license code does not match client configuration";
        return result;
    }

    // Check for conflicts based on current mode
#if DECENTRILICENSE_DEBUG
    std::cerr << "dl-core debug: checking for conflicts, token_id: " << token.token_id << std::endl;
#endif
    bool has_conflict = check_token_conflict(token.token_id);
#if DECENTRILICENSE_DEBUG
    std::cerr << "dl-core debug: conflict check result: " << has_conflict << std::endl;
#endif

    if (has_conflict) {
        // If there's a conflict, compare state chains
        auto current_token = token_manager_->get_current_token();

        if (current_token.has_value()) {
            // Compare state chains using comprehensive logic
            StateChainComparisonResult comparison = compare_state_chains(token, *current_token);

            if (comparison == StateChainComparisonResult::CONFLICT_LOSE) {
                result.success = false;
                result.message = "State chain conflict - existing token has higher precedence";
                return result;
            } else if (comparison == StateChainComparisonResult::CONFLICT_RANDOM) {
                // Random decision for equal precedence
                // Use current time as random seed for simplicity
                unsigned int seed = static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count());
                bool allow = (seed % 2) == 0; // Simple random decision

                if (!allow) {
                    result.success = false;
                    result.message = "State chain conflict - equal precedence, service denied randomly";
                    return result;
                }
            }
            // If comparison == CONFLICT_WIN or NO_CONFLICT, continue with activation
        }

        // In network modes, still perform election for additional coordination
        if (current_mode_ != ConnectionMode::OFFLINE) {
            resolve_conflicts(token.token_id);
            // Note: We don't re-check conflict here as we've already done state chain comparison
        }
    }

    // Check if license code has been used before
    {
        std::lock_guard<std::mutex> lock(license_mutex_);
        if (used_license_codes_.find(token.license_code) != used_license_codes_.end()) {
            result.success = false;
            result.message = "License code has already been used and archived";
            return result;
        }
    }

    // Set token using product public key for verification
#if DECENTRILICENSE_DEBUG
    std::cerr << "dl-core debug: setting token with product_public_key_pem length: " << product_public_key_pem_.length() << std::endl;
    if (!product_public_key_pem_.empty()) {
        std::cerr << "dl-core debug: product_public_key_pem starts with: " << product_public_key_pem_.substr(0, 50) << "..." << std::endl;
    }
    std::cerr << "dl-core debug: calling token_manager_->set_token" << std::endl;
#endif
    try {
        if (token_manager_->set_token(token, product_public_key_pem_)) {
        result.success = true;
        result.message = "License activated successfully with offline token";
        result.token = token;

        // Archive the license code to prevent reuse
        {
            std::lock_guard<std::mutex> lock(license_mutex_);
            used_license_codes_.insert(token.license_code);
        }

        std::cout << "DecentriLicense: License code '" << token.license_code << "' has been archived" << std::endl;
    } else {
        result.success = false;
        result.message = "Failed to set offline token";
    }
    } catch (const std::exception& e) {
#if DECENTRILICENSE_DEBUG
        std::cerr << "dl-core debug: exception in activate_with_token: " << e.what() << std::endl;
#endif
        result.success = false;
        result.message = "Activation failed due to internal error";
    } catch (...) {
#if DECENTRILICENSE_DEBUG
        std::cerr << "dl-core debug: unknown exception in activate_with_token" << std::endl;
#endif
        result.success = false;
        result.message = "Activation failed due to internal error";
    }

    return result;
}

void DecentriLicenseClient::handle_discovery(const std::string& device_id, const std::string& address) {
    // Update election manager with discovered devices
    PeerDevice peer;
    peer.device_id = device_id;
    peer.ip_address = address;
    peer.tcp_port = 8889; // Default port
    peer.timestamp = 0; // Placeholder
    election_manager_->register_peer(peer);
}

void DecentriLicenseClient::handle_message(const NetworkMessage& msg, const std::string& from_address) {
    switch (msg.type) {
        case MessageType::DISCOVERY:
            handle_discovery_message(msg, from_address);
            break;

        case MessageType::DISCOVERY_RESPONSE:
            handle_discovery_response(msg, from_address);
            break;

        case MessageType::ELECTION_REQUEST:
        case MessageType::ELECTION_RESPONSE:
            if (election_manager_) {
                election_manager_->handle_message(msg, from_address);
            }
            break;

        case MessageType::TOKEN_TRANSFER:
            handle_token_transfer(msg, from_address);
            break;

        default:
            // Forward other messages to token manager
            token_manager_->handle_message(msg, from_address, "");
            break;
    }
}

void DecentriLicenseClient::handle_token_change(TokenStatus status, const std::optional<Token>& token) {
    // Handle token status changes
    switch (status) {
        case TokenStatus::ACTIVE:
            std::cout << "Token is now active" << std::endl;
            break;
        case TokenStatus::EXPIRED:
            std::cout << "Token has expired" << std::endl;
            break;
        case TokenStatus::TRANSFERRED:
            std::cout << "Token has been transferred" << std::endl;
            break;
        case TokenStatus::NONE:
            std::cout << "No token available" << std::endl;
            break;
    }
}

void DecentriLicenseClient::handle_election_result(bool is_coordinator) {
    if (is_coordinator) {
        std::cout << "This device is now the coordinator" << std::endl;
    } else {
        std::cout << "This device is now a follower" << std::endl;
    }
}

void DecentriLicenseClient::handle_coordinator_change(const std::string& coordinator_id) {
    std::cout << "Coordinator changed to: " << coordinator_id << std::endl;
}

bool DecentriLicenseClient::verify_token_with_environment_check(const Token& token) {
    // First verify the token signature
    bool signature_valid = token_manager_->verify_token(token, ""); // Public key would be loaded from config
    
    if (!signature_valid) {
        return false;
    }
    
    // Check environment hash if present
    if (!token.environment_hash.empty()) {
        bool environment_match = EnvironmentChecker::verify_environment_hash(token.environment_hash);
        if (!environment_match) {
            // Environment doesn't match - this could be a warning or failure depending on policy
            std::cout << EnvironmentChecker::get_warning_message() << std::endl;
            // For now, we'll treat this as a verification failure
            // In a real implementation, this might be configurable
            return false;
        }
    }
    
    return true;
}

bool DecentriLicenseClient::verify_token_trust_chain(const Token& token) {
    if (!token_manager_) {
        return false;
    }
    return token_manager_->verify_token_trust_chain(token);
}

} // namespace decentrilicense