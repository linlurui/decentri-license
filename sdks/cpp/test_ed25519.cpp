#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include "decenlicense_c.h"

std::string read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::stringstream buf;
    buf << f.rdbuf();
    return buf.str();
}

DL_Client* new_client() {
    DL_Client* client = dl_client_create();
    if (!client) { std::cerr << "Failed to create client\n"; exit(1); }
    DL_ClientConfig config;
    memset(&config, 0, sizeof(config));
    config.udp_port = 13325;
    config.tcp_port = 23325;
    uint32_t rc = dl_client_initialize(client, &config);
    if (rc != DL_ERROR_SUCCESS) { std::cerr << "Init failed: " << rc << "\n"; exit(1); }
    return client;
}

void test_windsurf_free() {
    const std::string dir = "/Volumes/workspace/project/dl-issuer/dl-issuer";
    std::string token_json = read_file(dir + "/token_windsurf-free_AUTO-GENERATED-LICENSE-2026-018-0B3GNW_20260427090408.json");
    std::string product_key = read_file(dir + "/public_windsurf-free_20260427090402.pem");
    if (token_json.empty() || product_key.empty()) { std::cerr << "windsurf-free files not found\n"; exit(1); }
    std::cout << "windsurf-free token loaded\n";

    DL_Client* c1 = new_client();
    dl_client_set_product_public_key(c1, product_key.c_str());
    dl_client_import_token(c1, token_json.c_str());
    DL_VerificationResult vr1;
    dl_client_offline_verify_current_token(c1, &vr1);
    if (!vr1.valid) { std::cerr << "windsurf-free verify failed: " << vr1.error_message << "\n"; exit(1); }
    std::cout << "✅ C++ SDK: windsurf-free offline_verify passed\n";
    dl_client_shutdown(c1); dl_client_destroy(c1);

    DL_Client* c2 = new_client();
    dl_client_set_product_public_key(c2, product_key.c_str());
    dl_client_import_token(c2, token_json.c_str());
    DL_VerificationResult vr2;
    dl_client_activate_bind_device(c2, &vr2);
    if (!vr2.valid) { std::cerr << "windsurf-free activate failed: " << vr2.error_message << "\n"; exit(1); }
    std::cout << "✅ C++ SDK: windsurf-free activate_bind_device passed\n";
    dl_client_shutdown(c2); dl_client_destroy(c2);
}

void test_cursor_free() {
    const std::string dir = "/Volumes/workspace/project/dl-issuer/dl-issuer";
    std::string token_json = read_file(dir + "/token_cursor-free_AUTO-GENERATED-LICENSE-2026-019-I0SKNQ_20260427090411.json");
    std::string product_key = read_file(dir + "/public_cursor-free_20260427090405.pem");
    if (token_json.empty() || product_key.empty()) { std::cerr << "cursor-free files not found\n"; exit(1); }
    std::cout << "cursor-free token loaded\n";

    DL_Client* c1 = new_client();
    dl_client_set_product_public_key(c1, product_key.c_str());
    dl_client_import_token(c1, token_json.c_str());
    DL_VerificationResult vr1;
    dl_client_offline_verify_current_token(c1, &vr1);
    if (!vr1.valid) { std::cerr << "cursor-free verify failed: " << vr1.error_message << "\n"; exit(1); }
    std::cout << "✅ C++ SDK: cursor-free offline_verify passed\n";
    dl_client_shutdown(c1); dl_client_destroy(c1);

    DL_Client* c2 = new_client();
    dl_client_set_product_public_key(c2, product_key.c_str());
    dl_client_import_token(c2, token_json.c_str());
    DL_VerificationResult vr2;
    dl_client_activate_bind_device(c2, &vr2);
    if (!vr2.valid) { std::cerr << "cursor-free activate failed: " << vr2.error_message << "\n"; exit(1); }
    std::cout << "✅ C++ SDK: cursor-free activate_bind_device passed\n";
    dl_client_shutdown(c2); dl_client_destroy(c2);
}

int main() {
    test_windsurf_free();
    test_cursor_free();
    std::cout << "\n🎉 All C++ SDK tests passed!\n";
    return 0;
}
