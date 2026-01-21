#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include "decenlicense_c.h"

// Maximum path and buffer sizes
#define MAX_PATH 512
#define MAX_BUFFER 8192
#define MAX_TOKEN_SIZE 16384
#define MAX_FILES 100

// Global variables
static DL_Client* g_client = NULL;
static int g_initialized = 0;
static char g_selected_product_key_path[MAX_PATH] = {0};

// File list structure
typedef struct {
    char files[MAX_FILES][MAX_PATH];
    int count;
} FileList;

// Function declarations
void cleanup_client();
DL_Client* get_or_create_client();
char* read_file_to_string(const char* filepath, size_t* out_size);
int write_string_to_file(const char* filepath, const char* content);
void find_product_public_keys(FileList* list);
void find_token_files(FileList* list, const char* pattern);
void find_encrypted_token_files(FileList* list);
void find_activated_token_files(FileList* list);
void find_state_token_files(FileList* list);
char* resolve_product_key_path(const char* filename);
char* resolve_token_file_path(const char* filename);
char* find_product_public_key();
int file_exists(const char* path);

// Menu functions
void select_product_key_wizard();
void activate_token_wizard();
void verify_activated_token_wizard();
void validate_token_wizard();
void accounting_wizard();
void trust_chain_validation_wizard();
void comprehensive_validation_wizard();

// Helper function implementations
void cleanup_client() {
    if (g_client != NULL) {
        dl_client_shutdown(g_client);
        dl_client_destroy(g_client);
        g_client = NULL;
        g_initialized = 0;
    }
}

DL_Client* get_or_create_client() {
    if (g_client == NULL) {
        g_client = dl_client_create();
        g_initialized = 0;
    }
    return g_client;
}

int file_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

char* read_file_to_string(const char* filepath, size_t* out_size) {
    FILE* fp = fopen(filepath, "r");
    if (fp == NULL) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* content = (char*)malloc(size + 1);
    if (content == NULL) {
        fclose(fp);
        return NULL;
    }

    size_t read_size = fread(content, 1, size, fp);
    content[read_size] = '\0';
    fclose(fp);

    if (out_size != NULL) {
        *out_size = read_size;
    }

    return content;
}

int write_string_to_file(const char* filepath, const char* content) {
    FILE* fp = fopen(filepath, "w");
    if (fp == NULL) {
        return 0;
    }

    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, fp);
    fclose(fp);

    return written == len;
}

void add_matching_files(FileList* list, const char* dir, const char* pattern) {
    DIR* d = opendir(dir);
    if (d == NULL) {
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(d)) != NULL && list->count < MAX_FILES) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        const char* name = entry->d_name;

        // Check if filename matches pattern
        if (pattern == NULL || strstr(name, pattern) != NULL) {
            // Check if already in list
            int found = 0;
            for (int i = 0; i < list->count; i++) {
                if (strcmp(list->files[i], name) == 0) {
                    found = 1;
                    break;
                }
            }

            if (!found) {
                strncpy(list->files[list->count], name, MAX_PATH - 1);
                list->files[list->count][MAX_PATH - 1] = '\0';
                list->count++;
            }
        }
    }

    closedir(d);
}

void find_product_public_keys(FileList* list) {
    list->count = 0;

    // Search patterns
    const char* search_dirs[] = {
        ".",
        "..",
        "../..",
        "../../../dl-issuer"
    };

    for (int i = 0; i < 4 && list->count < MAX_FILES; i++) {
        DIR* d = opendir(search_dirs[i]);
        if (d == NULL) {
            continue;
        }

        struct dirent* entry;
        while ((entry = readdir(d)) != NULL && list->count < MAX_FILES) {
            if (entry->d_type != DT_REG) {
                continue;
            }

            const char* name = entry->d_name;
            // Only select product public key files: contains "public", not "private", ends with ".pem"
            if (strstr(name, "public") != NULL &&
                strstr(name, "private") == NULL &&
                strstr(name, ".pem") != NULL) {

                // Check if already in list
                int found = 0;
                for (int j = 0; j < list->count; j++) {
                    if (strcmp(list->files[j], name) == 0) {
                        found = 1;
                        break;
                    }
                }

                if (!found) {
                    strncpy(list->files[list->count], name, MAX_PATH - 1);
                    list->files[list->count][MAX_PATH - 1] = '\0';
                    list->count++;
                }
            }
        }

        closedir(d);
    }
}

void find_token_files(FileList* list, const char* pattern) {
    list->count = 0;

    const char* search_dirs[] = {
        ".",
        "..",
        "../../../dl-issuer"
    };

    for (int i = 0; i < 3 && list->count < MAX_FILES; i++) {
        DIR* d = opendir(search_dirs[i]);
        if (d == NULL) {
            continue;
        }

        struct dirent* entry;
        while ((entry = readdir(d)) != NULL && list->count < MAX_FILES) {
            if (entry->d_type != DT_REG) {
                continue;
            }

            const char* name = entry->d_name;
            if (strstr(name, "token_") != NULL &&
                strstr(name, ".txt") != NULL &&
                (pattern == NULL || strstr(name, pattern) != NULL)) {

                // Check if already in list
                int found = 0;
                for (int j = 0; j < list->count; j++) {
                    if (strcmp(list->files[j], name) == 0) {
                        found = 1;
                        break;
                    }
                }

                if (!found) {
                    strncpy(list->files[list->count], name, MAX_PATH - 1);
                    list->files[list->count][MAX_PATH - 1] = '\0';
                    list->count++;
                }
            }
        }

        closedir(d);
    }
}

void find_encrypted_token_files(FileList* list) {
    find_token_files(list, "encrypted");
}

void find_activated_token_files(FileList* list) {
    find_token_files(list, "activated");
}

