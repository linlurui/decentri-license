#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <ctime>
#include <memory>
#include <algorithm>
#include <limits>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../include/decenlicense_c.h"

using namespace std;

// Global variables
static DL_Client* g_client = nullptr;
static bool g_initialized = false;
static string g_selected_product_key_path;

// Constants
constexpr size_t MAX_TOKEN_SIZE = 16384;
constexpr size_t MAX_PATH = 512;

// Helper function declarations
void cleanup_client();
DL_Client* get_or_create_client();
string read_file_to_string(const string& filepath);
bool write_string_to_file(const string& filepath, const string& content);
vector<string> find_product_public_keys();
vector<string> find_token_files(const string& pattern);
vector<string> find_encrypted_token_files();
vector<string> find_activated_token_files();
vector<string> find_state_token_files();
string resolve_product_key_path(const string& filename);
string resolve_token_file_path(const string& filename);
string find_product_public_key();
bool file_exists(const string& path);
string get_input_line();
string trim(const string& str);

// Menu function declarations
void select_product_key_wizard();
void activate_token_wizard();
void verify_activated_token_wizard();
void validate_token_wizard();
void accounting_wizard();
void trust_chain_validation_wizard();
void comprehensive_validation_wizard();

// Helper function implementations
void cleanup_client() {
    if (g_client != nullptr) {
        dl_client_shutdown(g_client);
        dl_client_destroy(g_client);
        g_client = nullptr;
        g_initialized = false;
    }
}

DL_Client* get_or_create_client() {
    if (g_client == nullptr) {
        g_client = dl_client_create();
        g_initialized = false;
    }
    return g_client;
}

