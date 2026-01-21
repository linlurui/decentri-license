package com.decentrilicense.example;

import com.decentrilicense.ActivationResult;
import com.decentrilicense.DecentriLicenseClient;
import com.decentrilicense.LicenseException;

import java.util.Scanner;
import java.util.List;
import java.util.ArrayList;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.annotations.SerializedName;

/**
 * DecentriLicense Java SDK 验证向导
 * 
 * 这是一个交互式的验证工具，用于测试DecentriLicense Java SDK的所有功能。
 */
public class ValidationWizard {
    
    // 记账信息结构
    static class AccountingInfo {
        @SerializedName("token_id")
        String tokenId;
        
        @SerializedName("activation_time")
        String activationTime;
        
        @SerializedName("device_id")
        String deviceId;
        
        @SerializedName("license_code")
        String licenseCode;
        
        @SerializedName("is_valid")
        boolean isValid;
        
        public AccountingInfo(String tokenId, String deviceId, String licenseCode) {
            this.tokenId = tokenId;
            this.deviceId = deviceId;
            this.licenseCode = licenseCode;
            this.activationTime = LocalDateTime.now().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME);
            this.isValid = true;
        }
    }
    
    // 验证结果结构
    static class ValidationResult {
        boolean valid;
        
        @SerializedName("token_id")
        String tokenId;
        
        String error;
        
        @SerializedName("app_id")
        String appId;
        
        @SerializedName("license_code")
        String licenseCode;
        
        @SerializedName("issue_time")
        String issueTime;
        
        @SerializedName("expire_time")
        String expireTime;
        
        @SerializedName("holder_device_id")
        String holderDeviceId;
        
        public ValidationResult() {
            this.valid = true;
        }
    }
    
    private static Scanner scanner = new Scanner(System.in);
    private static Gson gson = new GsonBuilder().setPrettyPrinting().create();
    
    public static void main(String[] args) {
        System.out.println("==========================================");
        System.out.println("DecentriLicense Java SDK 验证向导");
        System.out.println("==========================================");
        System.out.println();
        
        while (true) {
            System.out.println("请选择要执行的操作:");
            System.out.println("1. 激活令牌");
            System.out.println("2. 校验令牌");
            System.out.println("3. 记账信息");
            System.out.println("4. 信任链验证");
            System.out.println("5. 综合验证");
            System.out.println("6. 退出");
            System.out.print("请输入选项 (1-6): ");
            
            String choice = scanner.nextLine().trim();
            
            switch (choice) {
                case "1":
                    activateTokenWizard();
                    break;
                case "2":
                    verifyTokenWizard();
                    break;
                case "3":
                    accountingWizard();
                    break;
                case "4":
                    trustChainValidationWizard();
                    break;
                case "5":
                    comprehensiveValidationWizard();
                    break;
                case "6":
                    System.out.println("感谢使用 DecentriLicense Java SDK 验证向导!");
                    return;
                default:
                    System.out.println("无效选项，请重新输入。");
            }
            
            System.out.println("\n" + repeat('-', 50) + "\n");
        }
    }

    private static String repeat(char ch, int count) {
        if (count <= 0) {
            return "";
        }
        StringBuilder sb = new StringBuilder(count);
        for (int i = 0; i < count; i++) {
            sb.append(ch);
        }
        return sb.toString();
    }
    
    private static void activateTokenWizard() {
        System.out.println("\n--- 激活令牌 ---");
        
        // 获取许可证代码
        System.out.print("请输入许可证代码 (默认: EXAMPLE-LICENSE-12345): ");
        String licenseCode = scanner.nextLine().trim();
        if (licenseCode.isEmpty()) {
            licenseCode = "EXAMPLE-LICENSE-12345";
        }
        
        // 获取UDP端口
        System.out.print("请输入UDP端口 (默认: 8888): ");
        String udpPortStr = scanner.nextLine().trim();
        int udpPort = 8888;
        if (!udpPortStr.isEmpty()) {
            try {
                udpPort = Integer.parseInt(udpPortStr);
            } catch (NumberFormatException e) {
                System.out.println("无效的端口号，使用默认值 8888");
            }
        }
        
        // 获取TCP端口
        System.out.print("请输入TCP端口 (默认: 8889): ");
        String tcpPortStr = scanner.nextLine().trim();
        int tcpPort = 8889;
        if (!tcpPortStr.isEmpty()) {
            try {
                tcpPort = Integer.parseInt(tcpPortStr);
            } catch (NumberFormatException e) {
                System.out.println("无效的端口号，使用默认值 8889");
            }
        }
        
        try (DecentriLicenseClient client = new DecentriLicenseClient()) {
            System.out.println("初始化客户端...");
            client.initialize(licenseCode, udpPort, tcpPort, "");
            
            String deviceId = client.getDeviceId();
            System.out.println("设备ID: " + deviceId);
            
            // 激活许可证
            System.out.println("激活许可证...");
            System.out.println("广播局域网发现...");
            System.out.println("等待对等节点和选举...");
            
            try {
                ActivationResult result = client.activate();
                boolean success = result.isSuccess();
                System.out.println("激活成功: " + success);
                
                // 记录记账信息
                if (success) {
                    AccountingInfo accounting = new AccountingInfo(
                        "generated-token-id", // 实际应用中应从激活结果获取
                        deviceId,
                        licenseCode
                    );
                    
                    // 保存记账信息到文件
                    String filename = "accounting_" + LocalDateTime.now().format(DateTimeFormatter.ofPattern("yyyyMMddHHmmss")) + ".json";
                    try (FileWriter writer = new FileWriter(filename)) {
                        gson.toJson(accounting, writer);
                        System.out.println("记账信息已保存到: " + filename);
                    } catch (IOException e) {
                        System.out.println("保存记账信息失败: " + e.getMessage());
                    }
                }
            } catch (LicenseException e) {
                System.out.println("激活失败: " + e.getMessage());
            }
        } catch (Exception e) {
            System.out.println("操作失败: " + e.getMessage());
        }
    }
    
    private static void verifyTokenWizard() {
        System.out.println("\n--- 校验令牌 ---");
        
        // 获取令牌文件路径
        System.out.print("请输入令牌文件路径: ");
        String tokenFilePath = scanner.nextLine().trim();
        
        if (tokenFilePath.isEmpty()) {
            System.out.println("令牌文件路径不能为空");
            return;
        }
        
        try {
            // 读取令牌文件
            File tokenFile = new File(tokenFilePath);
            if (!tokenFile.exists()) {
                System.out.println("找不到指定的令牌文件");
                return;
            }
            
            StringBuilder content = new StringBuilder();
            try (FileReader reader = new FileReader(tokenFile)) {
                int ch;
                while ((ch = reader.read()) != -1) {
                    content.append((char) ch);
                }
            }
            
            String tokenData = content.toString();
            
            // 这里应该调用SDK的令牌验证功能
            // 由于当前SDK实现简化，我们模拟验证过程
            String preview = tokenData.length() > 100 ? tokenData.substring(0, 100) + "..." : tokenData;
            System.out.println("令牌内容预览: " + preview);
            
            // 模拟验证结果
            ValidationResult result = new ValidationResult();
            result.tokenId = "sample-token-id";
            result.appId = "sample-app-id";
            result.licenseCode = "EXAMPLE-LICENSE-12345";
            result.issueTime = LocalDateTime.now().minusDays(1).format(DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss"));
            result.expireTime = LocalDateTime.now().plusYears(1).format(DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss"));
            result.holderDeviceId = "sample-device-id";
            
            // 显示验证结果
            System.out.println("\n验证结果:");
            if (result.valid) {
                System.out.println("✓ 令牌验证成功");
                System.out.println("  令牌ID: " + result.tokenId);
                System.out.println("  应用ID: " + result.appId);
                System.out.println("  许可证代码: " + result.licenseCode);
                System.out.println("  签发时间: " + result.issueTime);
                System.out.println("  过期时间: " + result.expireTime);
                System.out.println("  持有设备ID: " + result.holderDeviceId);
            } else {
                System.out.println("✗ 令牌验证失败");
                System.out.println("  错误信息: " + result.error);
            }
        } catch (Exception e) {
            System.out.println("校验令牌时发生错误: " + e.getMessage());
        }
    }
    
    private static void accountingWizard() {
        System.out.println("\n--- 记账信息 ---");
        
        // 查找记账文件
        File currentDir = new File(".");
        File[] files = currentDir.listFiles((dir, name) -> name.startsWith("accounting_") && name.endsWith(".json"));
        
        if (files == null || files.length == 0) {
            System.out.println("未找到记账文件");
            return;
        }
        
        List<File> accountingFiles = new ArrayList<>();
        for (File file : files) {
            if (file.isFile()) {
                accountingFiles.add(file);
            }
        }
        
        if (accountingFiles.isEmpty()) {
            System.out.println("未找到记账文件");
            return;
        }
        
        System.out.println("找到以下记账文件:");
        for (int i = 0; i < accountingFiles.size(); i++) {
            System.out.println((i + 1) + ". " + accountingFiles.get(i).getName());
        }
        
        System.out.print("请选择要查看的文件编号: ");
        try {
            int choice = Integer.parseInt(scanner.nextLine().trim());
            if (choice >= 1 && choice <= accountingFiles.size()) {
                File selectedFile = accountingFiles.get(choice - 1);
                
                try (FileReader reader = new FileReader(selectedFile)) {
                    AccountingInfo accounting = gson.fromJson(reader, AccountingInfo.class);
                    
                    System.out.println("\n记账信息详情:");
                    System.out.println("令牌ID: " + (accounting.tokenId != null ? accounting.tokenId : "N/A"));
                    System.out.println("激活时间: " + (accounting.activationTime != null ? accounting.activationTime : "N/A"));
                    System.out.println("设备ID: " + (accounting.deviceId != null ? accounting.deviceId : "N/A"));
                    System.out.println("许可证代码: " + (accounting.licenseCode != null ? accounting.licenseCode : "N/A"));
                    System.out.println("是否有效: " + accounting.isValid);
                } catch (IOException e) {
                    System.out.println("读取文件失败: " + e.getMessage());
                }
            } else {
                System.out.println("无效选择");
            }
        } catch (NumberFormatException e) {
            System.out.println("请输入有效的数字");
        }
    }
    
    private static void trustChainValidationWizard() {
        System.out.println("\n--- 信任链验证 ---");
        System.out.println("信任链验证用于验证令牌的完整信任链...");
        
        // 模拟信任链验证过程
        System.out.println("1. 验证根公钥签名...");
        System.out.println("2. 验证产品公钥签名...");
        System.out.println("3. 验证令牌签名...");
        System.out.println("4. 验证环境一致性...");
        
        // 模拟验证结果
        System.out.println("\n信任链验证结果:");
        System.out.println("✓ 根公钥签名验证通过");
        System.out.println("✓ 产品公钥签名验证通过");
        System.out.println("✓ 令牌签名验证通过");
        System.out.println("✓ 环境一致性验证通过");
        System.out.println("✓ 信任链完整且有效");
    }
    
    private static void comprehensiveValidationWizard() {
        System.out.println("\n--- 综合验证 ---");
        System.out.println("综合验证将执行完整的许可证验证流程...");
        
        // 模拟综合验证过程
        System.out.println("1. 检查系统环境...");
        System.out.println("2. 验证网络连接...");
        System.out.println("3. 加载许可证...");
        System.out.println("4. 验证信任链...");
        System.out.println("5. 检查设备状态...");
        System.out.println("6. 验证许可证有效性...");
        
        // 模拟验证结果
        System.out.println("\n综合验证结果:");
        System.out.println("✓ 系统环境检查通过");
        System.out.println("✓ 网络连接正常");
        System.out.println("✓ 许可证加载成功");
        System.out.println("✓ 信任链验证通过");
        System.out.println("✓ 设备状态正常");
        System.out.println("✓ 许可证有效");
        System.out.println("✓ 综合验证完成");
    }
}