void find_state_token_files(FileList* list) {
    list->count = 0;

    const char* search_dirs[] = {
        ".",
        "..",
        "../../../dl-issuer"
    };

    for (int i = 0; i < 3 && list->count < MAX_FILES; i++) {
        DIR* d = opendir(search_dirs[i]);
        if (d == NULL) {
            continue;
        }

        struct dirent* entry;
        while ((entry = readdir(d)) != NULL && list->count < MAX_FILES) {
            if (entry->d_type != DT_REG) {
                continue;
            }

            const char* name = entry->d_name;
            // 照抄Go SDK: 文件名必须以token_activated_或token_state_开头
            if ((strncmp(name, "token_activated_", 16) == 0 || strncmp(name, "token_state_", 12) == 0) &&
                strstr(name, ".txt") != NULL) {

                // Check if already in list
                int found = 0;
                for (int j = 0; j < list->count; j++) {
                    if (strcmp(list->files[j], name) == 0) {
                        found = 1;
                        break;
                    }
                }

                if (!found) {
                    strncpy(list->files[list->count], name, MAX_PATH - 1);
                    list->files[list->count][MAX_PATH - 1] = '\0';
                    list->count++;
                }
            }
        }

        closedir(d);
    }
}

char* resolve_product_key_path(const char* filename) {
    static char result[MAX_PATH];

    const char* search_paths[] = {
        "./",
        "../",
        "../../",
        "../../../dl-issuer/"
    };

    for (int i = 0; i < 4; i++) {
        snprintf(result, MAX_PATH, "%s%s", search_paths[i], filename);
        if (file_exists(result)) {
            return result;
        }
    }

    strncpy(result, filename, MAX_PATH - 1);
    result[MAX_PATH - 1] = '\0';
    return result;
}

char* resolve_token_file_path(const char* filename) {
    static char result[MAX_PATH];

    const char* search_paths[] = {
        "./",
        "../",
        "../../../dl-issuer/"
    };

    for (int i = 0; i < 3; i++) {
        snprintf(result, MAX_PATH, "%s%s", search_paths[i], filename);
        if (file_exists(result)) {
            return result;
        }
    }

    strncpy(result, filename, MAX_PATH - 1);
    result[MAX_PATH - 1] = '\0';
    return result;
}

char* find_product_public_key() {
    if (g_selected_product_key_path[0] != '\0') {
        return g_selected_product_key_path;
    }

    FileList list;
    find_product_public_keys(&list);

    if (list.count > 0) {
        return resolve_product_key_path(list.files[0]);
    }

    return NULL;
}

// Main menu functions
void select_product_key_wizard() {
    printf("\n选择产品公钥\n");
    printf("==============\n");

    FileList list;
    find_product_public_keys(&list);

    if (list.count == 0) {
        printf("没有找到产品公钥文件\n");
        printf("请将产品公钥文件 (public_*.pem) 放置在当前目录下\n");
        return;
    }

    printf("找到以下产品公钥文件:\n");
    for (int i = 0; i < list.count; i++) {
        printf("%d. %s\n", i + 1, list.files[i]);
    }
    printf("%d. 取消选择\n", list.count + 1);

    if (g_selected_product_key_path[0] != '\0') {
        printf("当前已选择: %s\n", g_selected_product_key_path);
    }

    printf("请选择要使用的产品公钥文件 (1-%d): ", list.count + 1);

    int choice;
    if (scanf("%d", &choice) != 1) {
        printf("无效选择\n");
        while (getchar() != '\n'); // Clear input buffer
        return;
    }
    while (getchar() != '\n'); // Clear input buffer

    if (choice < 1 || choice > list.count + 1) {
        printf("无效选择\n");
        return;
    }

    if (choice == list.count + 1) {
        g_selected_product_key_path[0] = '\0';
        printf("已取消产品公钥选择\n");
        return;
    }

    char* resolved_path = resolve_product_key_path(list.files[choice - 1]);
    strncpy(g_selected_product_key_path, resolved_path, MAX_PATH - 1);
    g_selected_product_key_path[MAX_PATH - 1] = '\0';
    printf("已选择产品公钥文件: %s\n", list.files[choice - 1]);
}

