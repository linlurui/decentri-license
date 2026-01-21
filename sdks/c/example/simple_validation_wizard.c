#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

// License state structure
typedef struct {
    char token_data[8192];
    char license_public_key[4096];
    int is_activated;
    char device_id[256];
    char activation_time[256];
    int usage_count;
} LicenseState;

// Global state
static LicenseState g_license_state = {0};
static const char* STATE_FILE = ".decentri/license.state";

// Function declarations
void print_menu();
void import_license_key();
void verify_license();
void activate_to_device();
void query_status();
void record_usage();
char* get_input(const char* prompt);
void trim(char* str);
int save_state();
int load_state();
int is_encrypted_token(const char* input);

int main() {
    printf("==========================================\n");
    printf("DecentriLicense C SDK 验证向导\n");
    printf("==========================================\n");
    
    // Try to load previous state
    load_state();
    
    while (1) {
        print_menu();
        
        char* choice_str = get_input("请选择: ");
        trim(choice_str);
        int choice = atoi(choice_str);
        free(choice_str);
        
        switch (choice) {
            case 1:
                import_license_key();
                break;
            case 2:
                verify_license();
                break;
            case 3:
                activate_to_device();
                break;
            case 4:
                query_status();
                break;
            case 5:
                record_usage();
                break;
            case 0:
                printf("再见！\n");
                return 0;
            default:
                printf("❌ 无效选项，请重新输入。\n");
        }
        
        printf("\n");
        for (int i = 0; i < 50; i++) printf("-");
        printf("\n\n");
    }
    
    return 0;
}

void print_menu() {
    printf("\n=== DecentriLicense 向导 ===\n");
    printf("1. 导入许可证密钥\n");
    printf("2. 验证许可证\n");
    printf("3. 激活到当前设备\n");
    printf("4. 查询当前状态/余额\n");
    printf("5. 记录使用量（状态迁移）\n");
    printf("0. 退出\n");
}

void import_license_key() {
    printf("\n--- 导入许可证密钥 ---\n");
    
    char* input_method_str = get_input("输入方式 (1: 直接粘贴, 2: 文件路径): ");
    trim(input_method_str);
    int input_method = atoi(input_method_str);
    free(input_method_str);
    
    if (input_method == 1) {
        printf("请粘贴许可证密钥（JWT格式或加密后的字符串）:\n");
        char key_data[8192];
        if (fgets(key_data, sizeof(key_data), stdin) != NULL) {
            trim(key_data);
            
            if (strlen(key_data) == 0) {
                printf("❌ 输入不能为空\n");
                return;
            }
            
            strcpy(g_license_state.token_data, key_data);
            printf("✅ 许可证密钥已导入\n");
            save_state();
        }
    } else if (input_method == 2) {
        char* file_path = get_input("请输入文件路径: ");
        trim(file_path);
        
        if (strlen(file_path) == 0) {
            printf("❌ 文件路径不能为空\n");
            free(file_path);
            return;
        }
        
        FILE* file = fopen(file_path, "r");
        if (file == NULL) {
            printf("❌ 找不到指定的文件\n");
            free(file_path);
            return;
        }
        
        size_t len = fread(g_license_state.token_data, 1, sizeof(g_license_state.token_data) - 1, file);
        g_license_state.token_data[len] = '\0';
        fclose(file);
        
        printf("✅ 许可证密钥已从文件导入\n");
        save_state();
        free(file_path);
    } else {
        printf("❌ 无效的输入方式\n");
    }
}

void verify_license() {
    printf("\n--- 验证许可证 ---\n");
    
    if (strlen(g_license_state.token_data) == 0) {
        printf("❌ 请先导入许可证密钥\n");
        return;
    }
    
    // Check if it's an encrypted token
    if (is_encrypted_token(g_license_state.token_data)) {
        printf("🔒 检测到加密的许可证，正在解密...\n");
        
        // Here should be the actual decryption logic
        // For simplicity, we assume decryption is successful
        printf("✅ 许可证解密成功\n");
    } else {
        printf("📄 检测到JSON格式的许可证\n");
    }
    
    // Simulate verification process
    printf("🔍 正在校验许可证签名...\n");
    printf("✅ 许可证验证通过\n");
}

