#include "decentrilicense/network_manager.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <thread>
#include <mutex>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#endif

namespace decentrilicense {

// NetworkMessage implementation
std::vector<uint8_t> NetworkMessage::serialize() const {
    std::vector<uint8_t> result;
    uint32_t payload_size = static_cast<uint32_t>(payload.size());
    uint32_t total_size = payload_size + 1; // 1 byte for type
    
    // Write length (4 bytes, network byte order)
    result.push_back((total_size >> 24) & 0xFF);
    result.push_back((total_size >> 16) & 0xFF);
    result.push_back((total_size >> 8) & 0xFF);
    result.push_back(total_size & 0xFF);
    
    // Write type (1 byte)
    result.push_back(static_cast<uint8_t>(type));
    
    // Write payload
    result.insert(result.end(), payload.begin(), payload.end());
    
    return result;
}

NetworkMessage NetworkMessage::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 5) {
        throw std::runtime_error("Message too short");
    }
    
    // Read length
    uint32_t total_size = (static_cast<uint32_t>(data[0]) << 24) |
                         (static_cast<uint32_t>(data[1]) << 16) |
                         (static_cast<uint32_t>(data[2]) << 8) |
                         static_cast<uint32_t>(data[3]);
    
    if (data.size() < total_size + 4) {
        throw std::runtime_error("Incomplete message");
    }
    
    NetworkMessage msg;
    msg.type = static_cast<MessageType>(data[4]);
    
    if (total_size > 1) {
        msg.payload.assign(data.begin() + 5, data.begin() + 4 + total_size);
    }
    
    return msg;
}

// DiscoveryMessage implementation (simple JSON-like format)
std::string DiscoveryMessage::to_json() const {
    std::ostringstream oss;
    oss << "{"
        << "\"device_id\":\"" << device_id << "\","
        << "\"token_id\":\"" << token_id << "\","
        << "\"timestamp\":" << timestamp
        << "}";
    return oss.str();
}

DiscoveryMessage DiscoveryMessage::from_json(const std::string& json) {
    DiscoveryMessage msg;
    // Simple parser (in production, use a real JSON library)
    size_t pos = 0;
    
    // Extract device_id
    pos = json.find("\"device_id\":\"");
    if (pos != std::string::npos) {
        pos += 13;
        size_t end = json.find("\"", pos);
        msg.device_id = json.substr(pos, end - pos);
    }
    
    // Extract token_id (changed from license_code)
    pos = json.find("\"token_id\":\"");
    if (pos != std::string::npos) {
        pos += 12;
        size_t end = json.find("\"", pos);
        msg.token_id = json.substr(pos, end - pos);
    }
    
    // Extract timestamp
    pos = json.find("\"timestamp\":");
    if (pos != std::string::npos) {
        pos += 12;
        msg.timestamp = std::stoull(json.substr(pos));
    }
    
    return msg;
}

// NetworkManager implementation
NetworkManager::NetworkManager(uint16_t udp_port, uint16_t tcp_port)
    : udp_port_(udp_port), tcp_port_(tcp_port), running_(false) {
}

NetworkManager::~NetworkManager() {
    stop();
}

void NetworkManager::start() {
    running_ = true;
    
    // Create work guard to keep io_context running
    work_ = std::make_unique<asio::io_context::work>(io_context_);
    
    // Initialize UDP socket for broadcast
    try {
        udp_socket_ = std::make_unique<asio::ip::udp::socket>(
            io_context_, asio::ip::udp::endpoint(asio::ip::udp::v4(), udp_port_));
        
        // Enable broadcast
        asio::socket_base::broadcast option(true);
        udp_socket_->set_option(option);
        
        // Enable address reuse
        asio::socket_base::reuse_address reuse_option(true);
        udp_socket_->set_option(reuse_option);
        
        start_udp_receive();
    } catch (const std::exception& e) {
        if (error_callback_) {
            error_callback_(std::string("UDP socket error: ") + e.what());
        }
    }
    
    // Initialize TCP acceptor
    try {
        tcp_acceptor_ = std::make_unique<asio::ip::tcp::acceptor>(
            io_context_, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), tcp_port_));
        
        start_tcp_accept();
    } catch (const std::exception& e) {
        if (error_callback_) {
            error_callback_(std::string("TCP acceptor error: ") + e.what());
        }
    }
    
    // Start io_context in separate thread
    io_thread_ = std::make_unique<std::thread>([this]() { run_io_context(); });
}

void NetworkManager::stop() {
    running_ = false;
    
    if (udp_socket_) {
        udp_socket_->close();
    }
    
    if (tcp_acceptor_) {
        tcp_acceptor_->close();
    }
    
    work_.reset();
    
    if (io_thread_ && io_thread_->joinable()) {
        io_thread_->join();
    }
}

bool NetworkManager::is_running() const {
    return running_;
}

void NetworkManager::broadcast_discovery(const DiscoveryMessage& discovery) {
    if (!udp_socket_) return;
    
    NetworkMessage msg;
    msg.type = MessageType::DISCOVERY;
    msg.payload = discovery.to_json();
    
    auto data = msg.serialize();
    
    asio::ip::udp::endpoint broadcast_endpoint(
        asio::ip::address_v4::broadcast(), udp_port_);
    
    udp_socket_->async_send_to(
        asio::buffer(data), broadcast_endpoint,
        [this](const asio::error_code& error, size_t /*bytes_transferred*/) {
            if (error && error_callback_) {
                error_callback_("Broadcast failed: " + error.message());
            }
        });
}

