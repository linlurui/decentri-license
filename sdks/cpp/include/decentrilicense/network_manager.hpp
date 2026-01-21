#ifndef DECENTRILICENSE_NETWORK_MANAGER_HPP
#define DECENTRILICENSE_NETWORK_MANAGER_HPP

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <cstdint>
#include <asio.hpp>
#include <atomic>

namespace decentrilicense {

// Message types for protocol
enum class MessageType : uint8_t {
    DISCOVERY = 0x01,
    DISCOVERY_RESPONSE = 0x02,
    ELECTION_REQUEST = 0x03,
    ELECTION_RESPONSE = 0x04,
    TOKEN_TRANSFER = 0x05,
    TOKEN_ACK = 0x06,
    HEARTBEAT = 0x07
};

// Network message structure
struct NetworkMessage {
    MessageType type;
    std::string payload;
    
    // Serialize to binary format: [4-byte length][1-byte type][payload]
    std::vector<uint8_t> serialize() const;
    
    // Deserialize from binary format
    static NetworkMessage deserialize(const std::vector<uint8_t>& data);
};

// Discovery message payload - using token_id for security
struct DiscoveryMessage {
    std::string device_id;
    std::string token_id;     // Use token_id instead of license_code for uniqueness and security
    uint64_t timestamp;

    std::string to_json() const;
    static DiscoveryMessage from_json(const std::string& json);
};

// Callback types
using MessageCallback = std::function<void(const NetworkMessage&, const std::string& from_address)>;
using ErrorCallback = std::function<void(const std::string& error_msg)>;

/**
 * NetworkManager - Manages UDP broadcast discovery and TCP point-to-point communication
 * 
 * Features:
 * - UDP broadcast for LAN device discovery (255.255.255.255)
 * - TCP connections for reliable data transfer (elections, tokens)
 * - Asynchronous I/O with thread-safe callbacks
 * - Cross-platform socket handling (Windows/Unix)
 */
class NetworkManager {
public:
    explicit NetworkManager(uint16_t udp_port = 8888, uint16_t tcp_port = 8889);
    ~NetworkManager();
    
    // Non-copyable
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;
    
    /**
     * Start the network manager
     * Begins listening for UDP broadcasts and TCP connections
     */
    void start();
    
    /**
     * Stop the network manager
     * Closes all sockets and stops I/O operations
     */
    void stop();
    
    /**
     * Check if the network manager is running
     */
    bool is_running() const;
    
    /**
     * Broadcast discovery message on LAN
     * @param discovery Discovery message to broadcast
     */
    void broadcast_discovery(const DiscoveryMessage& discovery);
    
    /**
     * Broadcast message to all devices on LAN
     * @param message Message to broadcast
     */
    void broadcast_message(const NetworkMessage& message);
    
    /**
     * Send TCP message to specific peer
     * @param address Peer IP address
     * @param port Peer TCP port
     * @param message Message to send
     */
    void send_tcp_message(const std::string& address, uint16_t port, const NetworkMessage& message);
    
    /**
     * Send message to specific peer
     * @param message Message to send
     * @param address Peer IP address
     */
    void send_message(const NetworkMessage& message, const std::string& address);
    
    /**
     * Set callback for received messages
     * @param callback Function to call when message received
     */
    void set_message_callback(MessageCallback callback);
    
    /**
     * Set callback for errors
     * @param callback Function to call on errors
     */
    void set_error_callback(ErrorCallback callback);
    
    /**
     * Get local IP address
     */
    std::string get_local_address() const;
    
    /**
     * Get TCP listening port
     */
    uint16_t get_tcp_port() const { return tcp_port_; }

private:
    void start_udp_receive();
    void start_tcp_accept();
    void handle_udp_receive(const asio::error_code& error, size_t bytes_transferred);
    void handle_tcp_accept(std::shared_ptr<asio::ip::tcp::socket> socket, const asio::error_code& error);
    void handle_tcp_read(std::shared_ptr<asio::ip::tcp::socket> socket, 
                        std::shared_ptr<std::vector<uint8_t>> buffer,
                        const asio::error_code& error, 
                        size_t bytes_transferred);
    
    void run_io_context();
    
    asio::io_context io_context_;
    std::unique_ptr<asio::io_context::work> work_;
    std::unique_ptr<std::thread> io_thread_;
    std::atomic<bool> running_;
    
    // UDP for discovery
    uint16_t udp_port_;
    std::unique_ptr<asio::ip::udp::socket> udp_socket_;
    asio::ip::udp::endpoint udp_remote_endpoint_;
    std::array<char, 1024> udp_recv_buffer_;
    
    // TCP for reliable communication
    uint16_t tcp_port_;
    std::unique_ptr<asio::ip::tcp::acceptor> tcp_acceptor_;
    
    // Callbacks
    MessageCallback message_callback_;
    ErrorCallback error_callback_;
    
    // Thread safety
    mutable std::mutex callback_mutex_;
};

} // namespace decentrilicense

#endif // DECENTRILICENSE_NETWORK_MANAGER_HPP