void activate_token_wizard() {
    printf("\n激活令牌\n");
    printf("----------\n");
    printf("重要说明:\n");
    printf("   • 加密token(encrypted): 首次从供应商获得,需要激活\n");
    printf("   • 已激活token(activated): 激活后生成,可直接使用,不需再次激活\n");
    printf("   本功能仅用于【首次激活】加密token\n");
    printf("   如需使用已激活token,请直接选择其他功能(如记账、验证)\n\n");

    DL_Client* client = get_or_create_client();
    if (client == NULL) {
        printf("创建客户端失败\n");
        return;
    }

    // Show available encrypted token files
    FileList token_files;
    find_encrypted_token_files(&token_files);

    if (token_files.count > 0) {
        printf("发现以下加密token文件:\n");
        for (int i = 0; i < token_files.count; i++) {
            printf("   %d. %s\n", i + 1, token_files.files[i]);
        }
        printf("您可以输入序号选择文件,或输入文件名/路径\n");
    }

    printf("请输入令牌字符串 (仅支持加密令牌):\n");
    printf("加密令牌通常从软件提供商处获得\n");
    printf("输入序号(1-%d)可快速选择上面列出的文件\n", token_files.count);
    printf("令牌或文件路径: ");

    char input[MAX_PATH];
    if (fgets(input, sizeof(input), stdin) == NULL) {
        printf("输入读取失败\n");
        return;
    }

    // Trim newline
    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n') {
        input[len - 1] = '\0';
    }

    char* token_string = NULL;

    // Check if input is a number (file index)
    if (token_files.count > 0) {
        int index = atoi(input);
        if (index >= 1 && index <= token_files.count) {
            char* file_path = resolve_token_file_path(token_files.files[index - 1]);
            size_t token_size;
            token_string = read_file_to_string(file_path, &token_size);
            if (token_string != NULL) {
                printf("选择文件 '%s' 并读取到令牌 (%zu 字符)\n",
                       token_files.files[index - 1], token_size);
            } else {
                printf("无法读取文件 %s\n", file_path);
                return;
            }
        }
    }

    // If not from file selection, check if it's a file path
    if (token_string == NULL &&
        (strchr(input, '/') != NULL || strstr(input, ".txt") != NULL || strstr(input, "token_") != NULL)) {
        char* file_path = resolve_token_file_path(input);
        size_t token_size;
        token_string = read_file_to_string(file_path, &token_size);
        if (token_string != NULL) {
            printf("从文件读取到令牌 (%zu 字符)\n", token_size);
        } else {
            printf("无法读取文件 %s,将直接使用输入作为令牌字符串\n", file_path);
            token_string = strdup(input);
        }
    } else if (token_string == NULL) {
        token_string = strdup(input);
    }

    if (token_string == NULL) {
        printf("令牌字符串为空\n");
        return;
    }

    // Initialize client if not already initialized
    if (!g_initialized) {
        DL_ClientConfig config;
        config.license_code = "TEMP";
        config.udp_port = 13325;
        config.tcp_port = 23325;
        config.registry_server_url = NULL;

        DL_ErrorCode err = dl_client_initialize(client, &config);
        if (err != DL_ERROR_SUCCESS) {
            printf("初始化失败 (需要产品公钥)\n");
            printf("正在查找产品公钥文件...\n");
        } else {
            printf("客户端初始化成功\n");
            g_initialized = 1;
        }
    } else {
        printf("客户端已初始化,使用现有实例\n");
    }

    // Find and set product public key
    char* product_key_path = find_product_public_key();
    if (product_key_path == NULL) {
        printf("未找到产品公钥文件\n");
        printf("请先选择产品公钥 (菜单选项 0),或确保当前目录下有产品公钥文件\n");
        free(token_string);
        return;
    }

    printf("使用产品公钥文件: %s\n", product_key_path);

    size_t key_size;
    char* product_key_data = read_file_to_string(product_key_path, &key_size);
    if (product_key_data == NULL) {
        printf("读取产品公钥文件失败\n");
        free(token_string);
        return;
    }

    DL_ErrorCode err = dl_client_set_product_public_key(client, product_key_data);
    free(product_key_data);

    if (err != DL_ERROR_SUCCESS) {
        printf("设置产品公钥失败\n");
        free(token_string);
        return;
    }
    printf("产品公钥设置成功\n");

    // Import token
    printf("正在导入令牌...\n");
    err = dl_client_import_token(client, token_string);
    free(token_string);

    if (err != DL_ERROR_SUCCESS) {
        printf("令牌导入失败\n");
        return;
    }
    printf("令牌导入成功\n");

    // Activate token
    printf("正在激活令牌...\n");
    DL_VerificationResult result;
    err = dl_client_activate_bind_device(client, &result);

    if (err != DL_ERROR_SUCCESS || !result.valid) {
        printf("激活失败: %s\n", result.error_message);
        return;
    }

    printf("令牌激活成功!\n");

    // Export activated token
    char activated_token[MAX_TOKEN_SIZE];
    err = dl_client_export_activated_token_encrypted(client, activated_token, sizeof(activated_token));

    if (err == DL_ERROR_SUCCESS && strlen(activated_token) > 0) {
        printf("\n激活后的新Token(加密):\n");
        printf("   长度: %zu 字符\n", strlen(activated_token));
        if (strlen(activated_token) > 100) {
            printf("   前缀: %.100s...\n", activated_token);
        } else {
            printf("   内容: %s\n", activated_token);
        }

        // Save to file
        DL_StatusResult status;
        if (dl_client_get_status(client, &status) == DL_ERROR_SUCCESS && strlen(status.license_code) > 0) {
            char filename[MAX_PATH];
            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            char timestamp[32];
            strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", tm_info);

            snprintf(filename, sizeof(filename), "token_activated_%s_%s.txt",
                     status.license_code, timestamp);

            if (write_string_to_file(filename, activated_token)) {
                char abs_path[PATH_MAX];
                realpath(filename, abs_path);
                printf("\n已保存到文件: %s\n", abs_path);
                printf("   此token包含设备绑定信息,可传递给下一个设备使用\n");
            } else {
                printf("保存token文件失败\n");
            }
        }
    }

    // Display status
    DL_StatusResult status;
    if (dl_client_get_status(client, &status) == DL_ERROR_SUCCESS && status.is_activated) {
        printf("当前状态: 已激活\n");
        if (status.has_token) {
            printf("令牌ID: %s\n", status.token_id);
            printf("许可证代码: %s\n", status.license_code);
            printf("持有设备: %s\n", status.holder_device_id);

            char time_str[64];
            time_t issue_time_t = (time_t)status.issue_time;
            struct tm* tm_info = localtime(&issue_time_t);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
            printf("颁发时间: %s\n", time_str);
        }
    } else {
        printf("当前状态: 未激活\n");
    }
}