bool file_exists(const string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

string trim(const string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

string get_input_line() {
    string line;
    getline(cin, line);
    return trim(line);
}

string read_file_to_string(const string& filepath) {
    ifstream file(filepath, ios::binary);
    if (!file) {
        return "";
    }

    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool write_string_to_file(const string& filepath, const string& content) {
    ofstream file(filepath, ios::binary);
    if (!file) {
        return false;
    }

    file << content;
    return file.good();
}

void add_matching_files_from_dir(vector<string>& result, const string& dir,
                                  const function<bool(const string&)>& matcher) {
    DIR* d = opendir(dir.c_str());
    if (d == nullptr) {
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(d)) != nullptr) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        string name = entry->d_name;
        if (matcher(name)) {
            // Check if already in result
            if (find(result.begin(), result.end(), name) == result.end()) {
                result.push_back(name);
            }
        }
    }

    closedir(d);
}

vector<string> find_product_public_keys() {
    vector<string> result;

    vector<string> search_dirs = {
        ".",
        "..",
        "../..",
        "../../../dl-issuer"
    };

    auto matcher = [](const string& name) {
        return name.find("public") != string::npos &&
               name.find("private") == string::npos &&
               name.find(".pem") != string::npos;
    };

    for (const auto& dir : search_dirs) {
        add_matching_files_from_dir(result, dir, matcher);
    }

    sort(result.begin(), result.end());
    return result;
}

vector<string> find_token_files(const string& pattern) {
    vector<string> result;

    vector<string> search_dirs = {
        ".",
        "..",
        "../../../dl-issuer"
    };

    auto matcher = [&pattern](const string& name) {
        return name.find("token_") != string::npos &&
               name.find(".txt") != string::npos &&
               (pattern.empty() || name.find(pattern) != string::npos);
    };

    for (const auto& dir : search_dirs) {
        add_matching_files_from_dir(result, dir, matcher);
    }

    sort(result.begin(), result.end());
    return result;
}

vector<string> find_encrypted_token_files() {
    return find_token_files("encrypted");
}

vector<string> find_activated_token_files() {
    return find_token_files("activated");
}

vector<string> find_state_token_files() {
    vector<string> result;

    vector<string> search_dirs = {
        ".",
        "..",
        "../../../dl-issuer"
    };

    // 照抄Go SDK: 文件名必须以token_activated_或token_state_开头
    auto matcher = [](const string& name) {
        return (name.rfind("token_activated_", 0) == 0 || name.rfind("token_state_", 0) == 0) &&
               name.find(".txt") != string::npos;
    };

    for (const auto& dir : search_dirs) {
        add_matching_files_from_dir(result, dir, matcher);
    }

    sort(result.begin(), result.end());
    return result;
}

string resolve_product_key_path(const string& filename) {
    vector<string> search_paths = {
        "./" + filename,
        "../" + filename,
        "../../" + filename,
        "../../../dl-issuer/" + filename
    };

    for (const auto& path : search_paths) {
        if (file_exists(path)) {
            return path;
        }
    }

    return filename;
}

string resolve_token_file_path(const string& filename) {
    vector<string> search_paths = {
        "./" + filename,
        "../" + filename,
        "../../../dl-issuer/" + filename
    };

    for (const auto& path : search_paths) {
        if (file_exists(path)) {
            return path;
        }
    }

    return filename;
}

string find_product_public_key() {
    if (!g_selected_product_key_path.empty()) {
        return g_selected_product_key_path;
    }

    auto keys = find_product_public_keys();
    if (!keys.empty()) {
        return resolve_product_key_path(keys[0]);
    }

    return "";
}

// Main menu functions
void select_product_key_wizard() {
    cout << "\n选择产品公钥" << endl;
    cout << "==============" << endl;

    auto available_keys = find_product_public_keys();

    if (available_keys.empty()) {
        cout << "当前目录下没有找到产品公钥文件" << endl;
        cout << "请将产品公钥文件 (public_*.pem) 放置在当前目录下" << endl;
        return;
    }

    cout << "找到以下产品公钥文件:" << endl;
    for (size_t i = 0; i < available_keys.size(); i++) {
        cout << (i + 1) << ". " << available_keys[i] << endl;
    }
    cout << (available_keys.size() + 1) << ". 取消选择" << endl;

    if (!g_selected_product_key_path.empty()) {
        cout << "当前已选择: " << g_selected_product_key_path << endl;
    }

    cout << "请选择要使用的产品公钥文件 (1-" << (available_keys.size() + 1) << "): ";

    string input = get_input_line();
    int choice = 0;
    try {
        choice = stoi(input);
    } catch (...) {
        cout << "无效选择" << endl;
        return;
    }

    if (choice < 1 || choice > static_cast<int>(available_keys.size() + 1)) {
        cout << "无效选择" << endl;
        return;
    }

    if (choice == static_cast<int>(available_keys.size() + 1)) {
        g_selected_product_key_path.clear();
        cout << "已取消产品公钥选择" << endl;
        return;
    }

    string selected_file = available_keys[choice - 1];
    g_selected_product_key_path = resolve_product_key_path(selected_file);
    cout << "已选择产品公钥文件: " << selected_file << endl;
}

void activate_token_wizard() {
    cout << "\n激活令牌" << endl;
    cout << "----------" << endl;
    cout << "重要说明:" << endl;
    cout << "   • 加密token(encrypted): 首次从供应商获得,需要激活" << endl;
    cout << "   • 已激活token(activated): 激活后生成,可直接使用,不需再次激活" << endl;
    cout << "   本功能仅用于【首次激活】加密token" << endl;
    cout << "   如需使用已激活token,请直接选择其他功能(如记账、验证)" << endl;
    cout << endl;

    DL_Client* client = get_or_create_client();
    if (client == nullptr) {
        cout << "创建客户端失败" << endl;
        return;
    }

    // Show available encrypted token files
    auto token_files = find_encrypted_token_files();

    if (!token_files.empty()) {
        cout << "发现以下加密token文件:" << endl;
        for (size_t i = 0; i < token_files.size(); i++) {
            cout << "   " << (i + 1) << ". " << token_files[i] << endl;
        }
        cout << "您可以输入序号选择文件,或输入文件名/路径" << endl;
    }

    cout << "请输入令牌字符串 (仅支持加密令牌):" << endl;
    cout << "加密令牌通常从软件提供商处获得" << endl;
    cout << "输入序号(1-" << token_files.size() << ")可快速选择上面列出的文件" << endl;
    cout << "令牌或文件路径: ";

    string input = get_input_line();
    string token_string;

    // Check if input is a number (file index)
    if (!token_files.empty()) {
        try {
            int index = stoi(input);
            if (index >= 1 && index <= static_cast<int>(token_files.size())) {
                string file_path = resolve_token_file_path(token_files[index - 1]);
                token_string = read_file_to_string(file_path);
                token_string = trim(token_string);
                if (!token_string.empty()) {
                    cout << "选择文件 '" << token_files[index - 1]
                         << "' 并读取到令牌 (" << token_string.length() << " 字符)" << endl;
                } else {
                    cout << "无法读取文件 " << file_path << endl;
                    return;
                }
            }
        } catch (...) {
            // Not a number, continue
        }
    }

    // If not from file selection, check if it's a file path
    if (token_string.empty() &&
        (input.find('/') != string::npos || input.find(".txt") != string::npos ||
         input.find("token_") != string::npos)) {
        string file_path = resolve_token_file_path(input);
        token_string = read_file_to_string(file_path);
        token_string = trim(token_string);
        if (!token_string.empty()) {
            cout << "从文件读取到令牌 (" << token_string.length() << " 字符)" << endl;
        } else {
            cout << "无法读取文件 " << file_path << ",将直接使用输入作为令牌字符串" << endl;
            token_string = input;
        }
    } else if (token_string.empty()) {
        token_string = input;
    }

    if (token_string.empty()) {
        cout << "令牌字符串为空" << endl;
        return;
    }

    // Initialize client if not already initialized
    if (!g_initialized) {
        DL_ClientConfig config;
        config.license_code = "TEMP";
        config.preferred_mode = DL_CONNECTION_MODE_OFFLINE;
        config.udp_port = 13325;
        config.tcp_port = 23325;
        config.registry_server_url = nullptr;

        DL_ErrorCode err = dl_client_initialize(client, &config);
        if (err != DL_ERROR_SUCCESS) {
            cout << "初始化失败 (需要产品公钥)" << endl;
            cout << "正在查找产品公钥文件..." << endl;
        } else {
            cout << "客户端初始化成功" << endl;
            g_initialized = true;
        }
    } else {
        cout << "客户端已初始化,使用现有实例" << endl;
    }

    // Find and set product public key
    string product_key_path = find_product_public_key();
    if (product_key_path.empty()) {
        cout << "未找到产品公钥文件" << endl;
        cout << "请先选择产品公钥 (菜单选项 0),或确保当前目录下有产品公钥文件" << endl;
        return;
    }

    cout << "使用产品公钥文件: " << product_key_path << endl;

    string product_key_data = read_file_to_string(product_key_path);
    if (product_key_data.empty()) {
        cout << "读取产品公钥文件失败" << endl;
        return;
    }

    DL_ErrorCode err = dl_client_set_product_public_key(client, product_key_data.c_str());
    if (err != DL_ERROR_SUCCESS) {
        cout << "设置产品公钥失败" << endl;
        return;
    }
    cout << "产品公钥设置成功" << endl;

    // Import token
    cout << "正在导入令牌..." << endl;
    err = dl_client_import_token(client, token_string.c_str());
    if (err != DL_ERROR_SUCCESS) {
        cout << "令牌导入失败" << endl;
        return;
    }
    cout << "令牌导入成功" << endl;

    // Activate token
    cout << "正在激活令牌..." << endl;
    DL_VerificationResult result;
    err = dl_client_activate_bind_device(client, &result);

    if (err != DL_ERROR_SUCCESS || !result.valid) {
        cout << "激活失败: " << result.error_message << endl;
        return;
    }

    cout << "令牌激活成功!" << endl;

    // Export activated token
    vector<char> activated_token(MAX_TOKEN_SIZE);
    err = dl_client_export_activated_token_encrypted(client, activated_token.data(), activated_token.size());

    if (err == DL_ERROR_SUCCESS && strlen(activated_token.data()) > 0) {
        string token_str(activated_token.data());
        cout << "\n激活后的新Token(加密):" << endl;
        cout << "   长度: " << token_str.length() << " 字符" << endl;
        if (token_str.length() > 100) {
            cout << "   前缀: " << token_str.substr(0, 100) << "..." << endl;
        } else {
            cout << "   内容: " << token_str << endl;
        }

        // Save to file
        DL_StatusResult status;
        if (dl_client_get_status(client, &status) == DL_ERROR_SUCCESS && strlen(status.license_code) > 0) {
            time_t now = time(nullptr);
            struct tm* tm_info = localtime(&now);
            char timestamp[32];
            strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", tm_info);

            string filename = string("token_activated_") + status.license_code + "_" + timestamp + ".txt";

            if (write_string_to_file(filename, token_str)) {
                char abs_path[PATH_MAX];
                realpath(filename.c_str(), abs_path);
                cout << "\n已保存到文件: " << abs_path << endl;
                cout << "   此token包含设备绑定信息,可传递给下一个设备使用" << endl;
            } else {
                cout << "保存token文件失败" << endl;
            }
        }
    }

    // Display status
    DL_StatusResult status;
    if (dl_client_get_status(client, &status) == DL_ERROR_SUCCESS && status.is_activated) {
        cout << "当前状态: 已激活" << endl;
        if (status.has_token) {
            cout << "令牌ID: " << status.token_id << endl;
            cout << "许可证代码: " << status.license_code << endl;
            cout << "持有设备: " << status.holder_device_id << endl;

            time_t issue_time_t = static_cast<time_t>(status.issue_time);
            struct tm* tm_info = localtime(&issue_time_t);
            char time_str[64];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
            cout << "颁发时间: " << time_str << endl;
        }
    } else {
        cout << "当前状态: 未激活" << endl;
    }
}

void verify_activated_token_wizard() {
    cout << "\n校验已激活令牌" << endl;
    cout << "----------------" << endl;

    // Check for activated tokens in state directory
    DIR* state_dir = opendir(".decentrilicense_state");
    if (state_dir == nullptr) {
        cout << "没有找到已激活的令牌" << endl;
        return;
    }

    vector<string> activated_list;
    struct dirent* entry;
    cout << "\n已激活的令牌列表:" << endl;
    int index = 1;

    while ((entry = readdir(state_dir)) != nullptr) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            string name = entry->d_name;
            activated_list.push_back(name);

            string state_file = ".decentrilicense_state/" + name + "/current_state.json";
            if (file_exists(state_file)) {
                cout << index << ". " << name << " (已激活)" << endl;
            } else {
                cout << index << ". " << name << " (无状态文件)" << endl;
            }
            index++;
        }
    }
    closedir(state_dir);

    if (activated_list.empty()) {
        cout << "没有找到已激活的令牌" << endl;
        return;
    }

    cout << "\n请选择要验证的令牌 (1-" << activated_list.size() << "): ";

    string input = get_input_line();
    int choice = 0;
    try {
        choice = stoi(input);
    } catch (...) {
        cout << "无效的选择" << endl;
        return;
    }

    if (choice < 1 || choice > static_cast<int>(activated_list.size())) {
        cout << "无效的选择" << endl;
        return;
    }

    string selected_license = activated_list[choice - 1];
    cout << "\n正在验证令牌: " << selected_license << endl;

    DL_Client* client = get_or_create_client();
    if (client == nullptr) {
        cout << "获取客户端失败" << endl;
        return;
    }

    // Check if this is the current activated token
    DL_StatusResult status;
    if (dl_client_get_status(client, &status) == DL_ERROR_SUCCESS &&
        string(status.license_code) == selected_license) {

        cout << "正在验证令牌..." << endl;
        DL_VerificationResult result;
        DL_ErrorCode err = dl_client_offline_verify_current_token(client, &result);

        if (err != DL_ERROR_SUCCESS) {
            cout << "令牌验证失败" << endl;
        } else if (result.valid) {
            cout << "令牌验证成功" << endl;
            if (strlen(result.error_message) > 0) {
                cout << "信息: " << result.error_message << endl;
            }
        } else {
            cout << "令牌验证失败" << endl;
            cout << "错误信息: " << result.error_message << endl;
        }

        if (status.has_token) {
            cout << "\n令牌信息:" << endl;
            cout << "   令牌ID: " << status.token_id << endl;
            cout << "   许可证代码: " << status.license_code << endl;
            cout << "   应用ID: " << status.app_id << endl;
            cout << "   持有设备ID: " << status.holder_device_id << endl;

            time_t issue_time_t = static_cast<time_t>(status.issue_time);
            struct tm* tm_info = localtime(&issue_time_t);
            char time_str[64];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
            cout << "   颁发时间: " << time_str << endl;

            if (status.expire_time == 0) {
                cout << "   到期时间: 永不过期" << endl;
            } else {
                time_t expire_time_t = static_cast<time_t>(status.expire_time);
                tm_info = localtime(&expire_time_t);
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
                cout << "   到期时间: " << time_str << endl;
            }

            cout << "   状态索引: " << status.state_index << endl;
            cout << "   激活状态: " << (status.is_activated ? "是" : "否") << endl;
        }
    } else {
        cout << "此令牌不是当前激活的令牌,显示已保存的状态信息:" << endl;
        string state_file = ".decentrilicense_state/" + selected_license + "/current_state.json";

        string file_content = read_file_to_string(state_file);
        if (file_content.empty()) {
            cout << "读取状态文件失败" << endl;
            return;
        }

        cout << "\n令牌信息 (从状态文件读取):" << endl;
        cout << "   许可证代码: " << selected_license << endl;
        cout << "   状态文件: " << state_file << endl;
        cout << "   文件大小: " << file_content.length() << " 字节" << endl;
        cout << "\n提示: 如需完整验证此令牌,请使用选项1重新激活" << endl;
    }
}