void NetworkManager::broadcast_message(const NetworkMessage& message) {
    if (!udp_socket_) return;
    
    auto data = message.serialize();
    
    asio::ip::udp::endpoint broadcast_endpoint(
        asio::ip::address_v4::broadcast(), udp_port_);
    
    udp_socket_->async_send_to(
        asio::buffer(data), broadcast_endpoint,
        [this](const asio::error_code& error, size_t /*bytes_transferred*/) {
            if (error && error_callback_) {
                error_callback_("Broadcast failed: " + error.message());
            }
        });
}

void NetworkManager::send_tcp_message(const std::string& address, uint16_t port, const NetworkMessage& message) {
    auto socket = std::make_shared<asio::ip::tcp::socket>(io_context_);
    
    asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(address), port);
    
    socket->async_connect(endpoint,
        [this, socket, message](const asio::error_code& error) {
            if (error) {
                if (error_callback_) {
                    error_callback_("TCP connect failed: " + error.message());
                }
                return;
            }
            
            auto data = message.serialize();
            asio::async_write(*socket, asio::buffer(data),
                [this, socket](const asio::error_code& error, size_t /*bytes_transferred*/) {
                    if (error && error_callback_) {
                        error_callback_("TCP send failed: " + error.message());
                    }
                });
        });
}

void NetworkManager::send_message(const NetworkMessage& message, const std::string& address) {
    // For now, we'll just send via TCP to the specified address on our TCP port
    send_tcp_message(address, tcp_port_, message);
}

void NetworkManager::set_message_callback(MessageCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    message_callback_ = callback;
}

void NetworkManager::set_error_callback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    error_callback_ = callback;
}

std::string NetworkManager::get_local_address() const {
    try {
        // Get local IP address - simplified approach
        // In a real implementation, this would be more robust
        return "127.0.0.1";
    } catch (...) {
        return "127.0.0.1";
    }
}

void NetworkManager::start_udp_receive() {
    if (!udp_socket_) return;
    
    udp_socket_->async_receive_from(
        asio::buffer(udp_recv_buffer_), udp_remote_endpoint_,
        [this](const asio::error_code& error, size_t bytes_transferred) {
            handle_udp_receive(error, bytes_transferred);
        });
}

void NetworkManager::start_tcp_accept() {
    if (!tcp_acceptor_) return;
    
    auto socket = std::make_shared<asio::ip::tcp::socket>(io_context_);
    tcp_acceptor_->async_accept(*socket,
        [this, socket](const asio::error_code& error) {
            handle_tcp_accept(socket, error);
        });
}

void NetworkManager::handle_udp_receive(const asio::error_code& error, size_t bytes_transferred) {
    if (error) {
        if (error_callback_) {
            error_callback_("UDP receive error: " + error.message());
        }
        return;
    }
    
    try {
        std::vector<uint8_t> data(udp_recv_buffer_.begin(), 
                                 udp_recv_buffer_.begin() + bytes_transferred);
        NetworkMessage msg = NetworkMessage::deserialize(data);
        
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (message_callback_) {
            message_callback_(msg, udp_remote_endpoint_.address().to_string());
        }
    } catch (const std::exception& e) {
        if (error_callback_) {
            error_callback_("UDP message parse error: " + std::string(e.what()));
        }
    }
    
    // Continue receiving
    start_udp_receive();
}

void NetworkManager::handle_tcp_accept(std::shared_ptr<asio::ip::tcp::socket> socket, 
                                      const asio::error_code& error) {
    if (error) {
        if (error_callback_) {
            error_callback_("TCP accept error: " + error.message());
        }
        return;
    }
    
    // Start reading from the new connection
    auto buffer = std::make_shared<std::vector<uint8_t>>(4); // First read length
    asio::async_read(*socket, asio::buffer(*buffer),
        [this, socket, buffer](const asio::error_code& error, size_t bytes_transferred) {
            handle_tcp_read(socket, buffer, error, bytes_transferred);
        });
    
    // Continue accepting new connections
    start_tcp_accept();
}

void NetworkManager::handle_tcp_read(std::shared_ptr<asio::ip::tcp::socket> socket, 
                                    std::shared_ptr<std::vector<uint8_t>> buffer,
                                    const asio::error_code& error, 
                                    size_t bytes_transferred) {
    if (error) {
        if (error_callback_) {
            error_callback_("TCP read error: " + error.message());
        }
        return;
    }
    
    if (buffer->size() == 4) {
        // We've read the length, now read the rest of the message
        uint32_t total_size = (static_cast<uint32_t>((*buffer)[0]) << 24) |
                             (static_cast<uint32_t>((*buffer)[1]) << 16) |
                             (static_cast<uint32_t>((*buffer)[2]) << 8) |
                             static_cast<uint32_t>((*buffer)[3]);
        
        buffer->resize(4 + total_size);
        asio::async_read(*socket, asio::buffer(buffer->data() + 4, total_size),
            [this, socket, buffer](const asio::error_code& error, size_t bytes_transferred) {
                handle_tcp_read(socket, buffer, error, bytes_transferred);
            });
    } else {
        // We've read the complete message
        try {
            NetworkMessage msg = NetworkMessage::deserialize(*buffer);
            
            std::lock_guard<std::mutex> lock(callback_mutex_);
            if (message_callback_) {
                asio::ip::tcp::endpoint remote_endpoint = socket->remote_endpoint();
                message_callback_(msg, remote_endpoint.address().to_string());
            }
        } catch (const std::exception& e) {
            if (error_callback_) {
                error_callback_("TCP message parse error: " + std::string(e.what()));
            }
        }
    }
}

void NetworkManager::run_io_context() {
    io_context_.run();
}

} // namespace decentrilicense