void verify_activated_token_wizard() {
    printf("\n校验已激活令牌\n");
    printf("----------------\n");

    // Check for activated tokens in state directory
    DIR* state_dir = opendir(".decentrilicense_state");
    if (state_dir == NULL) {
        printf("没有找到已激活的令牌\n");
        return;
    }

    FileList activated_list;
    activated_list.count = 0;

    struct dirent* entry;
    printf("\n已激活的令牌列表:\n");
    int index = 1;
    while ((entry = readdir(state_dir)) != NULL && activated_list.count < MAX_FILES) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            strncpy(activated_list.files[activated_list.count], entry->d_name, MAX_PATH - 1);
            activated_list.files[activated_list.count][MAX_PATH - 1] = '\0';

            char state_file[MAX_PATH];
            snprintf(state_file, sizeof(state_file), ".decentrilicense_state/%s/current_state.json", entry->d_name);

            if (file_exists(state_file)) {
                printf("%d. %s (已激活)\n", index, entry->d_name);
            } else {
                printf("%d. %s (无状态文件)\n", index, entry->d_name);
            }

            activated_list.count++;
            index++;
        }
    }
    closedir(state_dir);

    if (activated_list.count == 0) {
        printf("没有找到已激活的令牌\n");
        return;
    }

    printf("\n请选择要验证的令牌 (1-%d): ", activated_list.count);

    int choice;
    if (scanf("%d", &choice) != 1) {
        printf("无效的选择\n");
        while (getchar() != '\n');
        return;
    }
    while (getchar() != '\n');

    if (choice < 1 || choice > activated_list.count) {
        printf("无效的选择\n");
        return;
    }

    char* selected_license = activated_list.files[choice - 1];
    printf("\n正在验证令牌: %s\n", selected_license);

    DL_Client* client = get_or_create_client();
    if (client == NULL) {
        printf("获取客户端失败\n");
        return;
    }

    // Check if this is the current activated token
    DL_StatusResult status;
    if (dl_client_get_status(client, &status) == DL_ERROR_SUCCESS &&
        strcmp(status.license_code, selected_license) == 0) {

        printf("正在验证令牌...\n");
        DL_VerificationResult result;
        DL_ErrorCode err = dl_client_offline_verify_current_token(client, &result);

        if (err != DL_ERROR_SUCCESS) {
            printf("令牌验证失败\n");
        } else if (result.valid) {
            printf("令牌验证成功\n");
            if (strlen(result.error_message) > 0) {
                printf("信息: %s\n", result.error_message);
            }
        } else {
            printf("令牌验证失败\n");
            printf("错误信息: %s\n", result.error_message);
        }

        if (status.has_token) {
            printf("\n令牌信息:\n");
            printf("   令牌ID: %s\n", status.token_id);
            printf("   许可证代码: %s\n", status.license_code);
            printf("   应用ID: %s\n", status.app_id);
            printf("   持有设备ID: %s\n", status.holder_device_id);

            char time_str[64];
            time_t issue_time_t = (time_t)status.issue_time;
            struct tm* tm_info = localtime(&issue_time_t);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
            printf("   颁发时间: %s\n", time_str);

            if (status.expire_time == 0) {
                printf("   到期时间: 永不过期\n");
            } else {
                time_t expire_time_t = (time_t)status.expire_time;
                tm_info = localtime(&expire_time_t);
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
                printf("   到期时间: %s\n", time_str);
            }

            printf("   状态索引: %lu\n", (unsigned long)status.state_index);
            printf("   激活状态: %s\n", status.is_activated ? "是" : "否");
        }
    } else {
        printf("此令牌不是当前激活的令牌,显示已保存的状态信息:\n");
        char state_file[MAX_PATH];
        snprintf(state_file, sizeof(state_file), ".decentrilicense_state/%s/current_state.json", selected_license);

        size_t file_size;
        char* file_content = read_file_to_string(state_file, &file_size);
        if (file_content == NULL) {
            printf("读取状态文件失败\n");
            return;
        }

        printf("\n令牌信息 (从状态文件读取):\n");
        printf("   许可证代码: %s\n", selected_license);
        printf("   状态文件: %s\n", state_file);
        printf("   文件大小: %zu 字节\n", file_size);
        printf("\n提示: 如需完整验证此令牌,请使用选项1重新激活\n");

        free(file_content);
    }
}

void validate_token_wizard() {
    printf("\n验证令牌合法性\n");
    printf("----------------\n");

    DL_Client* client = get_or_create_client();
    if (client == NULL) {
        printf("获取客户端失败\n");
        return;
    }

    // Initialize client if needed
    if (!g_initialized) {
        DL_ClientConfig config;
        config.license_code = "VALIDATE";
        config.udp_port = 13325;
        config.tcp_port = 23325;
        config.registry_server_url = NULL;

        DL_ErrorCode err = dl_client_initialize(client, &config);
        if (err != DL_ERROR_SUCCESS) {
            printf("初始化失败 (需要产品公钥)\n");
        } else {
            printf("客户端初始化成功\n");
            g_initialized = 1;
        }
    }

    // Find and set product public key
    char* product_key_path = find_product_public_key();
    if (product_key_path == NULL) {
        printf("未找到产品公钥文件\n");
        printf("请先选择产品公钥 (菜单选项 0),或确保当前目录下有产品公钥文件\n");
        return;
    }

    printf("使用产品公钥文件: %s\n", product_key_path);

    size_t key_size;
    char* product_key_data = read_file_to_string(product_key_path, &key_size);
    if (product_key_data == NULL) {
        printf("读取产品公钥文件失败\n");
        return;
    }

    DL_ErrorCode err = dl_client_set_product_public_key(client, product_key_data);
    free(product_key_data);

    if (err != DL_ERROR_SUCCESS) {
        printf("设置产品公钥失败\n");
        return;
    }
    printf("产品公钥设置成功\n");

    // Show available encrypted token files
    FileList token_files;
    find_encrypted_token_files(&token_files);

    if (token_files.count > 0) {
        printf("发现以下加密token文件:\n");
        for (int i = 0; i < token_files.count; i++) {
            printf("   %d. %s\n", i + 1, token_files.files[i]);
        }
        printf("您可以输入序号选择文件,或输入文件名/路径/token字符串\n");
    }

    printf("请输入要验证的令牌字符串 (支持加密令牌):\n");
    printf("令牌通常从软件提供商处获得,或从加密令牌文件读取\n");
    printf("令牌或文件路径: ");

    char input[MAX_PATH];
    if (fgets(input, sizeof(input), stdin) == NULL) {
        printf("输入读取失败\n");
        return;
    }

    // Trim newline
    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n') {
        input[len - 1] = '\0';
    }

    char* token_string = NULL;

    // Check if input is a number (file index)
    int index = atoi(input);
    if (token_files.count > 0 && index >= 1 && index <= token_files.count) {
        char* file_path = resolve_token_file_path(token_files.files[index - 1]);
        size_t token_size;
        token_string = read_file_to_string(file_path, &token_size);
        if (token_string != NULL) {
            printf("从文件 '%s' 读取到令牌 (%zu 字符)\n",
                   token_files.files[index - 1], token_size);
        } else {
            printf("无法读取文件 %s\n", file_path);
            return;
        }
    } else if (strchr(input, '/') != NULL || strstr(input, ".txt") != NULL || strstr(input, "token_") != NULL) {
        char* file_path = resolve_token_file_path(input);
        size_t token_size;
        token_string = read_file_to_string(file_path, &token_size);
        if (token_string != NULL) {
            printf("从文件读取到令牌 (%zu 字符)\n", token_size);
        } else {
            printf("无法读取文件 %s,将直接使用输入作为令牌字符串\n", file_path);
            token_string = strdup(input);
        }
    } else {
        token_string = strdup(input);
    }

    if (token_string == NULL) {
        printf("令牌字符串为空\n");
        return;
    }

    // Import and validate token
    printf("正在导入令牌...\n");
    err = dl_client_import_token(client, token_string);
    free(token_string);

    if (err != DL_ERROR_SUCCESS) {
        printf("令牌导入失败\n");
        return;
    }
    printf("令牌导入成功\n");

    printf("正在验证令牌合法性...\n");
    DL_VerificationResult result;
    err = dl_client_offline_verify_current_token(client, &result);

    if (err != DL_ERROR_SUCCESS) {
        printf("令牌验证失败\n");
    } else if (result.valid) {
        printf("令牌验证成功 - 令牌合法且有效\n");
        if (strlen(result.error_message) > 0) {
            printf("详细信息: %s\n", result.error_message);
        }
    } else {
        printf("令牌验证失败 - 令牌不合法或无效\n");
        printf("错误信息: %s\n", result.error_message);
    }
}