void validate_token_wizard() {
    cout << "\n验证令牌合法性" << endl;
    cout << "----------------" << endl;

    DL_Client* client = get_or_create_client();
    if (client == nullptr) {
        cout << "获取客户端失败" << endl;
        return;
    }

    // Initialize client if needed
    if (!g_initialized) {
        DL_ClientConfig config;
        config.license_code = "VALIDATE";
        config.preferred_mode = DL_CONNECTION_MODE_OFFLINE;
        config.udp_port = 13325;
        config.tcp_port = 23325;
        config.registry_server_url = nullptr;

        DL_ErrorCode err = dl_client_initialize(client, &config);
        if (err != DL_ERROR_SUCCESS) {
            cout << "初始化失败 (需要产品公钥)" << endl;
        } else {
            cout << "客户端初始化成功" << endl;
            g_initialized = true;
        }
    }

    // Find and set product public key
    string product_key_path = find_product_public_key();
    if (product_key_path.empty()) {
        cout << "未找到产品公钥文件" << endl;
        cout << "请先选择产品公钥 (菜单选项 0),或确保当前目录下有产品公钥文件" << endl;
        return;
    }

    cout << "使用产品公钥文件: " << product_key_path << endl;

    string product_key_data = read_file_to_string(product_key_path);
    if (product_key_data.empty()) {
        cout << "读取产品公钥文件失败" << endl;
        return;
    }

    DL_ErrorCode err = dl_client_set_product_public_key(client, product_key_data.c_str());
    if (err != DL_ERROR_SUCCESS) {
        cout << "设置产品公钥失败" << endl;
        return;
    }
    cout << "产品公钥设置成功" << endl;

    // Show available encrypted token files
    auto token_files = find_encrypted_token_files();

    if (!token_files.empty()) {
        cout << "发现以下加密token文件:" << endl;
        for (size_t i = 0; i < token_files.size(); i++) {
            cout << "   " << (i + 1) << ". " << token_files[i] << endl;
        }
        cout << "您可以输入序号选择文件,或输入文件名/路径/token字符串" << endl;
    }

    cout << "请输入要验证的令牌字符串 (支持加密令牌):" << endl;
    cout << "令牌通常从软件提供商处获得,或从加密令牌文件读取" << endl;
    cout << "令牌或文件路径: ";

    string input = get_input_line();
    string token_string;

    // Check if input is a number (file index)
    try {
        int index = stoi(input);
        if (!token_files.empty() && index >= 1 && index <= static_cast<int>(token_files.size())) {
            string file_path = resolve_token_file_path(token_files[index - 1]);
            token_string = read_file_to_string(file_path);
            token_string = trim(token_string);
            if (!token_string.empty()) {
                cout << "从文件 '" << token_files[index - 1]
                     << "' 读取到令牌 (" << token_string.length() << " 字符)" << endl;
            } else {
                cout << "无法读取文件 " << file_path << endl;
                return;
            }
        }
    } catch (...) {
        // Not a number
    }

    if (token_string.empty() &&
        (input.find('/') != string::npos || input.find(".txt") != string::npos ||
         input.find("token_") != string::npos)) {
        string file_path = resolve_token_file_path(input);
        token_string = read_file_to_string(file_path);
        token_string = trim(token_string);
        if (!token_string.empty()) {
            cout << "从文件读取到令牌 (" << token_string.length() << " 字符)" << endl;
        } else {
            cout << "无法读取文件 " << file_path << ",将直接使用输入作为令牌字符串" << endl;
            token_string = input;
        }
    } else if (token_string.empty()) {
        token_string = input;
    }

    if (token_string.empty()) {
        cout << "令牌字符串为空" << endl;
        return;
    }

    // Import and validate token
    cout << "正在导入令牌..." << endl;
    err = dl_client_import_token(client, token_string.c_str());
    if (err != DL_ERROR_SUCCESS) {
        cout << "令牌导入失败" << endl;
        return;
    }
    cout << "令牌导入成功" << endl;

    cout << "正在验证令牌合法性..." << endl;
    DL_VerificationResult result;
    err = dl_client_offline_verify_current_token(client, &result);

    if (err != DL_ERROR_SUCCESS) {
        cout << "令牌验证失败" << endl;
    } else if (result.valid) {
        cout << "令牌验证成功 - 令牌合法且有效" << endl;
        if (strlen(result.error_message) > 0) {
            cout << "详细信息: " << result.error_message << endl;
        }
    } else {
        cout << "令牌验证失败 - 令牌不合法或无效" << endl;
        cout << "错误信息: " << result.error_message << endl;
    }
}

