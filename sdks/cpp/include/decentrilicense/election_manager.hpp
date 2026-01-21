#ifndef DECENTRILICENSE_ELECTION_MANAGER_HPP
#define DECENTRILICENSE_ELECTION_MANAGER_HPP

#include <string>
#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <chrono>
#include "network_manager.hpp"

namespace decentrilicense {

// Device state in the election process
enum class DeviceState {
    IDLE,           // Not participating in election
    DISCOVERING,    // Broadcasting discovery
    ELECTING,       // Participating in election
    COORDINATOR,    // Won election, is coordinator
    FOLLOWER        // Lost election, following coordinator
};

// Peer device information
struct PeerDevice {
    std::string device_id;
    std::string token_id;  // Token ID for conflict detection
    std::string ip_address;
    uint16_t tcp_port;
    uint64_t timestamp;
    std::chrono::system_clock::time_point last_seen;
};

// Election result callback
using ElectionResultCallback = std::function<void(DeviceState new_state, const std::string& coordinator_id)>;

/**
 * ElectionManager - Implements simplified Bully algorithm for coordinator election
 * 
 * Algorithm:
 * 1. When conflict detected (same license_code), initiate election
 * 2. Device with larger device_id (or earlier timestamp) wins
 * 3. Winner becomes coordinator and broadcasts result
 * 4. Losers become followers
 * 
 * Thread-safe using atomic operations and mutexes
 */
class ElectionManager {
public:
    ElectionManager(const std::string& device_id, const std::string& token_id);
    ~ElectionManager() = default;
    
    // Non-copyable
    ElectionManager(const ElectionManager&) = delete;
    ElectionManager& operator=(const ElectionManager&) = delete;
    
    /**
     * Start election process
     * Called when a peer with same license is discovered
     */
    void start_election();
    
    /**
     * Register discovered peer device
     * @param peer Peer device information
     */
    void register_peer(const PeerDevice& peer);
    
    /**
     * Handle election request from peer
     * @param peer_id Peer device ID
     * @param peer_timestamp Peer startup timestamp
     * @return true if we win, false if peer wins
     */
    bool handle_election_request(const std::string& peer_id, uint64_t peer_timestamp);
    
    /**
     * Handle election response from peer
     * @param peer_id Peer device ID
     * @param peer_wins true if peer claims victory
     */
    void handle_election_response(const std::string& peer_id, bool peer_wins);
    
    /**
     * Handle incoming network message
     * @param msg Network message
     * @param from_address Sender address
     */
    void handle_message(const NetworkMessage& msg, const std::string& from_address);
    
    /**
     * Set callback for election result
     */
    void set_election_callback(ElectionResultCallback callback);
    
    /**
     * Get current device state
     */
    DeviceState get_state() const { return state_.load(); }
    
    /**
     * Get coordinator device ID
     */
    std::string get_coordinator_id() const;
    
    /**
     * Get device ID
     */
    const std::string& get_device_id() const { return device_id_; }
    
    /**
     * Get startup timestamp
     */
    uint64_t get_timestamp() const { return startup_timestamp_; }
    
    /**
     * Remove inactive peers (no heartbeat)
     */
    void cleanup_inactive_peers(std::chrono::seconds timeout = std::chrono::seconds(30));

private:
    void set_state(DeviceState new_state);
    bool compare_priority(const std::string& peer_id, uint64_t peer_timestamp) const;
    
    std::string device_id_;
    std::string token_id_;
    uint64_t startup_timestamp_;
    
    std::atomic<DeviceState> state_;
    std::string coordinator_id_;
    
    std::map<std::string, PeerDevice> peers_;
    mutable std::mutex peers_mutex_;
    
    ElectionResultCallback election_callback_;
    mutable std::mutex callback_mutex_;
};

} // namespace decentrilicense

#endif // DECENTRILICENSE_ELECTION_MANAGER_HPP