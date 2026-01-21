#include "decentrilicense/token_manager.hpp"
#include "decentrilicense/device_key_manager.hpp"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace decentrilicense;

int main() {
    try {
        // Read token from file
        std::ifstream file("enhanced_token.json");
        if (!file.is_open()) {
            std::cerr << "Failed to open token file" << std::endl;
            return 1;
        }
        
        std::string token_json((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
        file.close();
        
        // Parse token
        auto token = Token::from_json(token_json);
        
        // Create device key manager
        DeviceKeyManager device_manager;
        
        // Verify device identity
        if (!token.device_info.fingerprint.empty()) {
            DeviceKeyManager::DeviceInfo device_info;
            device_info.fingerprint = token.device_info.fingerprint;
            device_info.public_key_pem = token.device_info.public_key;
            device_info.signature = token.device_info.signature;
            
            if (device_manager.verify_device_identity(device_info)) {
                std::cout << "✅ Device identity verification passed" << std::endl;
            } else {
                std::cout << "❌ Device identity verification failed" << std::endl;
                return 1;
            }
        }
        
        // Verify usage chain
        if (!token.usage_chain.empty()) {
            if (device_manager.verify_usage_chain(token_json)) {
                std::cout << "✅ Usage chain verification passed" << std::endl;
            } else {
                std::cout << "❌ Usage chain verification failed" << std::endl;
                return 1;
            }
        }
        
        // Add a new usage record
        std::string updated_token = device_manager.add_usage_record(
            token_json, 
            "api_call", 
            "{\"function\": \"process_image\"}"
        );
        
        // Save updated token
        std::ofstream outfile("updated_token.json");
        if (outfile.is_open()) {
            outfile << updated_token;
            outfile.close();
            std::cout << "✅ Updated token saved to updated_token.json" << std::endl;
        }
        
        std::cout << "✅ Enhanced validation completed successfully" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}