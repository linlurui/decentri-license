#ifndef STATE_CHAIN_STORAGE_H
#define STATE_CHAIN_STORAGE_H

#include "decentrilicense/token_manager.hpp"
#include <string>
#include <vector>
#include <optional>
#include <fstream>
#include <filesystem>
#include <cstring>

namespace decentrilicense {

struct ChainMetadata {
    uint32_t version = 1;
    uint64_t total_states = 0;
    uint64_t last_verification_time = 0;
    std::string license_id;
};

class StateChainStorage {
public:
    // 初始化，指定存储根目录（如 ~/.appname/chains/）
    explicit StateChainStorage(const std::string& storage_root);
    
    // 保存完整状态链（首次或全量备份）
    bool saveFullChain(const std::string& license_id, 
                       const std::vector<Token>& chain);
    
    // 追加一个状态到链尾（高效，仅追加日志）
    bool appendState(const std::string& license_id, 
                     const Token& new_state);
    
    // 从持久化存储加载完整状态链
    std::vector<Token> loadChain(const std::string& license_id);
    
    // 获取当前最新状态（快速读取，不加载完整链）
    std::optional<Token> getCurrentState(const std::string& license_id);
    
    // 验证存储的链完整性（从头验证所有签名和哈希）
    bool verifyStoredChain(const std::string& license_id);
    
    // 恢复损坏的链数据
    bool recoverChain(const std::string& license_id);

    // Device key persistence methods
    // Save device keys (private key, public key, device ID) for a license
    bool saveDeviceKeys(const std::string& license_id,
                       const std::string& device_private_key_pem,
                       const std::string& device_public_key_pem,
                       const std::string& device_id);

    // Load device keys from storage
    struct DeviceKeys {
        std::string device_private_key_pem;
        std::string device_public_key_pem;
        std::string device_id;
    };
    std::optional<DeviceKeys> loadDeviceKeys(const std::string& license_id);

    // Check if device keys exist for a license
    bool hasDeviceKeys(const std::string& license_id);

private:
    std::string getChainDir(const std::string& license_id) const;
    std::string getGenesisTokenPath(const std::string& license_id) const;
    std::string getChainLogPath(const std::string& license_id) const;
    std::string getCurrentStatePath(const std::string& license_id) const;
    std::string getMetadataPath(const std::string& license_id) const;
    std::string getBackupPath(const std::string& license_id) const;

    // Device key file paths
    std::string getDevicePrivateKeyPath(const std::string& license_id) const;
    std::string getDevicePublicKeyPath(const std::string& license_id) const;
    std::string getDeviceIdPath(const std::string& license_id) const;

    // 二进制序列化和反序列化
    std::vector<uint8_t> serializeToken(const Token& token) const;
    Token deserializeToken(const std::vector<uint8_t>& data) const;
    
    // 计算校验和
    uint32_t calculateChecksum(const std::vector<uint8_t>& data) const;
    
    // 原子写入文件
    bool atomicWriteFile(const std::string& filepath, const std::vector<uint8_t>& data) const;
    
    // 读取文件
    std::vector<uint8_t> readFile(const std::string& filepath) const;
    
    // 创建目录
    bool createDirectory(const std::string& dirpath) const;
    
    // 保存元数据
    bool saveMetadata(const std::string& license_id, const ChainMetadata& metadata);
    
    // 加载元数据
    std::optional<ChainMetadata> loadMetadata(const std::string& license_id);
    
    std::string storage_root_;
};

} // namespace decentrilicense

#endif // STATE_CHAIN_STORAGE_H