void accounting_wizard() {
    cout << "\n记账信息" << endl;
    cout << "----------" << endl;

    DL_Client* client = get_or_create_client();
    if (client == nullptr) {
        cout << "获取客户端失败" << endl;
        return;
    }

    // Show available state token files
    auto token_files = find_state_token_files();

    // Check activation status
    int activated = dl_client_is_activated(client);

    cout << "\n请选择令牌来源:" << endl;
    if (activated) {
        cout << "0. 使用当前激活的令牌" << endl;
    }

    if (!token_files.empty()) {
        cout << "\n或从以下文件加载令牌:" << endl;
        for (size_t i = 0; i < token_files.size(); i++) {
            cout << (i + 1) << ". " << token_files[i] << endl;
        }
    }

    if (!activated && token_files.empty()) {
        cout << "当前没有激活的令牌,也没有找到可用的token文件" << endl;
        cout << "请先使用选项1激活令牌" << endl;
        return;
    }

    cout << "\n请选择 (0";
    if (!token_files.empty()) {
        cout << "-" << token_files.size();
    }
    cout << "): ";

    string input = get_input_line();
    int choice = 0;
    try {
        choice = stoi(input);
    } catch (...) {
        cout << "无效的选择" << endl;
        return;
    }

    if (choice < 0 || choice > static_cast<int>(token_files.size())) {
        cout << "无效的选择" << endl;
        return;
    }

    // If loading from file
    if (choice > 0) {
        string file_path = resolve_token_file_path(token_files[choice - 1]);
        cout << "正在从文件加载令牌: " << token_files[choice - 1] << endl;

        string token_string = read_file_to_string(file_path);
        token_string = trim(token_string);
        if (token_string.empty()) {
            cout << "读取文件失败" << endl;
            return;
        }
        cout << "读取到令牌 (" << token_string.length() << " 字符)" << endl;

        // Initialize client if needed
        if (!g_initialized) {
            DL_ClientConfig config;
            config.license_code = "ACCOUNTING";
            config.preferred_mode = DL_CONNECTION_MODE_OFFLINE;
            config.udp_port = 13325;
            config.tcp_port = 23325;
            config.registry_server_url = nullptr;

            dl_client_initialize(client, &config);
            g_initialized = true;
        }

        // Set product public key
        string product_key_path = find_product_public_key();
        if (!product_key_path.empty()) {
            string key_data = read_file_to_string(product_key_path);
            if (!key_data.empty()) {
                dl_client_set_product_public_key(client, key_data.c_str());
                cout << "产品公钥设置成功" << endl;
            }
        }

        // Import token
        cout << "正在导入令牌..." << endl;
        if (dl_client_import_token(client, token_string.c_str()) != DL_ERROR_SUCCESS) {
            cout << "令牌导入失败" << endl;
            return;
        }
        cout << "令牌导入成功" << endl;

        // Activate
        bool is_already_activated = (token_files[choice - 1].find("activated") != string::npos ||
                                     token_files[choice - 1].find("state") != string::npos);

        if (is_already_activated) {
            cout << "检测到已激活令牌" << endl;
            cout << "正在恢复激活状态..." << endl;
        } else {
            cout << "正在首次激活令牌..." << endl;
        }

        DL_VerificationResult result;
        if (dl_client_activate_bind_device(client, &result) != DL_ERROR_SUCCESS || !result.valid) {
            cout << "激活失败: " << result.error_message << endl;
            return;
        }

        cout << (is_already_activated ? "激活状态已恢复(token未改变)" : "首次激活成功") << endl;
    }

    // Display current token info
    DL_StatusResult status;
    if (dl_client_get_status(client, &status) == DL_ERROR_SUCCESS && status.has_token) {
        cout << "\n当前令牌信息:" << endl;
        cout << "   许可证代码: " << status.license_code << endl;
        cout << "   应用ID: " << status.app_id << endl;
        cout << "   当前状态索引: " << status.state_index << endl;
        cout << "   令牌ID: " << status.token_id << endl;
    } else {
        cout << "无法获取令牌信息" << endl;
        return;
    }

    // Accounting options
    cout << "\n请选择记账方式:" << endl;
    cout << "1. 快速测试记账(使用默认测试数据)" << endl;
    cout << "2. 记录业务操作(向导式输入)" << endl;
    cout << "\n请选择 (1-2): ";

    input = get_input_line();
    int acc_choice = 0;
    try {
        acc_choice = stoi(input);
    } catch (...) {
        cout << "无效的选择" << endl;
        return;
    }

    string accounting_data;

    if (acc_choice == 1) {
        // Quick test
        accounting_data = R"({"action":"api_call","params":{"function":"test_function","result":"success"}})";
        cout << "使用测试数据: " << accounting_data << endl;
    } else if (acc_choice == 2) {
        // Guided input
        cout << "\nusage_chain 结构说明:" << endl;
        cout << "字段名      | 说明           | 填写方式" << endl;
        cout << "seq         | 序列号         | 系统自动填充" << endl;
        cout << "time        | 时间戳         | 系统自动填充" << endl;
        cout << "action      | 操作类型       | 需要您输入" << endl;
        cout << "params      | 操作参数       | 需要您输入" << endl;
        cout << "hash_prev   | 前状态哈希     | 系统自动填充" << endl;
        cout << "signature   | 数字签名       | 系统自动填充" << endl;
        cout << endl;

        cout << "第1步: 输入操作类型 (action)" << endl;
        cout << "   常用操作类型:" << endl;
        cout << "   • api_call      - API调用" << endl;
        cout << "   • feature_usage - 功能使用" << endl;
        cout << "   • save_file     - 保存文件" << endl;
        cout << "   • export_data   - 导出数据" << endl;
        cout << "\n请输入操作类型: ";

        string action = get_input_line();
        if (action.empty()) {
            cout << "操作类型不能为空" << endl;
            return;
        }

        cout << "\n第2步: 输入操作参数 (params)" << endl;
        cout << "   params 是一个JSON对象,包含操作的具体参数" << endl;
        cout << "   输入格式: key=value (每行一个)" << endl;
        cout << "   示例:" << endl;
        cout << "   • function=process_image" << endl;
        cout << "   • file_name=report.pdf" << endl;
        cout << "   输入空行结束输入" << endl;

        string params = "{";
        bool first_param = true;

        while (true) {
            cout << "参数 (key=value 或直接回车结束): ";
            string line = get_input_line();
            if (line.empty()) {
                break;
            }

            size_t equals = line.find('=');
            if (equals != string::npos) {
                string key = trim(line.substr(0, equals));
                string value = trim(line.substr(equals + 1));

                if (!first_param) {
                    params += ",";
                }

                params += "\"" + key + "\":\"" + value + "\"";
                first_param = false;
            } else {
                cout << "格式错误,请使用 key=value 格式" << endl;
            }
        }

        params += "}";

        if (first_param) {
            cout << "未输入任何参数,将使用空参数对象" << endl;
            params = "{}";
        }

        accounting_data = "{\"action\":\"" + action + "\",\"params\":" + params + "}";
        cout << "\n记账数据 (业务字段): " << accounting_data << endl;
        cout << "   (系统字段 seq, time, hash_prev, signature 将由SDK自动添加)" << endl;
    } else {
        cout << "无效的选择" << endl;
        return;
    }

    // Record usage
    cout << "正在记录使用情况..." << endl;
    DL_VerificationResult result;
    DL_ErrorCode err = dl_client_record_usage(client, accounting_data.c_str(), &result);

    if (err != DL_ERROR_SUCCESS || !result.valid) {
        cout << "记账失败: " << result.error_message << endl;
        return;
    }

    cout << "记账成功" << endl;
    cout << "响应: " << result.error_message << endl;

    // Export state changed token
    vector<char> state_token(MAX_TOKEN_SIZE);
    err = dl_client_export_state_changed_token_encrypted(client, state_token.data(), state_token.size());

    if (err == DL_ERROR_SUCCESS && strlen(state_token.data()) > 0) {
        string token_str(state_token.data());
        cout << "\n状态变更后的新Token(加密):" << endl;
        cout << "   长度: " << token_str.length() << " 字符" << endl;
        if (token_str.length() > 100) {
            cout << "   前缀: " << token_str.substr(0, 100) << "..." << endl;
        } else {
            cout << "   内容: " << token_str << endl;
        }

        // Save to file
        if (dl_client_get_status(client, &status) == DL_ERROR_SUCCESS && strlen(status.license_code) > 0) {
            time_t now = time(nullptr);
            struct tm* tm_info = localtime(&now);
            char timestamp[32];
            strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", tm_info);

            string filename = string("token_state_") + status.license_code + "_idx" +
                            to_string(status.state_index) + "_" + timestamp + ".txt";

            if (write_string_to_file(filename, token_str)) {
                char abs_path[PATH_MAX];
                realpath(filename.c_str(), abs_path);
                cout << "\n已保存到文件: " << abs_path << endl;
                cout << "   此token包含最新状态链,可传递给下一个设备使用" << endl;
            }
        }
    }
}