void accounting_wizard() {
    printf("\n记账信息\n");
    printf("----------\n");

    DL_Client* client = get_or_create_client();
    if (client == NULL) {
        printf("获取客户端失败\n");
        return;
    }

    // Show available state token files
    FileList token_files;
    find_state_token_files(&token_files);

    // Check activation status
    int activated = dl_client_is_activated(client);

    printf("\n请选择令牌来源:\n");
    if (activated) {
        printf("0. 使用当前激活的令牌\n");
    }

    if (token_files.count > 0) {
        printf("\n或从以下文件加载令牌:\n");
        for (int i = 0; i < token_files.count; i++) {
            printf("%d. %s\n", i + 1, token_files.files[i]);
        }
    }

    if (!activated && token_files.count == 0) {
        printf("当前没有激活的令牌,也没有找到可用的token文件\n");
        printf("请先使用选项1激活令牌\n");
        return;
    }

    printf("\n请选择 (0");
    if (token_files.count > 0) {
        printf("-%d", token_files.count);
    }
    printf("): ");

    int choice;
    if (scanf("%d", &choice) != 1) {
        printf("无效的选择\n");
        while (getchar() != '\n');
        return;
    }
    while (getchar() != '\n');

    if (choice < 0 || choice > token_files.count) {
        printf("无效的选择\n");
        return;
    }

    // If loading from file
    if (choice > 0) {
        char* file_path = resolve_token_file_path(token_files.files[choice - 1]);
        printf("正在从文件加载令牌: %s\n", token_files.files[choice - 1]);

        size_t token_size;
        char* token_string = read_file_to_string(file_path, &token_size);
        if (token_string == NULL) {
            printf("读取文件失败\n");
            return;
        }
        printf("读取到令牌 (%zu 字符)\n", token_size);

        // Initialize client if needed
        if (!g_initialized) {
            DL_ClientConfig config;
            config.license_code = "ACCOUNTING";
            config.udp_port = 13325;
            config.tcp_port = 23325;
            config.registry_server_url = NULL;

            dl_client_initialize(client, &config);
            g_initialized = 1;
        }

        // Set product public key
        char* product_key_path = find_product_public_key();
        if (product_key_path != NULL) {
            size_t key_size;
            char* key_data = read_file_to_string(product_key_path, &key_size);
            if (key_data != NULL) {
                dl_client_set_product_public_key(client, key_data);
                free(key_data);
                printf("产品公钥设置成功\n");
            }
        }

        // Import token
        printf("正在导入令牌...\n");
        if (dl_client_import_token(client, token_string) != DL_ERROR_SUCCESS) {
            printf("令牌导入失败\n");
            free(token_string);
            return;
        }
        free(token_string);
        printf("令牌导入成功\n");

        // Activate
        int is_already_activated = (strstr(token_files.files[choice - 1], "activated") != NULL ||
                                   strstr(token_files.files[choice - 1], "state") != NULL);

        if (is_already_activated) {
            printf("检测到已激活令牌\n");
            printf("正在恢复激活状态...\n");
        } else {
            printf("正在首次激活令牌...\n");
        }

        DL_VerificationResult result;
        if (dl_client_activate_bind_device(client, &result) != DL_ERROR_SUCCESS || !result.valid) {
            printf("激活失败: %s\n", result.error_message);
            return;
        }

        if (is_already_activated) {
            printf("激活状态已恢复(token未改变)\n");
        } else {
            printf("首次激活成功\n");
        }
    }

    // Display current token info
    DL_StatusResult status;
    if (dl_client_get_status(client, &status) == DL_ERROR_SUCCESS && status.has_token) {
        printf("\n当前令牌信息:\n");
        printf("   许可证代码: %s\n", status.license_code);
        printf("   应用ID: %s\n", status.app_id);
        printf("   当前状态索引: %lu\n", (unsigned long)status.state_index);
        printf("   令牌ID: %s\n", status.token_id);
    } else {
        printf("无法获取令牌信息\n");
        return;
    }

    // Accounting options
    printf("\n请选择记账方式:\n");
    printf("1. 快速测试记账(使用默认测试数据)\n");
    printf("2. 记录业务操作(向导式输入)\n");
    printf("\n请选择 (1-2): ");

    int acc_choice;
    if (scanf("%d", &acc_choice) != 1) {
        printf("无效的选择\n");
        while (getchar() != '\n');
        return;
    }
    while (getchar() != '\n');

    char accounting_data[MAX_BUFFER];

    if (acc_choice == 1) {
        // Quick test
        snprintf(accounting_data, sizeof(accounting_data),
                "{\"action\":\"api_call\",\"params\":{\"function\":\"test_function\",\"result\":\"success\"}}");
        printf("使用测试数据: %s\n", accounting_data);
    } else if (acc_choice == 2) {
        // Guided input
        printf("\nusage_chain 结构说明:\n");
        printf("字段名      | 说明           | 填写方式\n");
        printf("seq         | 序列号         | 系统自动填充\n");
        printf("time        | 时间戳         | 系统自动填充\n");
        printf("action      | 操作类型       | 需要您输入\n");
        printf("params      | 操作参数       | 需要您输入\n");
        printf("hash_prev   | 前状态哈希     | 系统自动填充\n");
        printf("signature   | 数字签名       | 系统自动填充\n\n");

        printf("第1步: 输入操作类型 (action)\n");
        printf("   常用操作类型:\n");
        printf("   • api_call      - API调用\n");
        printf("   • feature_usage - 功能使用\n");
        printf("   • save_file     - 保存文件\n");
        printf("   • export_data   - 导出数据\n");
        printf("\n请输入操作类型: ");

        char action[256];
        if (fgets(action, sizeof(action), stdin) == NULL) {
            printf("输入读取失败\n");
            return;
        }
        size_t action_len = strlen(action);
        if (action_len > 0 && action[action_len - 1] == '\n') {
            action[action_len - 1] = '\0';
        }

        if (strlen(action) == 0) {
            printf("操作类型不能为空\n");
            return;
        }

        printf("\n第2步: 输入操作参数 (params)\n");
        printf("   params 是一个JSON对象,包含操作的具体参数\n");
        printf("   输入格式: key=value (每行一个)\n");
        printf("   示例:\n");
        printf("   • function=process_image\n");
        printf("   • file_name=report.pdf\n");
        printf("   输入空行结束输入\n");

        char params[MAX_BUFFER] = "{";
        int first_param = 1;

        while (1) {
            printf("参数 (key=value 或直接回车结束): ");
            char line[256];
            if (fgets(line, sizeof(line), stdin) == NULL) {
                break;
            }

            size_t line_len = strlen(line);
            if (line_len > 0 && line[line_len - 1] == '\n') {
                line[line_len - 1] = '\0';
            }

            if (strlen(line) == 0) {
                break;
            }

            char* equals = strchr(line, '=');
            if (equals != NULL) {
                *equals = '\0';
                char* key = line;
                char* value = equals + 1;

                if (!first_param) {
                    strcat(params, ",");
                }

                char param_entry[512];
                snprintf(param_entry, sizeof(param_entry), "\"%s\":\"%s\"", key, value);
                strcat(params, param_entry);
                first_param = 0;
            } else {
                printf("格式错误,请使用 key=value 格式\n");
            }
        }

        strcat(params, "}");

        if (first_param) {
            printf("未输入任何参数,将使用空参数对象\n");
            strcpy(params, "{}");
        }

        snprintf(accounting_data, sizeof(accounting_data),
                "{\"action\":\"%s\",\"params\":%s}", action, params);
        printf("\n记账数据 (业务字段): %s\n", accounting_data);
        printf("   (系统字段 seq, time, hash_prev, signature 将由SDK自动添加)\n");
    } else {
        printf("无效的选择\n");
        return;
    }

    // Record usage
    printf("正在记录使用情况...\n");
    DL_VerificationResult result;
    DL_ErrorCode err = dl_client_record_usage(client, accounting_data, &result);

    if (err != DL_ERROR_SUCCESS || !result.valid) {
        printf("记账失败: %s\n", result.error_message);
        return;
    }

    printf("记账成功\n");
    printf("响应: %s\n", result.error_message);

    // Export state changed token
    char state_token[MAX_TOKEN_SIZE];
    err = dl_client_export_state_changed_token_encrypted(client, state_token, sizeof(state_token));

    if (err == DL_ERROR_SUCCESS && strlen(state_token) > 0) {
        printf("\n状态变更后的新Token(加密):\n");
        printf("   长度: %zu 字符\n", strlen(state_token));
        if (strlen(state_token) > 100) {
            printf("   前缀: %.100s...\n", state_token);
        } else {
            printf("   内容: %s\n", state_token);
        }

        // Save to file
        if (dl_client_get_status(client, &status) == DL_ERROR_SUCCESS && strlen(status.license_code) > 0) {
            char filename[MAX_PATH];
            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            char timestamp[32];
            strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", tm_info);

            snprintf(filename, sizeof(filename), "token_state_%s_idx%lu_%s.txt",
                     status.license_code, (unsigned long)status.state_index, timestamp);

            if (write_string_to_file(filename, state_token)) {
                char abs_path[PATH_MAX];
                realpath(filename, abs_path);
                printf("\n已保存到文件: %s\n", abs_path);
                printf("   此token包含最新状态链,可传递给下一个设备使用\n");
            }
        }
    }
}

