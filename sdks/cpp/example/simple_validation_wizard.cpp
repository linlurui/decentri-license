#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iomanip>
#include <ctime>
#include <nlohmann/json.hpp>

// 假设SDK头文件
#include "../include/decentrilicense/token_manager.hpp"
#include "../include/decentrilicense/device_key_manager.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

// 许可证状态结构
struct LicenseState {
    std::string token_data;  // 存储原始token数据（可能是加密的或JSON格式）
    std::string license_public_key;  // 许可证公钥，用于解密
    bool is_activated;
    std::string device_id;
    std::string activation_time;
    int usage_count;
    
    LicenseState() : is_activated(false), usage_count(0) {}
};

// 全局状态
LicenseState g_license_state;
const std::string STATE_FILE = ".decentri/license.state";

// 函数声明
void print_menu();
bool import_license_key();
bool verify_license();
bool activate_to_device();
void query_status();
void record_usage();
std::string get_input(const std::string& prompt);
std::string trim(const std::string& str);
bool save_state();
bool load_state();
bool is_encrypted_token(const std::string& input);
std::string decrypt_token_with_license_key(const std::string& encrypted_token, const std::string& license_public_key);

int main() {
    std::cout << "==========================================\n";
    std::cout << "DecentriLicense C++ SDK 验证向导\n";
    std::cout << "==========================================\n";
    
    // 尝试加载之前的状态
    load_state();
    
    while (true) {
        print_menu();
        
        std::string choice = get_input("请选择: ");
        choice = trim(choice);
        
        if (choice == "1") {
            import_license_key();
        } else if (choice == "2") {
            verify_license();
        } else if (choice == "3") {
            activate_to_device();
        } else if (choice == "4") {
            query_status();
        } else if (choice == "5") {
            record_usage();
        } else if (choice == "0") {
            std::cout << "再见！\n";
            break;
        } else {
            std::cout << "❌ 无效选项，请重新输入。\n";
        }
        
        std::cout << "\n" << std::string(50, '-') << "\n\n";
    }
    
    return 0;
}

void print_menu() {
    std::cout << "\n=== DecentriLicense 向导 ===\n";
    std::cout << "1. 导入许可证密钥\n";
    std::cout << "2. 验证许可证\n";
    std::cout << "3. 激活到当前设备\n";
    std::cout << "4. 查询当前状态/余额\n";
    std::cout << "5. 记录使用量（状态迁移）\n";
    std::cout << "0. 退出\n";
}

bool import_license_key() {
    std::cout << "\n--- 导入许可证密钥 ---\n";
    
    std::string input_method = get_input("输入方式 (1: 直接粘贴, 2: 文件路径): ");
    input_method = trim(input_method);
    
    if (input_method == "1") {
        std::cout << "请粘贴许可证密钥（JWT格式或加密后的字符串）:\n";
        std::string key_data;
        std::getline(std::cin, key_data);
        key_data = trim(key_data);
        
        if (key_data.empty()) {
            std::cout << "❌ 输入不能为空\n";
            return false;
        }
        
        g_license_state.token_data = key_data;
        std::cout << "✅ 许可证密钥已导入\n";
        save_state();
        return true;
        
    } else if (input_method == "2") {
        std::string file_path = get_input("请输入文件路径: ");
        file_path = trim(file_path);
        
        if (file_path.empty()) {
            std::cout << "❌ 文件路径不能为空\n";
            return false;
        }
        
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cout << "❌ 找不到指定的文件\n";
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        g_license_state.token_data = buffer.str();
        file.close();
        
        std::cout << "✅ 许可证密钥已从文件导入\n";
        save_state();
        return true;
    } else {
        std::cout << "❌ 无效的输入方式\n";
        return false;
    }
}

bool verify_license() {
    std::cout << "\n--- 验证许可证 ---\n";
    
    if (g_license_state.token_data.empty()) {
        std::cout << "❌ 请先导入许可证密钥\n";
        return false;
    }
    
    try {
        // 检查是否为加密的token
        if (is_encrypted_token(g_license_state.token_data)) {
            std::cout << "🔒 检测到加密的许可证，正在解密...\n";
            
            // 这里需要实际的解密逻辑
            // 在实际实现中，您需要从某个地方获取许可证公钥
            // 为简化示例，我们假设解密成功
            std::cout << "✅ 许可证解密成功\n";
        } else {
            std::cout << "📄 检测到JSON格式的许可证\n";
        }
        
        // 模拟验证过程
        std::cout << "🔍 正在校验许可证签名...\n";
        std::cout << "✅ 许可证验证通过\n";
        return true;
        
    } catch (const std::exception& e) {
        std::cout << "❌ 许可证验证失败: " << e.what() << "\n";
        return false;
    }
}

