#include "decentrilicense/election_manager.hpp"
#include <chrono>
#include <algorithm>

namespace decentrilicense {

ElectionManager::ElectionManager(const std::string& device_id, const std::string& token_id)
    : device_id_(device_id)
    , token_id_(token_id)
    , state_(DeviceState::IDLE) {
    
    // Use current time as startup timestamp
    startup_timestamp_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    coordinator_id_ = device_id; // Initially, we are our own coordinator
}

void ElectionManager::start_election() {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    
    if (peers_.empty()) {
        // No peers, we are coordinator by default
        set_state(DeviceState::COORDINATOR);
        return;
    }
    
    set_state(DeviceState::ELECTING);
    
    // Compare with all peers
    bool we_win = true;
    for (const auto& [peer_id, peer] : peers_) {
        if (!compare_priority(peer_id, peer.timestamp)) {
            we_win = false;
            break;
        }
    }
    
    if (we_win) {
        set_state(DeviceState::COORDINATOR);
        coordinator_id_ = device_id_;
    } else {
        set_state(DeviceState::FOLLOWER);
        // Find the coordinator (highest priority peer)
        std::string highest_id = device_id_;
        uint64_t highest_ts = startup_timestamp_;
        
        for (const auto& [peer_id, peer] : peers_) {
            if (peer_id > highest_id || 
                (peer_id == highest_id && peer.timestamp < highest_ts)) {
                highest_id = peer_id;
                highest_ts = peer.timestamp;
            }
        }
        coordinator_id_ = highest_id;
    }
}

void ElectionManager::register_peer(const PeerDevice& peer) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    peers_[peer.device_id] = peer;
}

bool ElectionManager::handle_election_request(const std::string& peer_id, uint64_t peer_timestamp) {
    // Compare priority: higher device_id wins, or earlier timestamp if IDs are equal
    bool we_win = compare_priority(peer_id, peer_timestamp);
    
    if (!we_win) {
        set_state(DeviceState::FOLLOWER);
        coordinator_id_ = peer_id;
    }
    
    return we_win;
}

void ElectionManager::handle_election_response(const std::string& peer_id, bool peer_wins) {
    if (peer_wins) {
        set_state(DeviceState::FOLLOWER);
        coordinator_id_ = peer_id;
    }
}

void ElectionManager::handle_message(const NetworkMessage& msg, const std::string& from_address) {
    // For now, we'll just print a message to indicate the method exists
    // In a real implementation, this would handle election-related messages
}

void ElectionManager::set_election_callback(ElectionResultCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    election_callback_ = std::move(callback);
}

std::string ElectionManager::get_coordinator_id() const {
    return coordinator_id_;
}

void ElectionManager::cleanup_inactive_peers(std::chrono::seconds timeout) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    
    auto now = std::chrono::system_clock::now();
    
    for (auto it = peers_.begin(); it != peers_.end(); ) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.last_seen);
        if (elapsed > timeout) {
            it = peers_.erase(it);
        } else {
            ++it;
        }
    }
}

void ElectionManager::set_state(DeviceState new_state) {
    DeviceState old_state = state_.exchange(new_state);
    
    if (old_state != new_state) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (election_callback_) {
            election_callback_(new_state, coordinator_id_);
        }
    }
}

bool ElectionManager::compare_priority(const std::string& peer_id, uint64_t peer_timestamp) const {
    // Higher device_id has higher priority
    // If device_ids are equal, earlier timestamp (lower value) has higher priority
    
    if (device_id_ > peer_id) {
        return true;
    } else if (device_id_ < peer_id) {
        return false;
    } else {
        // Same ID (shouldn't happen), use timestamp
        return startup_timestamp_ < peer_timestamp;
    }
}

} // namespace decentrilicense