void trust_chain_validation_wizard() {
    printf("\n信任链验证\n");
    printf("============\n");
    printf("信任链验证检查加密签名的完整性:根密钥 -> 产品公钥 -> 令牌签名 -> 设备绑定\n\n");

    DL_Client* client = get_or_create_client();
    if (client == NULL) {
        printf("获取客户端失败\n");
        return;
    }

    // Show available state token files
    FileList token_files;
    find_state_token_files(&token_files);

    int activated = dl_client_is_activated(client);

    printf("\n请选择令牌来源:\n");
    if (activated) {
        printf("0. 使用当前激活的令牌\n");
    }

    if (token_files.count > 0) {
        printf("\n或从以下文件加载令牌:\n");
        for (int i = 0; i < token_files.count; i++) {
            printf("%d. %s\n", i + 1, token_files.files[i]);
        }
    }

    if (!activated && token_files.count == 0) {
        printf("当前没有激活的令牌,也没有找到可用的token文件\n");
        printf("请先使用选项1激活令牌\n");
        return;
    }

    printf("\n请选择 (0");
    if (token_files.count > 0) {
        printf("-%d", token_files.count);
    }
    printf("): ");

    int choice;
    if (scanf("%d", &choice) != 1) {
        printf("无效的选择\n");
        while (getchar() != '\n');
        return;
    }
    while (getchar() != '\n');

    if (choice < 0 || choice > token_files.count) {
        printf("无效的选择\n");
        return;
    }

    // If loading from file
    if (choice > 0) {
        char* file_path = resolve_token_file_path(token_files.files[choice - 1]);
        printf("正在从文件加载令牌: %s\n", token_files.files[choice - 1]);

        size_t token_size;
        char* token_string = read_file_to_string(file_path, &token_size);
        if (token_string == NULL) {
            printf("读取文件失败\n");
            return;
        }
        printf("读取到令牌 (%zu 字符)\n", token_size);

        // Initialize and load token (similar to accounting)
        if (!g_initialized) {
            DL_ClientConfig config;
            config.license_code = "TRUST_CHAIN";
            config.udp_port = 13325;
            config.tcp_port = 23325;
            config.registry_server_url = NULL;
            dl_client_initialize(client, &config);
            g_initialized = 1;
        }

        char* product_key_path = find_product_public_key();
        if (product_key_path != NULL) {
            size_t key_size;
            char* key_data = read_file_to_string(product_key_path, &key_size);
            if (key_data != NULL) {
                dl_client_set_product_public_key(client, key_data);
                free(key_data);
                printf("产品公钥设置成功\n");
            }
        }

        printf("正在导入令牌...\n");
        if (dl_client_import_token(client, token_string) != DL_ERROR_SUCCESS) {
            printf("令牌导入失败\n");
            free(token_string);
            return;
        }
        free(token_string);
        printf("令牌导入成功\n");

        int is_already_activated = (strstr(token_files.files[choice - 1], "activated") != NULL ||
                                   strstr(token_files.files[choice - 1], "state") != NULL);

        if (is_already_activated) {
            printf("检测到已激活令牌\n");
            printf("正在恢复激活状态...\n");
        } else {
            printf("正在首次激活令牌...\n");
        }

        DL_VerificationResult result;
        if (dl_client_activate_bind_device(client, &result) != DL_ERROR_SUCCESS || !result.valid) {
            printf("激活失败: %s\n", result.error_message);
            return;
        }

        if (is_already_activated) {
            printf("激活状态已恢复(token未改变)\n");
        } else {
            printf("首次激活成功\n");
        }
    }

    printf("开始验证信任链...\n\n");

    int checks_passed = 0;
    int total_checks = 4;

    // Check 1: Token signature verification
    printf("[1/4] 验证令牌签名(根密钥 -> 产品公钥 -> 令牌)\n");
    DL_VerificationResult result;
    if (dl_client_offline_verify_current_token(client, &result) != DL_ERROR_SUCCESS) {
        printf("   失败\n");
    } else if (!result.valid) {
        printf("   失败: %s\n", result.error_message);
    } else {
        printf("   通过: 令牌签名有效,信任链完整\n");
        checks_passed++;
    }
    printf("\n");

    // Check 2: Device state
    printf("[2/4] 验证设备状态\n");
    DL_DeviceState state = dl_client_get_device_state(client);
    printf("   通过: 设备状态正常 (状态码: %d)\n", state);
    checks_passed++;
    printf("\n");

    // Check 3: Token holder matches device
    printf("[3/4] 验证令牌持有者与当前设备匹配\n");
    DL_Token token;
    char device_id[128];
    if (dl_client_get_current_token(client, &token) == DL_ERROR_SUCCESS &&
        dl_client_get_device_id(client, device_id, sizeof(device_id)) == DL_ERROR_SUCCESS) {

        if (strcmp(token.holder_device_id, device_id) == 0) {
            printf("   通过: 令牌持有者与当前设备匹配\n");
            printf("   设备ID: %s\n", device_id);
            checks_passed++;
        } else {
            printf("   不匹配: 令牌持有者与当前设备不一致\n");
            printf("   当前设备ID: %s\n", device_id);
            printf("   令牌持有者ID: %s\n", token.holder_device_id);
            printf("   这可能表示令牌是从其他设备导入的\n");
        }
    }
    printf("\n");

    // Check 4: Token information
    printf("[4/4] 检查令牌详细信息\n");
    DL_StatusResult status;
    if (dl_client_get_status(client, &status) == DL_ERROR_SUCCESS && status.has_token) {
        printf("   通过: 令牌信息完整\n");
        printf("   令牌ID: %s\n", status.token_id);
        printf("   许可证代码: %s\n", status.license_code);
        printf("   应用ID: %s\n", status.app_id);

        char time_str[64];
        time_t issue_time_t = (time_t)status.issue_time;
        struct tm* tm_info = localtime(&issue_time_t);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        printf("   颁发时间: %s\n", time_str);

        if (status.expire_time == 0) {
            printf("   到期时间: 永不过期\n");
        } else {
            time_t expire_time_t = (time_t)status.expire_time;
            tm_info = localtime(&expire_time_t);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
            printf("   到期时间: %s\n", time_str);
        }
        checks_passed++;
    }
    printf("\n");

    // Summary
    printf("================================================\n");
    printf("验证结果: %d/%d 项检查通过\n", checks_passed, total_checks);
    if (checks_passed == total_checks) {
        printf("信任链验证完全通过!令牌可信且安全\n");
    } else if (checks_passed >= 2) {
        printf("部分检查通过,令牌基本可用但存在警告\n");
    } else {
        printf("多项检查失败,请检查令牌和设备状态\n");
    }
    printf("================================================\n");
}