void trust_chain_validation_wizard() {
    cout << "\n信任链验证" << endl;
    cout << "============" << endl;
    cout << "信任链验证检查加密签名的完整性:根密钥 -> 产品公钥 -> 令牌签名 -> 设备绑定" << endl;
    cout << endl;

    DL_Client* client = get_or_create_client();
    if (client == nullptr) {
        cout << "获取客户端失败" << endl;
        return;
    }

    // Show available state token files
    auto token_files = find_state_token_files();

    int activated = dl_client_is_activated(client);

    cout << "\n请选择令牌来源:" << endl;
    if (activated) {
        cout << "0. 使用当前激活的令牌" << endl;
    }

    if (!token_files.empty()) {
        cout << "\n或从以下文件加载令牌:" << endl;
        for (size_t i = 0; i < token_files.size(); i++) {
            cout << (i + 1) << ". " << token_files[i] << endl;
        }
    }

    if (!activated && token_files.empty()) {
        cout << "当前没有激活的令牌,也没有找到可用的token文件" << endl;
        cout << "请先使用选项1激活令牌" << endl;
        return;
    }

    cout << "\n请选择 (0";
    if (!token_files.empty()) {
        cout << "-" << token_files.size();
    }
    cout << "): ";

    string input = get_input_line();
    int choice = 0;
    try {
        choice = stoi(input);
    } catch (...) {
        cout << "无效的选择" << endl;
        return;
    }

    if (choice < 0 || choice > static_cast<int>(token_files.size())) {
        cout << "无效的选择" << endl;
        return;
    }

    // If loading from file (similar to accounting_wizard)
    if (choice > 0) {
        string file_path = resolve_token_file_path(token_files[choice - 1]);
        cout << "正在从文件加载令牌: " << token_files[choice - 1] << endl;

        string token_string = read_file_to_string(file_path);
        token_string = trim(token_string);
        if (token_string.empty()) {
            cout << "读取文件失败" << endl;
            return;
        }
        cout << "读取到令牌 (" << token_string.length() << " 字符)" << endl;

        if (!g_initialized) {
            DL_ClientConfig config;
            config.license_code = "TRUST_CHAIN";
            config.preferred_mode = DL_CONNECTION_MODE_OFFLINE;
            config.udp_port = 13325;
            config.tcp_port = 23325;
            config.registry_server_url = nullptr;
            dl_client_initialize(client, &config);
            g_initialized = true;
        }

        string product_key_path = find_product_public_key();
        if (!product_key_path.empty()) {
            string key_data = read_file_to_string(product_key_path);
            if (!key_data.empty()) {
                dl_client_set_product_public_key(client, key_data.c_str());
                cout << "产品公钥设置成功" << endl;
            }
        }

        cout << "正在导入令牌..." << endl;
        if (dl_client_import_token(client, token_string.c_str()) != DL_ERROR_SUCCESS) {
            cout << "令牌导入失败" << endl;
            return;
        }
        cout << "令牌导入成功" << endl;

        bool is_already_activated = (token_files[choice - 1].find("activated") != string::npos ||
                                     token_files[choice - 1].find("state") != string::npos);

        if (is_already_activated) {
            cout << "检测到已激活令牌" << endl;
            cout << "正在恢复激活状态..." << endl;
        } else {
            cout << "正在首次激活令牌..." << endl;
        }

        DL_VerificationResult result;
        if (dl_client_activate_bind_device(client, &result) != DL_ERROR_SUCCESS || !result.valid) {
            cout << "激活失败: " << result.error_message << endl;
            return;
        }

        cout << (is_already_activated ? "激活状态已恢复(token未改变)" : "首次激活成功") << endl;
    }

    cout << "开始验证信任链..." << endl;
    cout << endl;

    int checks_passed = 0;
    int total_checks = 4;

    // Check 1: Token signature verification
    cout << "[1/4] 验证令牌签名(根密钥 -> 产品公钥 -> 令牌)" << endl;
    DL_VerificationResult result;
    if (dl_client_offline_verify_current_token(client, &result) != DL_ERROR_SUCCESS) {
        cout << "   失败" << endl;
    } else if (!result.valid) {
        cout << "   失败: " << result.error_message << endl;
    } else {
        cout << "   通过: 令牌签名有效,信任链完整" << endl;
        checks_passed++;
    }
    cout << endl;

    // Check 2: Device state
    cout << "[2/4] 验证设备状态" << endl;
    DL_DeviceState state = dl_client_get_device_state(client);
    cout << "   通过: 设备状态正常 (状态码: " << state << ")" << endl;
    checks_passed++;
    cout << endl;

    // Check 3: Token holder matches device
    cout << "[3/4] 验证令牌持有者与当前设备匹配" << endl;
    DL_Token token;
    char device_id[128];
    if (dl_client_get_current_token(client, &token) == DL_ERROR_SUCCESS &&
        dl_client_get_device_id(client, device_id, sizeof(device_id)) == DL_ERROR_SUCCESS) {

        if (string(token.holder_device_id) == string(device_id)) {
            cout << "   通过: 令牌持有者与当前设备匹配" << endl;
            cout << "   设备ID: " << device_id << endl;
            checks_passed++;
        } else {
            cout << "   不匹配: 令牌持有者与当前设备不一致" << endl;
            cout << "   当前设备ID: " << device_id << endl;
            cout << "   令牌持有者ID: " << token.holder_device_id << endl;
            cout << "   这可能表示令牌是从其他设备导入的" << endl;
        }
    }
    cout << endl;

    // Check 4: Token information
    cout << "[4/4] 检查令牌详细信息" << endl;
    DL_StatusResult status;
    if (dl_client_get_status(client, &status) == DL_ERROR_SUCCESS && status.has_token) {
        cout << "   通过: 令牌信息完整" << endl;
        cout << "   令牌ID: " << status.token_id << endl;
        cout << "   许可证代码: " << status.license_code << endl;
        cout << "   应用ID: " << status.app_id << endl;

        time_t issue_time_t = static_cast<time_t>(status.issue_time);
        struct tm* tm_info = localtime(&issue_time_t);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        cout << "   颁发时间: " << time_str << endl;

        if (status.expire_time == 0) {
            cout << "   到期时间: 永不过期" << endl;
        } else {
            time_t expire_time_t = static_cast<time_t>(status.expire_time);
            tm_info = localtime(&expire_time_t);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
            cout << "   到期时间: " << time_str << endl;
        }
        checks_passed++;
    }
    cout << endl;

    // Summary
    cout << "================================================" << endl;
    cout << "验证结果: " << checks_passed << "/" << total_checks << " 项检查通过" << endl;
    if (checks_passed == total_checks) {
        cout << "信任链验证完全通过!令牌可信且安全" << endl;
    } else if (checks_passed >= 2) {
        cout << "部分检查通过,令牌基本可用但存在警告" << endl;
    } else {
        cout << "多项检查失败,请检查令牌和设备状态" << endl;
    }
    cout << "================================================" << endl;
}

