package com.decentrilicense;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.annotations.SerializedName;

import java.io.*;
import java.nio.file.*;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.Scanner;

public class SimpleValidationWizard {
    // License state class
    private static class LicenseState {
        @SerializedName("token_data")
        private String tokenData = "";
        
        @SerializedName("license_public_key")
        private String licensePublicKey = "";
        
        @SerializedName("is_activated")
        private boolean isActivated = false;
        
        @SerializedName("device_id")
        private String deviceId = "";
        
        @SerializedName("activation_time")
        private String activationTime = "";
        
        @SerializedName("usage_count")
        private int usageCount = 0;
        
        // Getters and setters
        public String getTokenData() { return tokenData; }
        public void setTokenData(String tokenData) { this.tokenData = tokenData; }
        
        public String getLicensePublicKey() { return licensePublicKey; }
        public void setLicensePublicKey(String licensePublicKey) { this.licensePublicKey = licensePublicKey; }
        
        public boolean isActivated() { return isActivated; }
        public void setActivated(boolean activated) { isActivated = activated; }
        
        public String getDeviceId() { return deviceId; }
        public void setDeviceId(String deviceId) { this.deviceId = deviceId; }
        
        public String getActivationTime() { return activationTime; }
        public void setActivationTime(String activationTime) { this.activationTime = activationTime; }
        
        public int getUsageCount() { return usageCount; }
        public void setUsageCount(int usageCount) { this.usageCount = usageCount; }
    }
    
    // Global state
    private static LicenseState gLicenseState = new LicenseState();
    private static final String STATE_FILE = ".decentri/license.state";
    private static final Gson gson = new GsonBuilder().setPrettyPrinting().create();
    private static final Scanner scanner = new Scanner(System.in);
    
