#include "state_chain_storage.h"
#include "decentrilicense/crypto_utils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <iomanip>

namespace fs = std::filesystem;

namespace decentrilicense {

StateChainStorage::StateChainStorage(const std::string& storage_root) 
    : storage_root_(storage_root) {
    // 确保存储根目录存在
    createDirectory(storage_root_);
}

std::string StateChainStorage::getChainDir(const std::string& license_id) const {
    return storage_root_ + "/" + license_id;
}

std::string StateChainStorage::getGenesisTokenPath(const std::string& license_id) const {
    return getChainDir(license_id) + "/genesis_token.json";
}

std::string StateChainStorage::getChainLogPath(const std::string& license_id) const {
    return getChainDir(license_id) + "/chain_log.bin";
}

std::string StateChainStorage::getCurrentStatePath(const std::string& license_id) const {
    return getChainDir(license_id) + "/current_state.json";
}

std::string StateChainStorage::getMetadataPath(const std::string& license_id) const {
    return getChainDir(license_id) + "/chain_meta.json";
}

std::string StateChainStorage::getBackupPath(const std::string& license_id) const {
    return getChainDir(license_id) + "/backup";
}

bool StateChainStorage::createDirectory(const std::string& dirpath) const {
    std::error_code ec;
    return fs::create_directories(dirpath, ec) || fs::exists(dirpath);
}

std::vector<uint8_t> StateChainStorage::serializeToken(const Token& token) const {
    std::string json_str = token.to_json();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

Token StateChainStorage::deserializeToken(const std::vector<uint8_t>& data) const {
    std::string json_str(data.begin(), data.end());
    // 解析JSON字符串为Token对象
    Token token = Token::from_json(json_str);
    return token;
}

uint32_t StateChainStorage::calculateChecksum(const std::vector<uint8_t>& data) const {
    uint32_t checksum = 0;
    for (const auto& byte : data) {
        checksum += byte;
    }
    return checksum;
}

bool StateChainStorage::atomicWriteFile(const std::string& filepath, const std::vector<uint8_t>& data) const {
    // 创建临时文件
    std::string temp_path = filepath + ".tmp";
    
    std::ofstream file(temp_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    file.close();
    
    if (file.fail()) {
        std::remove(temp_path.c_str());
        return false;
    }
    
    // 原子重命名
    std::error_code ec;
    fs::rename(temp_path, filepath, ec);
    return !ec;
}

std::vector<uint8_t> StateChainStorage::readFile(const std::string& filepath) const {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return {};
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    
    return file.good() ? buffer : std::vector<uint8_t>();
}

bool StateChainStorage::saveMetadata(const std::string& license_id, const ChainMetadata& metadata) {
    createDirectory(getChainDir(license_id));
    
    // 手动构造JSON字符串
    std::ostringstream oss;
    oss << "{";
    oss << "\"version\":" << metadata.version << ",";
    oss << "\"total_states\":" << metadata.total_states << ",";
    oss << "\"last_verification_time\":" << metadata.last_verification_time << ",";
    oss << "\"license_id\":\"" << metadata.license_id << "\"";
    oss << "}";
    
    std::string json_str = oss.str();
    std::vector<uint8_t> data(json_str.begin(), json_str.end());
    
    return atomicWriteFile(getMetadataPath(license_id), data);
}

std::optional<ChainMetadata> StateChainStorage::loadMetadata(const std::string& license_id) {
    auto data = readFile(getMetadataPath(license_id));
    if (data.empty()) {
        return std::nullopt;
    }
    
    try {
        std::string json_str(data.begin(), data.end());
        
        // 简单解析JSON字符串
        ChainMetadata metadata;
        
        // 查找version
        size_t pos = json_str.find("\"version\":");
        if (pos != std::string::npos) {
            pos += 10; // 跳过 "\"version\":"
            size_t end = json_str.find_first_of(",}", pos);
            std::string value = json_str.substr(pos, end - pos);
            metadata.version = static_cast<uint32_t>(std::stoul(value));
        }
        
        // 查找total_states
        pos = json_str.find("\"total_states\":");
        if (pos != std::string::npos) {
            pos += 15; // 跳过 "\"total_states\":"
            size_t end = json_str.find_first_of(",}", pos);
            std::string value = json_str.substr(pos, end - pos);
            metadata.total_states = static_cast<uint64_t>(std::stoull(value));
        }
        
        // 查找last_verification_time
        pos = json_str.find("\"last_verification_time\":");
        if (pos != std::string::npos) {
            pos += 23; // 跳过 "\"last_verification_time\":"
            size_t end = json_str.find_first_of(",}", pos);
            std::string value = json_str.substr(pos, end - pos);
            metadata.last_verification_time = static_cast<uint64_t>(std::stoull(value));
        }
        
        // 查找license_id
        pos = json_str.find("\"license_id\":\"");
        if (pos != std::string::npos) {
            pos += 13; // 跳过 "\"license_id\":\""
            size_t end = json_str.find("\"", pos);
            metadata.license_id = json_str.substr(pos, end - pos);
        }
        
        return metadata;
    } catch (...) {
        return std::nullopt;
    }
}

bool StateChainStorage::saveFullChain(const std::string& license_id, 
                                     const std::vector<Token>& chain) {
    if (chain.empty()) {
        return false;
    }
    
    // 创建链目录
    if (!createDirectory(getChainDir(license_id))) {
        return false;
    }
    
    // 保存创世Token
    std::string genesis_json = chain.front().to_json();
    std::vector<uint8_t> genesis_data(genesis_json.begin(), genesis_json.end());
    if (!atomicWriteFile(getGenesisTokenPath(license_id), genesis_data)) {
        return false;
    }
    
    // 创建新的链日志文件
    std::ofstream log_file(getChainLogPath(license_id), std::ios::binary);
    if (!log_file.is_open()) {
        return false;
    }
    
    // 写入所有状态到链日志
    for (const auto& token : chain) {
        auto token_data = serializeToken(token);
        uint32_t length = static_cast<uint32_t>(token_data.size());
        uint32_t checksum = calculateChecksum(token_data);
        
        // 写入记录长度
        log_file.write(reinterpret_cast<const char*>(&length), sizeof(length));
        // 写入Token数据
        log_file.write(reinterpret_cast<const char*>(token_data.data()), token_data.size());
        // 写入校验和
        log_file.write(reinterpret_cast<const char*>(&checksum), sizeof(checksum));
    }
    
    log_file.close();
    if (log_file.fail()) {
        return false;
    }
    
    // 保存当前状态
    std::string current_json = chain.back().to_json();
    std::vector<uint8_t> current_data(current_json.begin(), current_json.end());
    if (!atomicWriteFile(getCurrentStatePath(license_id), current_data)) {
        return false;
    }
    
    // 保存元数据
    ChainMetadata metadata;
    metadata.total_states = chain.size();
    metadata.license_id = license_id;
    metadata.last_verification_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return saveMetadata(license_id, metadata);
}

bool StateChainStorage::appendState(const std::string& license_id, 
                                   const Token& new_state) {
    // 以追加模式打开链日志文件
    std::ofstream log_file(getChainLogPath(license_id), std::ios::binary | std::ios::app);
    if (!log_file.is_open()) {
        return false;
    }
    
    // 序列化Token
    auto token_data = serializeToken(new_state);
    uint32_t length = static_cast<uint32_t>(token_data.size());
    uint32_t checksum = calculateChecksum(token_data);
    
    // 写入记录
    log_file.write(reinterpret_cast<const char*>(&length), sizeof(length));
    log_file.write(reinterpret_cast<const char*>(token_data.data()), token_data.size());
    log_file.write(reinterpret_cast<const char*>(&checksum), sizeof(checksum));
    
    log_file.close();
    if (log_file.fail()) {
        return false;
    }
    
    // 更新当前状态
    std::string current_json = new_state.to_json();
    std::vector<uint8_t> current_data(current_json.begin(), current_json.end());
    if (!atomicWriteFile(getCurrentStatePath(license_id), current_data)) {
        return false;
    }
    
    // 更新元数据
    auto metadata_opt = loadMetadata(license_id);
    if (metadata_opt.has_value()) {
        ChainMetadata metadata = metadata_opt.value();
        metadata.total_states++;
        metadata.last_verification_time = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return saveMetadata(license_id, metadata);
    }
    
    return true;
}

std::vector<Token> StateChainStorage::loadChain(const std::string& license_id) {
    std::vector<Token> chain;
    
    // 打开链日志文件
    std::ifstream log_file(getChainLogPath(license_id), std::ios::binary);
    if (!log_file.is_open()) {
        return chain;
    }
    
    // 逐个读取记录
    while (log_file.peek() != EOF) {
        // 读取记录长度
        uint32_t length;
        log_file.read(reinterpret_cast<char*>(&length), sizeof(length));
        if (log_file.eof() || log_file.fail()) {
            break;
        }
        
        // 读取Token数据
        std::vector<uint8_t> token_data(length);
        log_file.read(reinterpret_cast<char*>(token_data.data()), length);
        if (log_file.fail()) {
            break;
        }
        
        // 读取校验和
        uint32_t stored_checksum;
        log_file.read(reinterpret_cast<char*>(&stored_checksum), sizeof(stored_checksum));
        if (log_file.fail()) {
            break;
        }
        
        // 验证校验和
        uint32_t calculated_checksum = calculateChecksum(token_data);
        if (calculated_checksum != stored_checksum) {
            // 校验和不匹配，数据可能已损坏
            break;
        }
        
        // 反序列化Token
        try {
            Token token = deserializeToken(token_data);
            chain.push_back(token);
        } catch (...) {
            // 如果反序列化失败，跳过这个记录
            continue;
        }
    }
    
    return chain;
}

std::optional<Token> StateChainStorage::getCurrentState(const std::string& license_id) {
    auto data = readFile(getCurrentStatePath(license_id));
    if (data.empty()) {
        return std::nullopt;
    }
    
    try {
        std::string json_str(data.begin(), data.end());
        // 解析JSON字符串为Token对象
        Token token = Token::from_json(json_str);
        return token;
    } catch (...) {
        return std::nullopt;
    }
}

bool StateChainStorage::verifyStoredChain(const std::string& license_id) {
    auto chain = loadChain(license_id);
    if (chain.empty()) {
        return false;
    }
    
    // 实现完整的链验证逻辑
    // 检查每个状态的签名和哈希链接
    for (size_t i = 0; i < chain.size(); ++i) {
        const Token& token = chain[i];
        
        // 验证基本字段
        if (!token.is_valid()) {
            return false;
        }
        
        // 对于非创世状态，验证prev_state_hash
        if (i > 0) {
            const Token& prev_token = chain[i-1];
            std::string prev_json = prev_token.to_json();
            std::string expected_prev_hash = CryptoUtils::sha256(prev_json);

            if (token.prev_state_hash != expected_prev_hash) {
                return false;
            }
        }
        
        // 验证state_index连续性
        if (token.state_index != i) {
            return false;
        }

        // 验证状态签名
        TokenManager token_manager;
        std::vector<Token> single_token_chain = {token};
        if (!token_manager.verify_token_state_chain(token, single_token_chain)) {
            return false;
        }
    }
    
    return true;
}

bool StateChainStorage::recoverChain(const std::string& license_id) {
    // 尝试从当前状态文件恢复
    auto current_state = getCurrentState(license_id);
    if (current_state.has_value()) {
        // 如果当前状态文件存在且有效，我们可以从中恢复
        std::vector<Token> chain = {current_state.value()};
        return saveFullChain(license_id, chain);
    }

    // 如果当前状态文件不存在或无效，尝试从链日志恢复
    auto chain = loadChain(license_id);
    if (!chain.empty()) {
        // 链日志中有数据，保存当前状态
        std::string current_json = chain.back().to_json();
        std::vector<uint8_t> current_data(current_json.begin(), current_json.end());
        return atomicWriteFile(getCurrentStatePath(license_id), current_data);
    }

    return false;
}

// Device key file path helpers
std::string StateChainStorage::getDevicePrivateKeyPath(const std::string& license_id) const {
    return getChainDir(license_id) + "/device_private_key.pem";
}

std::string StateChainStorage::getDevicePublicKeyPath(const std::string& license_id) const {
    return getChainDir(license_id) + "/device_public_key.pem";
}

std::string StateChainStorage::getDeviceIdPath(const std::string& license_id) const {
    return getChainDir(license_id) + "/device_id.txt";
}

// Save device keys to persistent storage
bool StateChainStorage::saveDeviceKeys(const std::string& license_id,
                                       const std::string& device_private_key_pem,
                                       const std::string& device_public_key_pem,
                                       const std::string& device_id) {
    // Ensure the chain directory exists
    std::string chain_dir = getChainDir(license_id);
    if (!createDirectory(chain_dir)) {
        return false;
    }

    // Save private key
    std::vector<uint8_t> private_key_data(device_private_key_pem.begin(), device_private_key_pem.end());
    if (!atomicWriteFile(getDevicePrivateKeyPath(license_id), private_key_data)) {
        return false;
    }

    // Save public key
    std::vector<uint8_t> public_key_data(device_public_key_pem.begin(), device_public_key_pem.end());
    if (!atomicWriteFile(getDevicePublicKeyPath(license_id), public_key_data)) {
        return false;
    }

    // Save device ID
    std::vector<uint8_t> device_id_data(device_id.begin(), device_id.end());
    if (!atomicWriteFile(getDeviceIdPath(license_id), device_id_data)) {
        return false;
    }

    return true;
}

// Load device keys from persistent storage
std::optional<StateChainStorage::DeviceKeys> StateChainStorage::loadDeviceKeys(const std::string& license_id) {
    try {
        // Check if all files exist
        if (!hasDeviceKeys(license_id)) {
            return std::nullopt;
        }

        // Load private key
        auto private_key_data = readFile(getDevicePrivateKeyPath(license_id));
        if (private_key_data.empty()) {
            return std::nullopt;
        }
        std::string device_private_key_pem(private_key_data.begin(), private_key_data.end());

        // Load public key
        auto public_key_data = readFile(getDevicePublicKeyPath(license_id));
        if (public_key_data.empty()) {
            return std::nullopt;
        }
        std::string device_public_key_pem(public_key_data.begin(), public_key_data.end());

        // Load device ID
        auto device_id_data = readFile(getDeviceIdPath(license_id));
        if (device_id_data.empty()) {
            return std::nullopt;
        }
        std::string device_id(device_id_data.begin(), device_id_data.end());

        DeviceKeys keys;
        keys.device_private_key_pem = device_private_key_pem;
        keys.device_public_key_pem = device_public_key_pem;
        keys.device_id = device_id;

        return keys;
    } catch (...) {
        return std::nullopt;
    }
}

// Check if device keys exist for a license
bool StateChainStorage::hasDeviceKeys(const std::string& license_id) {
    return fs::exists(getDevicePrivateKeyPath(license_id)) &&
           fs::exists(getDevicePublicKeyPath(license_id)) &&
           fs::exists(getDeviceIdPath(license_id));
}

} // namespace decentrilicense