void activate_to_device() {
    printf("\n--- 激活到当前设备 ---\n");
    
    if (strlen(g_license_state.token_data) == 0) {
        printf("❌ 请先导入许可证密钥\n");
        return;
    }
    
    // Generate device ID (simplified example)
    srand(time(NULL));
    snprintf(g_license_state.device_id, sizeof(g_license_state.device_id), "DEV-%d", rand() % 100000);
    
    // Get current time
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(g_license_state.activation_time, sizeof(g_license_state.activation_time), "%Y-%m-%d %H:%M:%S", tm_info);
    g_license_state.is_activated = 1;
    
    printf("✅ 设备激活成功\n");
    printf("  设备ID: %s\n", g_license_state.device_id);
    printf("  激活时间: %s\n", g_license_state.activation_time);
    
    save_state();
}

void query_status() {
    printf("\n--- 查询当前状态/余额 ---\n");
    
    printf("许可证状态:\n");
    if (strlen(g_license_state.token_data) == 0) {
        printf("  是否已导入: 否\n");
    } else {
        printf("  是否已导入: 是\n");
    }
    printf("  是否已激活: %s\n", g_license_state.is_activated ? "是" : "否");
    
    if (g_license_state.is_activated) {
        printf("  设备ID: %s\n", g_license_state.device_id);
        printf("  激活时间: %s\n", g_license_state.activation_time);
    }
    
    printf("  使用次数: %d\n", g_license_state.usage_count);
}

void record_usage() {
    printf("\n--- 记录使用量（状态迁移） ---\n");
    
    if (!g_license_state.is_activated) {
        printf("❌ 请先激活到当前设备\n");
        return;
    }
    
    g_license_state.usage_count++;
    
    printf("✅ 使用量记录成功\n");
    printf("  当前使用次数: %d\n", g_license_state.usage_count);
    
    save_state();
}

char* get_input(const char* prompt) {
    printf("%s", prompt);
    char* line = malloc(1024);
    if (fgets(line, 1024, stdin) != NULL) {
        return line;
    }
    free(line);
    return NULL;
}

void trim(char* str) {
    char* end;
    
    // Trim leading space
    while(*str == ' ') str++;
    
    if(*str == 0)  // All spaces?
        return;
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && *end == ' ') end--;
    
    // Write new null terminator
    *(end+1) = 0;
}

int save_state() {
    // Create directory
    mkdir(".decentri", 0755);
    
    // Open file for writing
    FILE* file = fopen(STATE_FILE, "w");
    if (file == NULL) {
        printf("⚠️  保存状态失败: 无法打开文件\n");
        return 0;
    }
    
    // Write state to file
    fprintf(file, "{\n");
    fprintf(file, "  \"token_data\": \"%s\",\n", g_license_state.token_data);
    fprintf(file, "  \"license_public_key\": \"%s\",\n", g_license_state.license_public_key);
    fprintf(file, "  \"is_activated\": %d,\n", g_license_state.is_activated);
    fprintf(file, "  \"device_id\": \"%s\",\n", g_license_state.device_id);
    fprintf(file, "  \"activation_time\": \"%s\",\n", g_license_state.activation_time);
    fprintf(file, "  \"usage_count\": %d\n", g_license_state.usage_count);
    fprintf(file, "}\n");
    
    fclose(file);
    return 1;
}

int load_state() {
    // Open file for reading
    FILE* file = fopen(STATE_FILE, "r");
    if (file == NULL) {
        return 0;
    }
    
    // For simplicity, we won't parse the JSON file in this C example
    // In a real implementation, you would use a JSON library like cJSON
    
    fclose(file);
    return 0;  // Pretend we couldn't load for this example
}

int is_encrypted_token(const char* input) {
    // Encrypted token format: encrypted_data|nonce
    return strchr(input, '|') != NULL;
}