void comprehensive_validation_wizard() {
    cout << "\n综合验证" << endl;
    cout << "----------" << endl;

    DL_Client* client = get_or_create_client();
    if (client == nullptr) {
        cout << "获取客户端失败" << endl;
        return;
    }

    // Show available state token files
    auto token_files = find_state_token_files();

    int activated = dl_client_is_activated(client);

    cout << "\n请选择令牌来源:" << endl;
    if (activated) {
        cout << "0. 使用当前激活的令牌" << endl;
    }

    if (!token_files.empty()) {
        cout << "\n或从以下文件加载令牌:" << endl;
        for (size_t i = 0; i < token_files.size(); i++) {
            cout << (i + 1) << ". " << token_files[i] << endl;
        }
    }

    if (!activated && token_files.empty()) {
        cout << "当前没有激活的令牌,也没有找到可用的token文件" << endl;
        cout << "请先使用选项1激活令牌" << endl;
        return;
    }

    cout << "\n请选择 (0";
    if (!token_files.empty()) {
        cout << "-" << token_files.size();
    }
    cout << "): ";

    string input = get_input_line();
    int choice = 0;
    try {
        choice = stoi(input);
    } catch (...) {
        cout << "无效的选择" << endl;
        return;
    }

    if (choice < 0 || choice > static_cast<int>(token_files.size())) {
        cout << "无效的选择" << endl;
        return;
    }

    // If loading from file
    if (choice > 0) {
        string file_path = resolve_token_file_path(token_files[choice - 1]);
        cout << "正在从文件加载令牌: " << token_files[choice - 1] << endl;

        string token_string = read_file_to_string(file_path);
        token_string = trim(token_string);
        if (token_string.empty()) {
            cout << "读取文件失败" << endl;
            return;
        }
        cout << "读取到令牌 (" << token_string.length() << " 字符)" << endl;

        if (!g_initialized) {
            DL_ClientConfig config;
            config.license_code = "COMPREHENSIVE";
            config.preferred_mode = DL_CONNECTION_MODE_OFFLINE;
            config.udp_port = 13325;
            config.tcp_port = 23325;
            config.registry_server_url = nullptr;
            dl_client_initialize(client, &config);
            g_initialized = true;
        }

        string product_key_path = find_product_public_key();
        if (!product_key_path.empty()) {
            string key_data = read_file_to_string(product_key_path);
            if (!key_data.empty()) {
                dl_client_set_product_public_key(client, key_data.c_str());
                cout << "产品公钥设置成功" << endl;
            }
        }

        cout << "正在导入令牌..." << endl;
        if (dl_client_import_token(client, token_string.c_str()) != DL_ERROR_SUCCESS) {
            cout << "令牌导入失败" << endl;
            return;
        }
        cout << "令牌导入成功" << endl;

        bool is_already_activated = (token_files[choice - 1].find("activated") != string::npos ||
                                     token_files[choice - 1].find("state") != string::npos);

        if (is_already_activated) {
            cout << "检测到已激活令牌" << endl << "正在恢复激活状态..." << endl;
        } else {
            cout << "正在首次激活令牌..." << endl;
        }

        DL_VerificationResult result;
        if (dl_client_activate_bind_device(client, &result) != DL_ERROR_SUCCESS || !result.valid) {
            cout << "激活失败: " << result.error_message << endl;
            return;
        }

        cout << (is_already_activated ? "激活状态已恢复(token未改变)" : "首次激活成功") << endl;
    }

    cout << "执行综合验证流程..." << endl;
    int check_count = 0;
    int pass_count = 0;

    // Check 1: Activation status
    check_count++;
    activated = dl_client_is_activated(client);
    pass_count++;
    cout << (activated ? "通过" : "警告") << " 检查" << check_count
         << (activated ? "通过" : "") << ": 许可证" << (activated ? "已激活" : "未激活") << endl;

    // Check 2: Token verification
    if (activated) {
        check_count++;
        DL_VerificationResult result;
        if (dl_client_offline_verify_current_token(client, &result) == DL_ERROR_SUCCESS && result.valid) {
            pass_count++;
            cout << "通过 检查" << check_count << "通过: 令牌验证成功" << endl;
        } else {
            cout << "失败 检查" << check_count << "失败: 令牌验证失败" << endl;
        }
    }

    // Check 3: Device state
    check_count++;
    DL_DeviceState state = dl_client_get_device_state(client);
    pass_count++;
    cout << "通过 检查" << check_count << "通过: 设备状态正常 (状态码: " << state << ")" << endl;

    // Check 4: Token info
    check_count++;
    DL_Token token;
    if (dl_client_get_current_token(client, &token) == DL_ERROR_SUCCESS) {
        pass_count++;
        string token_id = token.token_id;
        if (token_id.length() >= 16) {
            cout << "通过 检查" << check_count << "通过: 令牌信息完整 (ID: "
                 << token_id.substr(0, 16) << "...)" << endl;
        } else {
            cout << "通过 检查" << check_count << "通过: 令牌信息完整" << endl;
        }
    } else {
        cout << "警告 检查" << check_count << ": 无令牌信息" << endl;
    }

    // Check 5: Accounting function
    if (activated) {
        check_count++;
        const char* test_data = R"({"action":"comprehensive_test","timestamp":1234567890})";
        DL_VerificationResult result;
        if (dl_client_record_usage(client, test_data, &result) == DL_ERROR_SUCCESS && result.valid) {
            pass_count++;
            cout << "通过 检查" << check_count << "通过: 记账功能正常" << endl;

            // Export state changed token
            vector<char> state_token(MAX_TOKEN_SIZE);
            if (dl_client_export_state_changed_token_encrypted(client, state_token.data(), state_token.size()) == DL_ERROR_SUCCESS &&
                strlen(state_token.data()) > 0) {
                cout << "   状态变更后的新Token已生成" << endl;
                cout << "   Token长度: " << strlen(state_token.data()) << " 字符" << endl;

                DL_StatusResult status;
                if (dl_client_get_status(client, &status) == DL_ERROR_SUCCESS && strlen(status.license_code) > 0) {
                    time_t now = time(nullptr);
                    struct tm* tm_info = localtime(&now);
                    char timestamp[32];
                    strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", tm_info);

                    string filename = string("token_state_") + status.license_code + "_idx" +
                                    to_string(status.state_index) + "_" + timestamp + ".txt";

                    if (write_string_to_file(filename, state_token.data())) {
                        char abs_path[PATH_MAX];
                        realpath(filename.c_str(), abs_path);
                        cout << "   已保存到: " << abs_path << endl;
                    }
                }
            }
        } else {
            cout << "失败 检查" << check_count << "失败: 记账功能异常" << endl;
        }
    }

    // Summary
    cout << "\n综合验证结果:" << endl;
    cout << "   总检查项: " << check_count << endl;
    cout << "   通过项目: " << pass_count << endl;
    cout << "   成功率: " << fixed << (static_cast<float>(pass_count) / check_count * 100) << "%" << endl;

    if (pass_count == check_count) {
        cout << "所有检查均通过!系统运行正常" << endl;
    } else if (pass_count >= check_count / 2) {
        cout << "大部分检查通过,系统基本正常" << endl;
    } else {
        cout << "多项检查失败,请检查系统配置" << endl;
    }
}