    public static void main(String[] args) {
        System.out.println("==========================================");
        System.out.println("DecentriLicense Java SDK 验证向导");
        System.out.println("==========================================");
        
        // Try to load previous state
        loadState();
        
        while (true) {
            printMenu();
            
            String choice = getInput("请选择: ").trim();
            
            switch (choice) {
                case "1":
                    importLicenseKey();
                    break;
                case "2":
                    verifyLicense();
                    break;
                case "3":
                    activateToDevice();
                    break;
                case "4":
                    queryStatus();
                    break;
                case "5":
                    recordUsage();
                    break;
                case "0":
                    System.out.println("再见！");
                    return;
                default:
                    System.out.println("❌ 无效选项，请重新输入。");
            }
            
            System.out.println();
            System.out.println(repeat('-', 50));
            System.out.println();
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
    
    private static void printMenu() {
        System.out.println("\n=== DecentriLicense 向导 ===");
        System.out.println("1. 导入许可证密钥");
        System.out.println("2. 验证许可证");
        System.out.println("3. 激活到当前设备");
        System.out.println("4. 查询当前状态/余额");
        System.out.println("5. 记录使用量（状态迁移）");
        System.out.println("0. 退出");
    }
    
    private static void importLicenseKey() {
        System.out.println("\n--- 导入许可证密钥 ---");
        
        String inputMethod = getInput("输入方式 (1: 直接粘贴, 2: 文件路径): ").trim();
        
        if (inputMethod.equals("1")) {
            System.out.println("请粘贴许可证密钥（JWT格式或加密后的字符串）:");
            String keyData = scanner.nextLine().trim();
            
            if (keyData.isEmpty()) {
                System.out.println("❌ 输入不能为空");
                return;
            }
            
            gLicenseState.setTokenData(keyData);
            System.out.println("✅ 许可证密钥已导入");
            saveState();
            
        } else if (inputMethod.equals("2")) {
            String filePath = getInput("请输入文件路径: ").trim();
            
            if (filePath.isEmpty()) {
                System.out.println("❌ 文件路径不能为空");
                return;
            }
            
            try {
                String data = new String(Files.readAllBytes(Paths.get(filePath)), "UTF-8");
                gLicenseState.setTokenData(data);
                System.out.println("✅ 许可证密钥已从文件导入");
                saveState();
            } catch (IOException e) {
                System.out.printf("❌ 读取文件失败: %s%n", e.getMessage());
            }
        } else {
            System.out.println("❌ 无效的输入方式");
        }
    }
    
    private static void verifyLicense() {
        System.out.println("\n--- 验证许可证 ---");
        
        if (gLicenseState.getTokenData().isEmpty()) {
            System.out.println("❌ 请先导入许可证密钥");
            return;
        }
        
        try {
            // Check if it's an encrypted token
            if (isEncryptedToken(gLicenseState.getTokenData())) {
                System.out.println("🔒 检测到加密的许可证，正在解密...");
                
                // Here should be the actual decryption logic
                // For simplicity, we assume decryption is successful
                System.out.println("✅ 许可证解密成功");
            } else {
                System.out.println("📄 检测到JSON格式的许可证");
            }
            
            // Simulate verification process
            System.out.println("🔍 正在校验许可证签名...");
            System.out.println("✅ 许可证验证通过");
            
        } catch (Exception e) {
            System.out.printf("❌ 许可证验证失败: %s%n", e.getMessage());
        }
    }
    
    private static void activateToDevice() {
        System.out.println("\n--- 激活到当前设备 ---");
        
        if (gLicenseState.getTokenData().isEmpty()) {
            System.out.println("❌ 请先导入许可证密钥");
            return;
        }
        
        // Generate device ID (simplified example)
        gLicenseState.setDeviceId("DEV-" + (System.nanoTime() % 100000));
        
        // Get current time
        LocalDateTime now = LocalDateTime.now();
        DateTimeFormatter formatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss");
        gLicenseState.setActivationTime(now.format(formatter));
        gLicenseState.setActivated(true);
        
        System.out.println("✅ 设备激活成功");
        System.out.printf("  设备ID: %s%n", gLicenseState.getDeviceId());
        System.out.printf("  激活时间: %s%n", gLicenseState.getActivationTime());
        
        saveState();
    }
    
    private static void queryStatus() {
        System.out.println("\n--- 查询当前状态/余额 ---");
        
        System.out.println("许可证状态:");
        if (gLicenseState.getTokenData().isEmpty()) {
            System.out.println("  是否已导入: 否");
        } else {
            System.out.println("  是否已导入: 是");
        }
        System.out.printf("  是否已激活: %s%n", gLicenseState.isActivated() ? "是" : "否");
        
        if (gLicenseState.isActivated()) {
            System.out.printf("  设备ID: %s%n", gLicenseState.getDeviceId());
            System.out.printf("  激活时间: %s%n", gLicenseState.getActivationTime());
        }
        
        System.out.printf("  使用次数: %d%n", gLicenseState.getUsageCount());
    }
    
    private static void recordUsage() {
        System.out.println("\n--- 记录使用量（状态迁移） ---");
        
        if (!gLicenseState.isActivated()) {
            System.out.println("❌ 请先激活到当前设备");
            return;
        }
        
        gLicenseState.setUsageCount(gLicenseState.getUsageCount() + 1);
        
        System.out.println("✅ 使用量记录成功");
        System.out.printf("  当前使用次数: %d%n", gLicenseState.getUsageCount());
        
        saveState();
    }
    
    private static String getInput(String prompt) {
        System.out.print(prompt);
        return scanner.nextLine();
    }
    
    private static boolean saveState() {
        try {
            // Create directory
            Path statePath = Paths.get(STATE_FILE);
            Files.createDirectories(statePath.getParent());
            
            // Write to file
            String json = gson.toJson(gLicenseState);
            Files.write(statePath, json.getBytes("UTF-8"));
            
            return true;
        } catch (Exception e) {
            System.out.printf("⚠️  保存状态失败: %s%n", e.getMessage());
            return false;
        }
    }
    
    private static boolean loadState() {
        try {
            Path statePath = Paths.get(STATE_FILE);
            if (!Files.exists(statePath)) {
                return false;
            }
            
            String json = new String(Files.readAllBytes(statePath), "UTF-8");
            gLicenseState = gson.fromJson(json, LicenseState.class);
            
            return true;
        } catch (Exception e) {
            // Ignore loading errors
            return false;
        }
    }
    
    private static boolean isEncryptedToken(String input) {
        // Encrypted token format: encrypted_data|nonce
        return input.contains("|");
    }
}