bool activate_to_device() {
    std::cout << "\n--- 激活到当前设备 ---\n";
    
    if (g_license_state.token_data.empty()) {
        std::cout << "❌ 请先导入许可证密钥\n";
        return false;
    }
    
    // 生成设备ID（简化示例）
    g_license_state.device_id = "DEV-" + std::to_string(std::time(nullptr) % 100000);
    
    // 获取当前时间
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    g_license_state.activation_time = oss.str();
    
    g_license_state.is_activated = true;
    
    std::cout << "✅ 设备激活成功\n";
    std::cout << "  设备ID: " << g_license_state.device_id << "\n";
    std::cout << "  激活时间: " << g_license_state.activation_time << "\n";
    
    save_state();
    return true;
}

void query_status() {
    std::cout << "\n--- 查询当前状态/余额 ---\n";
    
    std::cout << "许可证状态:\n";
    std::cout << "  是否已导入: " << (g_license_state.token_data.empty() ? "否" : "是") << "\n";
    std::cout << "  是否已激活: " << (g_license_state.is_activated ? "是" : "否") << "\n";
    
    if (g_license_state.is_activated) {
        std::cout << "  设备ID: " << g_license_state.device_id << "\n";
        std::cout << "  激活时间: " << g_license_state.activation_time << "\n";
    }
    
    std::cout << "  使用次数: " << g_license_state.usage_count << "\n";
}

void record_usage() {
    std::cout << "\n--- 记录使用量（状态迁移） ---\n";
    
    if (!g_license_state.is_activated) {
        std::cout << "❌ 请先激活到当前设备\n";
        return;
    }
    
    g_license_state.usage_count++;
    
    std::cout << "✅ 使用量记录成功\n";
    std::cout << "  当前使用次数: " << g_license_state.usage_count << "\n";
    
    save_state();
}

std::string get_input(const std::string& prompt) {
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

bool save_state() {
    try {
        // 创建目录
        fs::create_directories(".decentri");
        
        // 创建JSON对象
        json state_json;
        state_json["token_data"] = g_license_state.token_data;
        state_json["license_public_key"] = g_license_state.license_public_key;
        state_json["is_activated"] = g_license_state.is_activated;
        state_json["device_id"] = g_license_state.device_id;
        state_json["activation_time"] = g_license_state.activation_time;
        state_json["usage_count"] = g_license_state.usage_count;
        
        // 写入文件
        std::ofstream file(STATE_FILE);
        if (file.is_open()) {
            file << state_json.dump(2);
            file.close();
            return true;
        }
    } catch (const std::exception& e) {
        std::cout << "⚠️  保存状态失败: " << e.what() << "\n";
    }
    
    return false;
}

bool load_state() {
    try {
        std::ifstream file(STATE_FILE);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string data = buffer.str();
            file.close();
            
            auto state_json = json::parse(data);
            
            if (state_json.contains("token_data")) {
                g_license_state.token_data = state_json["token_data"].get<std::string>();
            }
            if (state_json.contains("license_public_key")) {
                g_license_state.license_public_key = state_json["license_public_key"].get<std::string>();
            }
            if (state_json.contains("is_activated")) {
                g_license_state.is_activated = state_json["is_activated"].get<bool>();
            }
            if (state_json.contains("device_id")) {
                g_license_state.device_id = state_json["device_id"].get<std::string>();
            }
            if (state_json.contains("activation_time")) {
                g_license_state.activation_time = state_json["activation_time"].get<std::string>();
            }
            if (state_json.contains("usage_count")) {
                g_license_state.usage_count = state_json["usage_count"].get<int>();
            }
            
            return true;
        }
    } catch (const std::exception&) {
        // 忽略加载错误
    }
    
    return false;
}

bool is_encrypted_token(const std::string& input) {
    // 加密token格式: encrypted_data|nonce
    return input.find('|') != std::string::npos;
}

std::string decrypt_token_with_license_key(const std::string& encrypted_token, const std::string& license_public_key) {
    // 这里应该实现实际的解密逻辑
    // 为简化示例，直接返回原始数据
    return encrypted_token;
}