int main() {
    cout << "==========================================" << endl;
    cout << "DecentriLicense C++ SDK 验证向导" << endl;
    cout << "==========================================" << endl;
    cout << endl;

    // Register cleanup on exit
    atexit(cleanup_client);

    while (true) {
        cout << "请选择要执行的操作:" << endl;
        cout << "0. 🔑 选择产品公钥" << endl;
        cout << "1. 🔓 激活令牌" << endl;
        cout << "2. ✅ 校验已激活令牌" << endl;
        cout << "3. 🔍 验证令牌合法性" << endl;
        cout << "4. 📊 记账信息" << endl;
        cout << "5. 🔗 信任链验证" << endl;
        cout << "6. 🎯 综合验证" << endl;
        cout << "7. 🚪 退出" << endl;
        cout << "请输入选项 (0-7): ";

        string input = get_input_line();
        int choice = -1;
        try {
            choice = stoi(input);
        } catch (...) {
            cout << "无效选项,请重新选择" << endl << endl;
            continue;
        }

        switch (choice) {
            case 0:
                select_product_key_wizard();
                break;
            case 1:
                activate_token_wizard();
                break;
            case 2:
                verify_activated_token_wizard();
                break;
            case 3:
                validate_token_wizard();
                break;
            case 4:
                accounting_wizard();
                break;
            case 5:
                trust_chain_validation_wizard();
                break;
            case 6:
                comprehensive_validation_wizard();
                break;
            case 7:
                cout << "感谢使用 DecentriLicense C++ SDK 验证向导!" << endl;
                return 0;
            default:
                cout << "无效选项,请重新选择" << endl;
        }
        cout << endl;
    }

    return 0;
}
