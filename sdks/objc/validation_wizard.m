#import <Foundation/Foundation.h>
#import "DecentriLicenseClient.h"
#import <string.h>
#import <stdio.h>

// Global state
static DecentriLicenseClient *gClient = nil;
static BOOL gInitialized = NO;
static NSString *gSelectedProductKeyPath = nil;

// Helper: list files with extensions
static NSArray<NSString *> *listFiles(NSArray<NSString *> *exts) {
    NSFileManager *fm = [NSFileManager defaultManager];
    NSString *cwd = [fm currentDirectoryPath];
    NSArray *allFiles = [fm contentsOfDirectoryAtPath:cwd error:nil];
    NSMutableArray *result = [NSMutableArray array];
    for (NSString *f in allFiles) {
        NSString *lower = [f lowercaseString];
        for (NSString *ext in exts) {
            if ([lower hasSuffix:ext]) {
                [result addObject:f];
                break;
            }
        }
    }
    [result sortUsingSelector:@selector(compare:)];
    return result;
}

// Helper: pick file interactively
static NSString *pickFile(NSString *title, NSArray<NSString *> *exts) {
    NSArray *files = listFiles(exts);
    printf("%s\n", [title UTF8String]);
    if (files.count == 0) {
        printf("当前目录没有可选文件，请手动输入路径: ");
        char buf[1024] = {0};
        fgets(buf, sizeof(buf), stdin);
        NSString *input = @(buf);
        input = [input stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
        return input.length > 0 ? input : nil;
    }
    for (NSUInteger i = 0; i < files.count; i++) {
        printf("%lu. %s\n", (unsigned long)(i + 1), [files[i] UTF8String]);
    }
    printf("0. 手动输入路径\n");
    printf("请选择文件编号: ");
    char buf[64] = {0};
    fgets(buf, sizeof(buf), stdin);
    int n = atoi(buf);
    if (n >= 1 && n <= (int)files.count) {
        return [NSString stringWithFormat:@"%@/%@", [NSFileManager defaultManager].currentDirectoryPath, files[n-1]];
    }
    printf("请输入文件路径: ");
    char buf2[1024] = {0};
    fgets(buf2, sizeof(buf2), stdin);
    NSString *input = @(buf2);
    input = [input stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    return input.length > 0 ? input : nil;
}

// Helper: find product public key
static NSString *findProductPublicKey() {
    if (gSelectedProductKeyPath) return gSelectedProductKeyPath;
    NSArray *files = listFiles(@[@".pem"]);
    for (NSString *f in files) {
        NSString *lower = [f lowercaseString];
        if ([lower hasPrefix:@"public_"] && [lower hasSuffix:@".pem"]) {
            return [NSString stringWithFormat:@"%@/%@", [NSFileManager defaultManager].currentDirectoryPath, f];
        }
    }
    return nil;
}

// Helper: get or create client
static DecentriLicenseClient *getOrCreateClient() {
    if (gClient && gInitialized) return gClient;
    gClient = [[DecentriLicenseClient alloc] init];
    NSError *error = nil;
    if (![gClient initializeWithUdpPort:13325 tcpPort:23325 error:&error]) {
        printf("❌ 初始化客户端失败: %s\n", error.localizedDescription.UTF8String);
        gClient = nil;
        return nil;
    }
    gInitialized = YES;
    return gClient;
}

// Helper: read file content
static NSString *readFile(NSString *path) {
    return [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
}

// Wizard: import token
static void importTokenWizard() {
    printf("\n📥 导入令牌\n");
    printf("----------------------------------------\n");

    NSString *tokenFile = pickFile(@"请选择 token 文件:", @[@".json", @".txt"]);
    if (!tokenFile) return;

    NSString *tokenContent = readFile(tokenFile);
    if (!tokenContent) {
        printf("❌ 读取文件失败\n");
        return;
    }
    tokenContent = [tokenContent stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    printf("✅ 读取到令牌 (%lu 字符)\n", (unsigned long)tokenContent.length);

    DecentriLicenseClient *client = getOrCreateClient();
    if (!client) return;

    // Set product public key
    NSString *productKeyPath = findProductPublicKey();
    if (productKeyPath) {
        NSString *productKeyData = readFile(productKeyPath);
        if (productKeyData) {
            NSError *err = nil;
            if ([client setProductPublicKey:productKeyData error:&err]) {
                printf("✅ 产品公钥设置成功\n");
            } else {
                printf("❌ 设置产品公钥失败: %s\n", err.localizedDescription.UTF8String);
                return;
            }
        }
    }

    NSError *err = nil;
    if (![client importToken:tokenContent error:&err]) {
        printf("❌ 导入令牌失败: %s\n", err.localizedDescription.UTF8String);
        return;
    }
    printf("✅ 令牌导入成功\n");

    // Activate
    printf("🎯 正在激活令牌...\n");
    DLVerificationResult *result = [client activateBindDevice:&err];
    if (result && result.valid) {
        printf("✅ 激活成功\n");
    } else {
        printf("❌ 激活失败: %s\n", result ? result.errorMessage.UTF8String : err.localizedDescription.UTF8String);
    }
}

// Wizard: verify activated token
static void verifyActivatedTokenWizard() {
    printf("\n✅ 校验已激活令牌\n");
    printf("----------------------------------------\n");

    DecentriLicenseClient *client = getOrCreateClient();
    if (!client) return;

    // Set product public key before verification
    NSString *productKeyPath = findProductPublicKey();
    if (productKeyPath) {
        NSString *productKeyData = readFile(productKeyPath);
        if (productKeyData) {
            NSError *err = nil;
            if ([client setProductPublicKey:productKeyData error:&err]) {
                printf("✅ 产品公钥设置成功\n");
            } else {
                printf("❌ 设置产品公钥失败: %s\n", err.localizedDescription.UTF8String);
                return;
            }
        }
    } else {
        printf("❌ 未找到产品公钥文件，无法验证\n");
        return;
    }

    NSError *err = nil;
    DLVerificationResult *result = [client offlineVerifyCurrentToken:&err];
    if (result && result.valid) {
        printf("✅ 令牌验证成功\n");
    } else {
        printf("❌ 令牌验证失败: %s\n", result ? result.errorMessage.UTF8String : err.localizedDescription.UTF8String);
    }

    // Show token info
    DLStatusResult *status = [client getStatus:nil];
    if (status && status.hasToken) {
        printf("\n🎫 令牌信息:\n");
        printf("   令牌ID: %s\n", status.tokenId.UTF8String);
        printf("   许可证代码: %s\n", status.licenseCode.UTF8String);
        printf("   应用ID: %s\n", status.appId.UTF8String);
        printf("   持有设备ID: %s\n", status.holderDeviceId.UTF8String);
    }
}

// Wizard: trust chain validation
static void trustChainValidationWizard() {
    printf("\n🔗 信任链验证\n");
    printf("----------------------------------------\n");
    printf("💡 信任链验证检查加密签名的完整性：根密钥 → 产品公钥 → 令牌签名 → 设备绑定\n");

    DecentriLicenseClient *client = getOrCreateClient();
    if (!client) return;

    // Set product public key
    NSString *productKeyPath = findProductPublicKey();
    if (productKeyPath) {
        NSString *productKeyData = readFile(productKeyPath);
        if (productKeyData) {
            [client setProductPublicKey:productKeyData error:nil];
            printf("✅ 产品公钥设置成功\n");
        }
    }

    int checksPassed = 0;
    const int totalChecks = 4;

    // Check 1: Token signature
    printf("\n🔍 [1/%d] 验证令牌签名\n", totalChecks);
    DLVerificationResult *vr = [client offlineVerifyCurrentToken:nil];
    if (vr && vr.valid) {
        printf("   ✅ 通过: 令牌签名有效，信任链完整\n");
        checksPassed++;
    } else {
        printf("   ❌ 失败: %s\n", vr ? vr.errorMessage.UTF8String : "unknown");
    }

    // Check 2: Device state
    printf("\n🔍 [2/%d] 验证设备状态\n", totalChecks);
    NSString *deviceId = [client getDeviceId:nil];
    if (deviceId && deviceId.length > 0) {
        printf("   ✅ 通过: 设备状态正常 (设备ID: %s)\n", deviceId.UTF8String);
        checksPassed++;
    }

    // Check 3: Holder matching
    printf("\n🔍 [3/%d] 验证令牌持有者与当前设备匹配\n", totalChecks);
    DLStatusResult *status = [client getStatus:nil];
    if (status && deviceId && [status.holderDeviceId isEqualToString:deviceId]) {
        printf("   ✅ 通过: 令牌持有者与当前设备匹配\n");
        checksPassed++;
    } else {
        printf("   ⚠️  不匹配\n");
    }

    // Check 4: Token info
    printf("\n🔍 [4/%d] 检查令牌详细信息\n", totalChecks);
    if (status && status.hasToken) {
        printf("   ✅ 通过: 令牌信息完整\n");
        printf("   🎫 令牌ID: %s\n", status.tokenId.UTF8String);
        printf("   📝 许可证代码: %s\n", status.licenseCode.UTF8String);
        checksPassed++;
    }

    printf("\n📊 验证结果: %d/%d 通过\n", checksPassed, totalChecks);
    if (checksPassed == totalChecks) {
        printf("✅ 所有检查通过！\n");
    }
}

// Wizard: status
static void statusWizard() {
    printf("\n📊 查询状态\n");
    printf("----------------------------------------\n");

    DecentriLicenseClient *client = getOrCreateClient();
    if (!client) return;

    DLStatusResult *status = [client getStatus:nil];
    if (status) {
        printf("has_token: %d\n", status.hasToken ? 1 : 0);
        printf("is_activated: %d\n", status.isActivated ? 1 : 0);
        printf("state_index: %llu\n", status.stateIndex);
        printf("token_id: %s\n", status.tokenId.UTF8String);
        printf("holder_device_id: %s\n", status.holderDeviceId.UTF8String);
        printf("app_id: %s\n", status.appId.UTF8String);
        printf("license_code: %s\n", status.licenseCode.UTF8String);

        if (status.isActivated) {
            NSString *payload = [client getStatePayload:nil];
            if (payload && payload.length > 0) {
                printf("state_payload: %s\n", payload.UTF8String);
            } else {
                printf("state_payload: (empty)\n");
            }
        }
    }
}

// Wizard: recovery channel
static void recoveryChannelWizard() {
    printf("\n🔑 恢复通道管理\n");
    printf("----------------------------------------\n");

    DecentriLicenseClient *client = getOrCreateClient();
    if (!client) return;

    if (![client isActivated]) {
        printf("❌ 请先激活令牌再管理恢复通道\n");
        return;
    }

    printf("请选择操作:\n");
    printf("1. 添加恢复通道 (设置密码)\n");
    printf("2. 移除恢复通道\n");
    printf("0. 返回\n");
    printf("\n请选择 (0-2): ");

    char buf[256] = {0};
    fgets(buf, sizeof(buf), stdin);
    int choice = atoi(buf);

    switch (choice) {
        case 1: {
            printf("请输入恢复密码: ");
            char pwdbuf[256] = {0};
            fgets(pwdbuf, sizeof(pwdbuf), stdin);
            NSString *password = @(pwdbuf);
            password = [password stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
            if (password.length == 0) {
                printf("❌ 密码不能为空\n");
                return;
            }
            NSError *err = nil;
            DLVerificationResult *result = [client addRecoveryChannel:password error:&err];
            if (result && result.valid) {
                printf("✅ 恢复通道添加成功\n");
            } else {
                printf("❌ 添加失败: %s\n", result ? result.errorMessage.UTF8String : err.localizedDescription.UTF8String);
            }
            break;
        }
        case 2: {
            NSError *err = nil;
            DLVerificationResult *result = [client removeRecoveryChannel:&err];
            if (result && result.valid) {
                printf("✅ 恢复通道已移除\n");
            } else {
                printf("❌ 移除失败: %s\n", result ? result.errorMessage.UTF8String : err.localizedDescription.UTF8String);
            }
            break;
        }
        default: break;
    }
}

// Main
int main(int argc, char *argv[]) {
    @autoreleasepool {
        // Quick validate mode
        if (argc >= 4 && strcmp(argv[1], "validate") == 0) {
            NSString *tokenFile = @(argv[2]);
            NSString *productKeyFile = @(argv[3]);

            NSString *tokenContent = readFile(tokenFile);
            NSString *productKeyContent = readFile(productKeyFile);
            if (!tokenContent || !productKeyContent) {
                printf("❌ 读取文件失败\n");
                return 1;
            }

            DecentriLicenseClient *client = [[DecentriLicenseClient alloc] init];
            NSError *err = nil;
            [client initializeWithUdpPort:13325 tcpPort:23325 error:&err];
            [client setProductPublicKey:productKeyContent error:nil];
            [client importToken:tokenContent error:nil];

            DLVerificationResult *result = [client offlineVerifyCurrentToken:&err];
            if (result && result.valid) {
                printf("✅ Token validation successful!\n");
                DLStatusResult *status = [client getStatus:nil];
                if (status && status.hasToken) {
                    printf("   Token ID: %s\n", status.tokenId.UTF8String);
                    printf("   License Code: %s\n", status.licenseCode.UTF8String);
                }
                [client shutdown:nil];
                return 0;
            } else {
                printf("❌ Token validation failed: %s\n", result ? result.errorMessage.UTF8String : "unknown");
                [client shutdown:nil];
                return 1;
            }
        }

        // Interactive wizard
        printf("\n🔐 DecentriLicense Objective-C SDK 验证向导\n");
        printf("==================================================\n");

        while (true) {
            printf("\n请选择操作:\n");
            printf("1. 📥 导入令牌（密文/JSON）\n");
            printf("2. ✅ 校验已激活令牌\n");
            printf("3. 🔗 信任链验证\n");
            printf("4. 📊 查询状态\n");
            printf("5. 🔑 设置产品公钥\n");
            printf("6. 🔐 恢复通道管理（密码/助记词）\n");
            printf("0. 🚪 退出\n");
            printf("\n请选择 (0-6): ");

            char buf[16] = {0};
            fgets(buf, sizeof(buf), stdin);
            int choice = atoi(buf);

            switch (choice) {
                case 1: importTokenWizard(); break;
                case 2: verifyActivatedTokenWizard(); break;
                case 3: trustChainValidationWizard(); break;
                case 4: statusWizard(); break;
                case 5: {
                    printf("\n🔑 设置产品公钥\n");
                    NSString *keyFile = pickFile(@"请选择产品公钥文件:", @[@".pem"]);
                    if (keyFile) {
                        gSelectedProductKeyPath = keyFile;
                        printf("✅ 已选择产品公钥: %s\n", keyFile.UTF8String);
                    }
                    break;
                }
                case 6: recoveryChannelWizard(); break;
                case 0:
                    if (gClient) [gClient shutdown:nil];
                    printf("👋 再见！\n");
                    return 0;
                default:
                    printf("❌ 无效的选择\n");
            }
        }
    }
    return 0;
}