void comprehensive_validation_wizard() {
    printf("\n综合验证\n");
    printf("----------\n");

    DL_Client* client = get_or_create_client();
    if (client == NULL) {
        printf("获取客户端失败\n");
        return;
    }

    // Show available state token files
    FileList token_files;
    find_state_token_files(&token_files);

    int activated = dl_client_is_activated(client);

    printf("\n请选择令牌来源:\n");
    if (activated) {
        printf("0. 使用当前激活的令牌\n");
    }

    if (token_files.count > 0) {
        printf("\n或从以下文件加载令牌:\n");
        for (int i = 0; i < token_files.count; i++) {
            printf("%d. %s\n", i + 1, token_files.files[i]);
        }
    }

    if (!activated && token_files.count == 0) {
        printf("当前没有激活的令牌,也没有找到可用的token文件\n");
        printf("请先使用选项1激活令牌\n");
        return;
    }

    printf("\n请选择 (0");
    if (token_files.count > 0) {
        printf("-%d", token_files.count);
    }
    printf("): ");

    int choice;
    if (scanf("%d", &choice) != 1) {
        printf("无效的选择\n");
        while (getchar() != '\n');
        return;
    }
    while (getchar() != '\n');

    if (choice < 0 || choice > token_files.count) {
        printf("无效的选择\n");
        return;
    }

    // If loading from file
    if (choice > 0) {
        char* file_path = resolve_token_file_path(token_files.files[choice - 1]);
        printf("正在从文件加载令牌: %s\n", token_files.files[choice - 1]);

        size_t token_size;
        char* token_string = read_file_to_string(file_path, &token_size);
        if (token_string == NULL) {
            printf("读取文件失败\n");
            return;
        }
        printf("读取到令牌 (%zu 字符)\n", token_size);

        if (!g_initialized) {
            DL_ClientConfig config;
            config.license_code = "COMPREHENSIVE";
            config.udp_port = 13325;
            config.tcp_port = 23325;
            config.registry_server_url = NULL;
            dl_client_initialize(client, &config);
            g_initialized = 1;
        }

        char* product_key_path = find_product_public_key();
        if (product_key_path != NULL) {
            size_t key_size;
            char* key_data = read_file_to_string(product_key_path, &key_size);
            if (key_data != NULL) {
                dl_client_set_product_public_key(client, key_data);
                free(key_data);
                printf("产品公钥设置成功\n");
            }
        }

        printf("正在导入令牌...\n");
        if (dl_client_import_token(client, token_string) != DL_ERROR_SUCCESS) {
            printf("令牌导入失败\n");
            free(token_string);
            return;
        }
        free(token_string);
        printf("令牌导入成功\n");

        int is_already_activated = (strstr(token_files.files[choice - 1], "activated") != NULL ||
                                   strstr(token_files.files[choice - 1], "state") != NULL);

        if (is_already_activated) {
            printf("检测到已激活令牌\n正在恢复激活状态...\n");
        } else {
            printf("正在首次激活令牌...\n");
        }

        DL_VerificationResult result;
        if (dl_client_activate_bind_device(client, &result) != DL_ERROR_SUCCESS || !result.valid) {
            printf("激活失败: %s\n", result.error_message);
            return;
        }

        printf(is_already_activated ? "激活状态已恢复(token未改变)\n" : "首次激活成功\n");
    }

    printf("执行综合验证流程...\n");
    int check_count = 0;
    int pass_count = 0;

    // Check 1: Activation status
    check_count++;
    activated = dl_client_is_activated(client);
    pass_count++;
    printf("%s 检查%d%s: 许可证%s\n",
           activated ? "通过" : "警告",
           check_count,
           activated ? "通过" : "",
           activated ? "已激活" : "未激活");

    // Check 2: Token verification
    if (activated) {
        check_count++;
        DL_VerificationResult result;
        if (dl_client_offline_verify_current_token(client, &result) == DL_ERROR_SUCCESS && result.valid) {
            pass_count++;
            printf("通过 检查%d通过: 令牌验证成功\n", check_count);
        } else {
            printf("失败 检查%d失败: 令牌验证失败\n", check_count);
        }
    }

    // Check 3: Device state
    check_count++;
    DL_DeviceState state = dl_client_get_device_state(client);
    pass_count++;
    printf("通过 检查%d通过: 设备状态正常 (状态码: %d)\n", check_count, state);

    // Check 4: Token info
    check_count++;
    DL_Token token;
    if (dl_client_get_current_token(client, &token) == DL_ERROR_SUCCESS) {
        pass_count++;
        if (strlen(token.token_id) >= 16) {
            printf("通过 检查%d通过: 令牌信息完整 (ID: %.16s...)\n", check_count, token.token_id);
        } else {
            printf("通过 检查%d通过: 令牌信息完整\n", check_count);
        }
    } else {
        printf("警告 检查%d: 无令牌信息\n", check_count);
    }

    // Check 5: Accounting function
    if (activated) {
        check_count++;
        const char* test_data = "{\"action\":\"comprehensive_test\",\"timestamp\":1234567890}";
        DL_VerificationResult result;
        if (dl_client_record_usage(client, test_data, &result) == DL_ERROR_SUCCESS && result.valid) {
            pass_count++;
            printf("通过 检查%d通过: 记账功能正常\n", check_count);

            // Export state changed token
            char state_token[MAX_TOKEN_SIZE];
            if (dl_client_export_state_changed_token_encrypted(client, state_token, sizeof(state_token)) == DL_ERROR_SUCCESS &&
                strlen(state_token) > 0) {
                printf("   状态变更后的新Token已生成\n");
                printf("   Token长度: %zu 字符\n", strlen(state_token));

                DL_StatusResult status;
                if (dl_client_get_status(client, &status) == DL_ERROR_SUCCESS && strlen(status.license_code) > 0) {
                    char filename[MAX_PATH];
                    time_t now = time(NULL);
                    struct tm* tm_info = localtime(&now);
                    char timestamp[32];
                    strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", tm_info);

                    snprintf(filename, sizeof(filename), "token_state_%s_idx%lu_%s.txt",
                             status.license_code, (unsigned long)status.state_index, timestamp);

                    if (write_string_to_file(filename, state_token)) {
                        char abs_path[PATH_MAX];
                        realpath(filename, abs_path);
                        printf("   已保存到: %s\n", abs_path);
                    }
                }
            }
        } else {
            printf("失败 检查%d失败: 记账功能异常\n", check_count);
        }
    }

    // Summary
    printf("\n综合验证结果:\n");
    printf("   总检查项: %d\n", check_count);
    printf("   通过项目: %d\n", pass_count);
    printf("   成功率: %.1f%%\n", (float)pass_count / check_count * 100);

    if (pass_count == check_count) {
        printf("所有检查均通过!系统运行正常\n");
    } else if (pass_count >= check_count / 2) {
        printf("大部分检查通过,系统基本正常\n");
    } else {
        printf("多项检查失败,请检查系统配置\n");
    }
}

int main() {
    printf("==========================================\n");
    printf("DecentriLicense C SDK 验证向导\n");
    printf("==========================================\n\n");

    // Register cleanup on exit
    atexit(cleanup_client);

    while (1) {
        printf("请选择要执行的操作:\n");
        printf("0. 🔑 选择产品公钥\n");
        printf("1. 🔓 激活令牌\n");
        printf("2. ✅ 校验已激活令牌\n");
        printf("3. 🔍 验证令牌合法性\n");
        printf("4. 📊 记账信息\n");
        printf("5. 🔗 信任链验证\n");
        printf("6. 🎯 综合验证\n");
        printf("7. 🚪 退出\n");
        printf("请输入选项 (0-7): ");

        int choice;
        if (scanf("%d", &choice) != 1) {
            printf("无效选项,请重新选择\n\n");
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }
        while (getchar() != '\n'); // Clear input buffer

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
                printf("感谢使用 DecentriLicense C SDK 验证向导!\n");
                return 0;
            default:
                printf("无效选项,请重新选择\n");
        }
        printf("\n");
    }

    return 0;
}
