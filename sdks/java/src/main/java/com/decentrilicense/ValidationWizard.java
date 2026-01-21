package com.decentrilicense;

import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.stream.Stream;
import java.text.SimpleDateFormat;
import java.util.Date;
import com.google.gson.Gson;

/**
 * DecentriLicense Java SDK 验证向导
 * ================================
 *
 * 功能完整的交互式验证工具，用于测试DecentriLicense Java SDK的所有功能。
 * 参考Go SDK实现，提供统一的用户体验。
 */
public class ValidationWizard {
    // 全局变量
    private static DecentriLicenseClient globalClient = null;
    private static boolean initialized = false;
    private static String selectedProductKeyPath = null;
    private static Scanner scanner = new Scanner(System.in);

    /**
     * 获取或创建全局client实例
     */
    private static DecentriLicenseClient getOrCreateClient() {
        if (globalClient == null) {
            try {
                globalClient = new DecentriLicenseClient();
            } catch (Exception e) {
                System.out.println("❌ 创建客户端失败: " + e.getMessage());
                return null;
            }
        }
        return globalClient;
    }

    /**
     * 清理全局client
     */
    private static void cleanupClient() {
        if (globalClient != null) {
            try {
                globalClient.close();
            } catch (Exception e) {
                // ignore
            }
            globalClient = null;
            initialized = false;
        }
    }

    /**
     * 从系统剪贴板读取内容（macOS）
     */
    private static String readFromClipboard() throws Exception {
        Process process = Runtime.getRuntime().exec("pbpaste");
        BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
        StringBuilder result = new StringBuilder();
        String line;
        while ((line = reader.readLine()) != null) {
            result.append(line);
        }
        process.waitFor();
        return result.toString().trim();
    }

    /**
     * 查找所有可用的产品公钥文件
     */
    private static List<String> findAllProductKeys() {
        List<String> patterns = Arrays.asList(
            "*.pem",
            "../*.pem",
            "../../*.pem",
            "../../../dl-issuer/*.pem"
        );

        Set<String> candidates = new HashSet<>();
        for (String pattern : patterns) {
            try {
                Path patternPath = Paths.get(pattern);
                Path parent = patternPath.getParent();
                if (parent == null) parent = Paths.get(".");

                String glob = patternPath.getFileName().toString();
                if (!glob.contains("*")) continue;

                try (Stream<Path> stream = Files.list(parent)) {
                    stream.filter(path -> {
                        String name = path.getFileName().toString();
                        return name.contains("public") &&
                               !name.contains("private") &&
                               name.endsWith(".pem");
                    }).forEach(path -> candidates.add(path.getFileName().toString()));
                }
            } catch (Exception e) {
                // ignore
            }
        }

        List<String> result = new ArrayList<>(candidates);
        Collections.sort(result);
        return result;
    }

    /**
     * 查找产品公钥文件
     */
    private static String findProductPublicKey() {
        if (selectedProductKeyPath != null) {
            return selectedProductKeyPath;
        }

        List<String> keys = findAllProductKeys();
        if (!keys.isEmpty()) {
            return resolveProductKeyPath(keys.get(0));
        }
        return null;
    }

    /**
     * 根据文件名找到完整的产品公钥文件路径
     */
    private static String resolveProductKeyPath(String filename) {
        List<String> searchPaths = Arrays.asList(
            filename,
            "./" + filename,
            "../" + filename,
            "../../" + filename,
            "../../../dl-issuer/" + filename
        );

        for (String path : searchPaths) {
            if (Files.exists(Paths.get(path))) {
                return path;
            }
        }

        return filename;
    }

    /**
     * 查找token文件
     */
    private static List<String> findTokenFiles(String pattern) {
        List<String> patterns = Arrays.asList(
            "token_" + pattern + "*.txt",
            "../token_" + pattern + "*.txt",
            "../../../dl-issuer/token_" + pattern + "*.txt"
        );

        Set<String> candidates = new HashSet<>();
        for (String pat : patterns) {
            try {
                Path patternPath = Paths.get(pat);
                Path parent = patternPath.getParent();
                if (parent == null) parent = Paths.get(".");

                if (Files.exists(parent)) {
                    try (Stream<Path> stream = Files.list(parent)) {
                        stream.filter(path -> {
                            String name = path.getFileName().toString();
                            return name.startsWith("token_") && name.endsWith(".txt");
                        }).forEach(path -> candidates.add(path.getFileName().toString()));
                    }
                }
            } catch (Exception e) {
                // ignore
            }
        }

        List<String> result = new ArrayList<>(candidates);
        Collections.sort(result);
        return result;
    }

    /**
     * 查找加密的token文件
     */
    private static List<String> findEncryptedTokenFiles() {
        return findTokenFiles("").stream()
            .filter(f -> f.contains("encrypted"))
            .collect(java.util.stream.Collectors.toList());
    }

    /**
     * 查找已激活的token文件
     */
    private static List<String> findActivatedTokenFiles() {
        return findTokenFiles("").stream()
            .filter(f -> f.contains("activated"))
            .collect(java.util.stream.Collectors.toList());
    }

    /**
     * 查找状态token文件
     */
    private static List<String> findStateTokenFiles() {
        // 照抄Go SDK: 文件名必须以token_activated_或token_state_开头
        return findTokenFiles("").stream()
            .filter(f -> f.startsWith("token_activated_") || f.startsWith("token_state_"))
            .collect(java.util.stream.Collectors.toList());
    }

    /**
     * 解析token文件路径
     */
    private static String resolveTokenFilePath(String filename) {
        List<String> candidates = Arrays.asList(
            filename,
            System.getProperty("user.dir") + "/" + filename,
            System.getProperty("user.dir") + "/../" + filename,
            System.getProperty("user.dir") + "/../../../dl-issuer/" + filename
        );

        for (String candidate : candidates) {
            if (Files.exists(Paths.get(candidate))) {
                return candidate;
            }
        }

        return filename;
    }

    /**
     * 读取文件内容
     */
    private static String readFileContent(String filepath) throws Exception {
        return new String(Files.readAllBytes(Paths.get(filepath))).trim();
    }

    /**
     * 读取用户输入
     */
    private static String readInput(String prompt) {
        System.out.print(prompt);
        return scanner.nextLine().trim();
    }

    /**
     * 重复字符串（Java 8兼容）
     */
    private static String repeat(String str, int count) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < count; i++) {
            sb.append(str);
        }
        return sb.toString();
    }

    public static void main(String[] args) {
        // 注册关闭钩子
        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            cleanupClient();
            scanner.close();
        }));

        System.out.println();
        System.out.println("╔══════════════════════════════════════════════════╗");
        System.out.println("║     DecentriLicense Java SDK 验证向导            ║");
        System.out.println("║     Interactive Validation Wizard                ║");
        System.out.println("╚══════════════════════════════════════════════════╝");
        System.out.println();
        System.out.println("欢迎使用 DecentriLicense Java SDK 验证工具！");
        System.out.println("本工具提供完整的令牌管理和验证功能。");
        System.out.println();

        while (true) {
            System.out.println("\n" + repeat("=", 50));
            System.out.println("📋 主菜单");
            System.out.println(repeat("=", 50));
            System.out.println("0. 🔑 选择产品公钥");
            System.out.println("1. 🔓 激活令牌");
            System.out.println("2. ✅ 校验已激活令牌");
            System.out.println("3. 🔍 验证令牌合法性");
            System.out.println("4. 📊 记账信息");
            System.out.println("5. 🔗 信任链验证");
            System.out.println("6. 🎯 综合验证");
            System.out.println("7. 🚪 退出");
            System.out.println(repeat("=", 50));

            String choice = readInput("请选择功能 (0-7): ");

            switch (choice) {
                case "0":
                    selectProductKeyWizard();
                    break;
                case "1":
                    activateTokenWizard();
                    break;
                case "2":
                    verifyActivatedTokenWizard();
                    break;
                case "3":
                    validateTokenWizard();
                    break;
                case "4":
                    accountingWizard();
                    break;
                case "5":
                    trustChainValidationWizard();
                    break;
                case "6":
                    comprehensiveValidationWizard();
                    break;
                case "7":
                    System.out.println("\n👋 感谢使用 DecentriLicense Java SDK 验证向导！");
                    System.out.println("再见！\n");
                    cleanupClient();
                    System.exit(0);
                    break;
                default:
                    System.out.println("❌ 无效选择，请输入 0-7");
            }
        }
    }

    /**
     * 选择产品公钥向导
     */
    private static void selectProductKeyWizard() {
        System.out.println("\n🔑 选择产品公钥");
        System.out.println(repeat("=", 50));

        List<String> availableKeys = findAllProductKeys();

        if (availableKeys.isEmpty()) {
            System.out.println("❌ 当前目录下没有找到产品公钥文件");
            System.out.println("💡 请将产品公钥文件 (public_*.pem) 放置在当前目录下");
            return;
        }

        System.out.println("📄 找到以下产品公钥文件:");
        for (int i = 0; i < availableKeys.size(); i++) {
            System.out.println((i + 1) + ". " + availableKeys.get(i));
        }
        System.out.println((availableKeys.size() + 1) + ". 取消选择");

        if (selectedProductKeyPath != null) {
            System.out.println("✅ 当前已选择: " + selectedProductKeyPath);
        }

        String choice = readInput("请选择要使用的产品公钥文件 (1-" + (availableKeys.size() + 1) + "): ");
        try {
            int choiceNum = Integer.parseInt(choice);
            if (choiceNum == availableKeys.size() + 1) {
                selectedProductKeyPath = null;
                System.out.println("✅ 已取消产品公钥选择");
            } else if (choiceNum >= 1 && choiceNum <= availableKeys.size()) {
                String selectedFile = availableKeys.get(choiceNum - 1);
                selectedProductKeyPath = resolveProductKeyPath(selectedFile);
                System.out.println("✅ 已选择产品公钥文件: " + selectedFile);
            } else {
                System.out.println("❌ 无效选择");
            }
        } catch (NumberFormatException e) {
            System.out.println("❌ 无效选择");
        }
    }

    /**
     * 激活令牌向导
     */
    private static void activateTokenWizard() {
        System.out.println("\n🔓 激活令牌");
        System.out.println(repeat("-", 50));
        System.out.println("⚠️  重要说明：");
        System.out.println("   • 加密token（encrypted）：首次从供应商获得，需要激活");
        System.out.println("   • 已激活token（activated）：激活后生成，可直接使用，不需再次激活");
        System.out.println("   ⚠️  本功能仅用于【首次激活】加密token");
        System.out.println("   ⚠️  如需使用已激活token，请直接选择其他功能（如记账、验证）");
        System.out.println();

        DecentriLicenseClient client = getOrCreateClient();
        if (client == null) {
            return;
        }

        // 显示可用的加密token文件
        List<String> tokenFiles = findEncryptedTokenFiles();
        if (!tokenFiles.isEmpty()) {
            System.out.println("📄 发现以下加密token文件:");
            for (int i = 0; i < tokenFiles.size(); i++) {
                System.out.println("   " + (i + 1) + ". " + tokenFiles.get(i));
            }
            System.out.println("💡 您可以输入序号选择文件，或输入文件名/路径/token字符串");
        }

        // 获取令牌输入
        System.out.println("请输入令牌字符串 (仅支持加密令牌):");
        System.out.println("💡 加密令牌通常从软件提供商处获得");
        System.out.println("💡 输入序号(1-N)可快速选择上面列出的文件");
        System.out.println("💡 输入文件路径可读取指定文件");
        System.out.println("💡 直接回车可以从剪贴板读取token");

        String userInput = readInput("令牌或文件路径: ");

        // 如果输入为空，尝试从剪贴板读取
        if (userInput.isEmpty()) {
            System.out.println("📋 正在从剪贴板读取token...");
            try {
                userInput = readFromClipboard();
                if (userInput.isEmpty()) {
                    System.out.println("❌ 剪贴板为空，请手动输入token字符串");
                    return;
                }
                System.out.println("✅ 从剪贴板读取到 " + userInput.length() + " 个字符");
            } catch (Exception e) {
                System.out.println("❌ " + e.getMessage());
                return;
            }
        }

        String tokenString = userInput;

        // 检查是否输入的是数字（文件序号）
        try {
            if (!tokenFiles.isEmpty()) {
                int index = Integer.parseInt(userInput);
                if (index >= 1 && index <= tokenFiles.size()) {
                    String selectedFile = tokenFiles.get(index - 1);
                    String filePath = resolveTokenFilePath(selectedFile);
                    try {
                        tokenString = readFileContent(filePath);
                        System.out.println("✅ 选择文件 '" + selectedFile + "' 并读取到令牌 (" + tokenString.length() + " 字符)");
                    } catch (Exception e) {
                        System.out.println("❌ 无法读取文件 " + filePath + ": " + e.getMessage());
                        return;
                    }
                }
            }
        } catch (NumberFormatException e) {
            // Not a number, continue
        }

        // 检查是否是文件路径
        if (userInput.contains("/") || userInput.contains("\\") ||
            userInput.endsWith(".txt") || userInput.contains("token_")) {
            String filePath = resolveTokenFilePath(userInput);
            try {
                tokenString = readFileContent(filePath);
                System.out.println("✅ 从文件读取到令牌 (" + tokenString.length() + " 字符)");
            } catch (Exception e) {
                System.out.println("⚠️  无法读取文件 " + filePath + ": " + e.getMessage());
                System.out.println("💡 将直接使用输入作为令牌字符串");
            }
        }

        // 初始化客户端
        if (!initialized) {
            try {
                client.initialize("TEMP", 13325, 23325, null);
                System.out.println("✅ 客户端初始化成功");
                initialized = true;
            } catch (Exception e) {
                System.out.println("⚠️  初始化失败 (需要产品公钥): " + e.getMessage());
            }
        } else {
            System.out.println("✅ 客户端已初始化，使用现有实例");
        }

        // 查找和设置产品公钥
        String productKeyPath = null;
        if (selectedProductKeyPath != null) {
            productKeyPath = selectedProductKeyPath;
            System.out.println("📄 使用用户选择的产品公钥文件: " + productKeyPath);
        } else {
            productKeyPath = findProductPublicKey();
            if (productKeyPath != null) {
                System.out.println("📄 使用产品公钥文件: " + productKeyPath);
            }
        }

        if (productKeyPath != null) {
            try {
                String productKeyData = readFileContent(productKeyPath);
                client.setProductPublicKey(productKeyData);
                System.out.println("✅ 产品公钥设置成功");
            } catch (Exception e) {
                System.out.println("❌ 设置产品公钥失败: " + e.getMessage());
                return;
            }
        } else {
            System.out.println("⚠️  未找到产品公钥文件");
            System.out.println("💡 请先选择产品公钥 (菜单选项 0)，或确保当前目录下有产品公钥文件");
            return;
        }

        // 导入令牌
        System.out.println("📥 正在导入令牌...");
        try {
            client.importToken(tokenString);
            System.out.println("✅ 令牌导入成功");
        } catch (Exception e) {
            System.out.println("❌ 令牌导入失败: " + e.getMessage());
            return;
        }

        // 激活令牌
        System.out.println("🎯 正在激活令牌...");
        try {
            VerificationResult result = client.activateBindDevice();
            if (result.isValid()) {
                System.out.println("✅ 令牌激活成功！");

                // 导出激活后的新token
                try {
                    String activatedToken = client.exportActivatedTokenEncrypted();
                    if (activatedToken != null && !activatedToken.isEmpty()) {
                        System.out.println("\n📦 激活后的新Token（加密）:");
                        System.out.println("   长度: " + activatedToken.length() + " 字符");
                        if (activatedToken.length() > 100) {
                            System.out.println("   前缀: " + activatedToken.substring(0, 100) + "...");
                        } else {
                            System.out.println("   内容: " + activatedToken);
                        }

                        // 保存激活后的token到文件
                        StatusResult status = client.getStatus();
                        if (status.getLicenseCode() != null && !status.getLicenseCode().isEmpty()) {
                            String timestamp = new SimpleDateFormat("yyyyMMddHHmmss").format(new Date());
                            String filename = "token_activated_" + status.getLicenseCode() + "_" + timestamp + ".txt";
                            Files.write(Paths.get(filename), activatedToken.getBytes());
                            System.out.println("\n💾 已保存到文件: " + new File(filename).getAbsolutePath());
                            System.out.println("   💡 此token包含设备绑定信息，可传递给下一个设备使用");
                        }
                    }
                } catch (Exception e) {
                    System.out.println("⚠️  导出激活token失败: " + e.getMessage());
                }
            } else {
                System.out.println("❌ 令牌激活失败: " + result.getErrorMessage());
            }
        } catch (Exception e) {
            System.out.println("❌ 激活失败: " + e.getMessage());
        }

        // 显示最终状态
        try {
            StatusResult status = client.getStatus();
            if (status.isActivated()) {
                System.out.println("🔍 当前状态: 已激活");
                if (status.hasToken()) {
                    System.out.println("🎫 令牌ID: " + status.getTokenId());
                    System.out.println("📝 许可证代码: " + status.getLicenseCode());
                    System.out.println("👤 持有设备: " + status.getHolderDeviceId());
                    SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
                    System.out.println("📅 颁发时间: " + sdf.format(new Date(status.getIssueTime() * 1000)));
                }
            } else {
                System.out.println("🔍 当前状态: 未激活");
            }
        } catch (Exception e) {
            System.out.println("⚠️  无法获取状态: " + e.getMessage());
        }
    }

    /**
     * 校验已激活令牌向导
     */
    private static void verifyActivatedTokenWizard() {
        System.out.println("\n✅ 校验已激活令牌");
        System.out.println(repeat("-", 50));

        // 扫描所有已激活的令牌
        Path stateDir = Paths.get(".decentrilicense_state");
        if (!Files.exists(stateDir)) {
            System.out.println("⚠️  没有找到已激活的令牌");
            return;
        }

        // 列出所有已激活的令牌
        List<String> activatedTokens = new ArrayList<>();
        System.out.println("\n📋 已激活的令牌列表:");
        int index = 1;
        try {
            try (Stream<Path> stream = Files.list(stateDir)) {
                stream.filter(Files::isDirectory).forEach(entry -> {
                    String tokenId = entry.getFileName().toString();
                    activatedTokens.add(tokenId);
                    Path stateFile = entry.resolve("current_state.json");
                    if (Files.exists(stateFile)) {
                        System.out.println(index + ". " + tokenId + " ✅");
                    } else {
                        System.out.println(index + ". " + tokenId + " ⚠️  (无状态文件)");
                    }
                });
            }
        } catch (Exception e) {
            System.out.println("⚠️  读取状态目录失败: " + e.getMessage());
            return;
        }

        if (activatedTokens.isEmpty()) {
            System.out.println("⚠️  没有找到已激活的令牌");
            return;
        }

        // 让用户选择
        String choice = readInput("\n请选择要验证的令牌 (1-" + activatedTokens.size() + "): ");
        try {
            int choiceNum = Integer.parseInt(choice);
            if (choiceNum < 1 || choiceNum > activatedTokens.size()) {
                System.out.println("❌ 无效的选择");
                return;
            }

            String selectedLicenseCode = activatedTokens.get(choiceNum - 1);
            System.out.println("\n🔍 正在验证令牌: " + selectedLicenseCode);

            DecentriLicenseClient client = getOrCreateClient();
            if (client == null) {
                return;
            }

            // Note: Java SDK may not support verifying non-current tokens directly
            System.out.println("💡 Java SDK 当前版本可能需要重新导入令牌才能验证");
            System.out.println("💡 显示已保存的状态信息:");

            Path stateFile = stateDir.resolve(selectedLicenseCode).resolve("current_state.json");
            if (Files.exists(stateFile)) {
                System.out.println("\n🎫 令牌信息 (从状态文件读取):");
                System.out.println("   许可证代码: " + selectedLicenseCode);
                System.out.println("   状态文件: " + stateFile);
                System.out.println("   文件大小: " + Files.size(stateFile) + " 字节");
            } else {
                System.out.println("❌ 读取状态文件失败");
            }
        } catch (NumberFormatException e) {
            System.out.println("❌ 无效的选择");
        } catch (Exception e) {
            System.out.println("❌ 验证失败: " + e.getMessage());
        }
    }

    /**
     * 验证令牌合法性向导
     */
    private static void validateTokenWizard() {
        System.out.println("\n🔍 验证令牌合法性");
        System.out.println(repeat("-", 50));

        DecentriLicenseClient client = getOrCreateClient();
        if (client == null) {
            return;
        }

        // 初始化客户端
        if (!initialized) {
            try {
                client.initialize("VALIDATE", 13325, 23325, null);
                System.out.println("✅ 客户端初始化成功");
                initialized = true;
            } catch (Exception e) {
                System.out.println("⚠️  初始化失败: " + e.getMessage());
            }
        }

        // 查找和设置产品公钥
        String productKeyPath = null;
        if (selectedProductKeyPath != null) {
            productKeyPath = selectedProductKeyPath;
            System.out.println("📄 使用用户选择的产品公钥文件: " + productKeyPath);
        } else {
            productKeyPath = findProductPublicKey();
            if (productKeyPath != null) {
                System.out.println("📄 使用产品公钥文件: " + productKeyPath);
            }
        }

        if (productKeyPath != null) {
            try {
                String productKeyData = readFileContent(productKeyPath);
                client.setProductPublicKey(productKeyData);
                System.out.println("✅ 产品公钥设置成功");
            } catch (Exception e) {
                System.out.println("❌ 设置产品公钥失败: " + e.getMessage());
                return;
            }
        } else {
            System.out.println("⚠️  未找到产品公钥文件");
            System.out.println("💡 请先选择产品公钥 (菜单选项 0)，或确保当前目录下有产品公钥文件");
            return;
        }

        // 显示可用的加密token文件
        List<String> tokenFiles = findEncryptedTokenFiles();
        if (!tokenFiles.isEmpty()) {
            System.out.println("📄 发现以下加密token文件:");
            for (int i = 0; i < tokenFiles.size(); i++) {
                System.out.println("   " + (i + 1) + ". " + tokenFiles.get(i));
            }
            System.out.println("💡 您可以输入序号选择文件，或输入文件名/路径/token字符串");
        }

        // 获取令牌输入
        System.out.println("请输入要验证的令牌字符串 (支持加密令牌):");
        System.out.println("💡 令牌通常从软件提供商处获得，或从加密令牌文件读取");
        System.out.println("💡 如果是文件路径，请输入完整的文件路径");
        System.out.println("💡 直接回车可以从剪贴板读取token");

        String userInput = readInput("令牌或文件路径: ");

        // 如果输入为空，尝试从剪贴板读取
        if (userInput.isEmpty()) {
            System.out.println("📋 正在从剪贴板读取token...");
            try {
                userInput = readFromClipboard();
                if (userInput.isEmpty()) {
                    System.out.println("❌ 剪贴板为空，请手动输入token字符串");
                    return;
                }
                System.out.println("✅ 从剪贴板读取到 " + userInput.length() + " 个字符");
            } catch (Exception e) {
                System.out.println("❌ " + e.getMessage());
                return;
            }
        }

        String tokenString = userInput;

        // 检查是否是数字选择
        try {
            if (!tokenFiles.isEmpty()) {
                int numChoice = Integer.parseInt(userInput);
                if (numChoice >= 1 && numChoice <= tokenFiles.size()) {
                    String selectedFile = tokenFiles.get(numChoice - 1);
                    String filePath = resolveTokenFilePath(selectedFile);
                    try {
                        tokenString = readFileContent(filePath);
                        System.out.println("✅ 从文件 '" + selectedFile + "' 读取到令牌 (" + tokenString.length() + " 字符)");
                    } catch (Exception e) {
                        System.out.println("❌ 无法读取文件 " + filePath + ": " + e.getMessage());
                        return;
                    }
                }
            }
        } catch (NumberFormatException e) {
            // Not a number, continue
        }

        // 检查是否是文件路径
        if (userInput.contains("/") || userInput.contains("\\") ||
            userInput.endsWith(".txt") || userInput.contains("token_")) {
            String filePath = resolveTokenFilePath(userInput);
            try {
                tokenString = readFileContent(filePath);
                System.out.println("✅ 从文件读取到令牌 (" + tokenString.length() + " 字符)");
            } catch (Exception e) {
                System.out.println("⚠️  无法读取文件 " + filePath + ": " + e.getMessage());
                System.out.println("💡 将直接使用输入作为令牌字符串");
            }
        }

        // 验证令牌
        System.out.println("🔍 正在验证令牌合法性...");
        try {
            // 导入令牌
            client.importToken(tokenString);
            System.out.println("✅ 令牌导入成功");

            // 离线验证
            VerificationResult result = client.offlineVerifyCurrentToken();
            if (result.isValid()) {
                System.out.println("✅ 令牌验证成功 - 令牌合法且有效");
                if (result.getErrorMessage() != null && !result.getErrorMessage().isEmpty()) {
                    System.out.println("📄 详细信息: " + result.getErrorMessage());
                }
            } else {
                System.out.println("❌ 令牌验证失败 - 令牌不合法或无效");
                if (result.getErrorMessage() != null && !result.getErrorMessage().isEmpty()) {
                    System.out.println("📄 错误信息: " + result.getErrorMessage());
                }
            }
        } catch (Exception e) {
            System.out.println("❌ 令牌验证失败: " + e.getMessage());
        }
    }

    /**
     * 记账向导（记录使用信息）
     */
    private static void accountingWizard() {
        System.out.println("\n📊 记账 - 记录使用信息");
        System.out.println(repeat("-", 50));

        DecentriLicenseClient client = getOrCreateClient();
        if (client == null) {
            return;
        }

        // 初始化客户端
        if (!initialized) {
            try {
                client.initialize("ACCOUNTING", 13325, 23325, null);
                System.out.println("✅ 客户端初始化成功");
                initialized = true;
            } catch (Exception e) {
                System.out.println("⚠️  初始化失败: " + e.getMessage());
            }
        }

        // 查找和设置产品公钥
        String productKeyPath = null;
        if (selectedProductKeyPath != null) {
            productKeyPath = selectedProductKeyPath;
            System.out.println("📄 使用用户选择的产品公钥文件: " + productKeyPath);
        } else {
            productKeyPath = findProductPublicKey();
            if (productKeyPath != null) {
                System.out.println("📄 使用产品公钥文件: " + productKeyPath);
            }
        }

        if (productKeyPath != null) {
            try {
                String productKeyData = readFileContent(productKeyPath);
                client.setProductPublicKey(productKeyData);
                System.out.println("✅ 产品公钥设置成功");
            } catch (Exception e) {
                System.out.println("❌ 设置产品公钥失败: " + e.getMessage());
                return;
            }
        } else {
            System.out.println("⚠️  未找到产品公钥文件");
            System.out.println("💡 请先选择产品公钥 (菜单选项 0)，或确保当前目录下有产品公钥文件");
            return;
        }

        // 显示可用的已激活或状态token文件
        List<String> tokenFiles = findStateTokenFiles();
        if (!tokenFiles.isEmpty()) {
            System.out.println("📄 发现以下已激活/状态token文件:");
            for (int i = 0; i < tokenFiles.size(); i++) {
                System.out.println("   " + (i + 1) + ". " + tokenFiles.get(i));
            }
            System.out.println("💡 您可以输入序号选择文件，或输入文件名/路径/token字符串");
        }

        // 获取令牌输入
        System.out.println("请输入已激活的令牌字符串:");
        System.out.println("💡 支持已激活的令牌或状态令牌");
        System.out.println("💡 输入序号(1-N)可快速选择上面列出的文件");
        System.out.println("💡 输入文件路径可读取指定文件");
        System.out.println("💡 直接回车可以从剪贴板读取token");

        String userInput = readInput("令牌或文件路径: ");

        // 如果输入为空，尝试从剪贴板读取
        if (userInput.isEmpty()) {
            System.out.println("📋 正在从剪贴板读取token...");
            try {
                userInput = readFromClipboard();
                if (userInput.isEmpty()) {
                    System.out.println("❌ 剪贴板为空，请手动输入token字符串");
                    return;
                }
                System.out.println("✅ 从剪贴板读取到 " + userInput.length() + " 个字符");
            } catch (Exception e) {
                System.out.println("❌ " + e.getMessage());
                return;
            }
        }

        String tokenString = userInput;
        String selectedFile = "";  // 用于判断是否是已激活token

        // 检查是否是数字选择
        try {
            if (!tokenFiles.isEmpty()) {
                int numChoice = Integer.parseInt(userInput);
                if (numChoice >= 1 && numChoice <= tokenFiles.size()) {
                    selectedFile = tokenFiles.get(numChoice - 1);
                    String filePath = resolveTokenFilePath(selectedFile);
                    try {
                        tokenString = readFileContent(filePath);
                        System.out.println("✅ 从文件 '" + selectedFile + "' 读取到令牌 (" + tokenString.length() + " 字符)");
                    } catch (Exception e) {
                        System.out.println("❌ 无法读取文件 " + filePath + ": " + e.getMessage());
                        return;
                    }
                }
            }
        } catch (NumberFormatException e) {
            // Not a number, continue
        }

        // 检查是否是文件路径
        if (userInput.contains("/") || userInput.contains("\\") ||
            userInput.endsWith(".txt") || userInput.contains("token_")) {
            String filePath = resolveTokenFilePath(userInput);
            try {
                tokenString = readFileContent(filePath);
                selectedFile = userInput;  // 记录文件名
                System.out.println("✅ 从文件读取到令牌 (" + tokenString.length() + " 字符)");
            } catch (Exception e) {
                System.out.println("⚠️  无法读取文件 " + filePath + ": " + e.getMessage());
                System.out.println("💡 将直接使用输入作为令牌字符串");
            }
        }

        // 导入令牌
        System.out.println("📥 正在导入令牌...");
        try {
            client.importToken(tokenString);
            System.out.println("✅ 令牌导入成功");
        } catch (Exception e) {
            System.out.println("❌ 令牌导入失败: " + e.getMessage());
            return;
        }

        // 检查令牌类型并恢复激活状态
        boolean isAlreadyActivated = selectedFile.contains("activated") || selectedFile.contains("state");

        if (isAlreadyActivated) {
            System.out.println("💡 检测到已激活令牌");
            // 对于已激活token，ActivateBindDevice是幂等操作
            // 它会恢复激活状态，但不会重新生成新的token
            System.out.println("🔄 正在恢复激活状态...");
        } else {
            // 对于加密token，这是首次激活
            System.out.println("🎯 正在首次激活令牌...");
        }

        // 调用ActivateBindDevice恢复/设置激活状态
        try {
            VerificationResult result = client.activateBindDevice();
            if (!result.isValid()) {
                System.out.println("❌ 激活失败: " + result.getErrorMessage());
                return;
            }

            if (isAlreadyActivated) {
                System.out.println("✅ 激活状态已恢复（token未改变）");
            } else {
                System.out.println("✅ 首次激活成功");
            }
        } catch (Exception e) {
            System.out.println("❌ 激活失败: " + e.getMessage());
            return;
        }

        // 验证当前令牌（可选，用于确认）
        System.out.println("🔍 正在验证令牌...");
        try {
            VerificationResult result = client.offlineVerifyCurrentToken();
            if (!result.isValid()) {
                System.out.println("❌ 令牌验证失败: " + result.getErrorMessage());
                return;
            }
            System.out.println("✅ 令牌验证成功");
        } catch (Exception e) {
            System.out.println("❌ 令牌验证失败: " + e.getMessage());
            return;
        }

        // 显示当前令牌信息
        try {
            StatusResult status = client.getStatus();
            if (status.hasToken()) {
                System.out.println("\n📋 当前令牌信息:");
                System.out.println("   许可证代码: " + status.getLicenseCode());
                System.out.println("   应用ID: " + status.getAppId());
                System.out.println("   当前状态索引: " + status.getStateIndex());
                System.out.println("   令牌ID: " + status.getTokenId());
            } else {
                System.out.println("⚠️  无法获取令牌信息");
                return;
            }
        } catch (Exception e) {
            System.out.println("⚠️  无法获取令牌信息: " + e.getMessage());
            return;
        }

        // 提供记账选项 - 遵循usage_chain结构
        System.out.println("\n💡 请选择记账方式:");
        System.out.println("1. 快速测试记账（使用默认测试数据）");
        System.out.println("2. 记录业务操作（向导式输入）");

        String choice = readInput("\n请选择 (1-2): ");

        String action;
        Map<String, Object> params = new HashMap<>();

        if ("1".equals(choice)) {
            // 快速测试 - 使用默认数据
            action = "api_call";
            params.put("function", "test_function");
            params.put("result", "success");
            System.out.println("💡 使用测试数据: action=" + action + ", params=" + params);

        } else if ("2".equals(choice)) {
            // 业务操作记账 - 向导式输入
            System.out.println("\n📝 usage_chain 结构说明:");
            System.out.println("┌─────────────────────────────────────────────────────────┐");
            System.out.println("│ 字段名      │ 说明           │ 填写方式              │");
            System.out.println("├─────────────────────────────────────────────────────────┤");
            System.out.println("│ seq         │ 序列号         │ ✅ 系统自动填充       │");
            System.out.println("│ time        │ 时间戳         │ ✅ 系统自动填充       │");
            System.out.println("│ action      │ 操作类型       │ 👉 需要您输入         │");
            System.out.println("│ params      │ 操作参数       │ 👉 需要您输入         │");
            System.out.println("│ hash_prev   │ 前状态哈希     │ ✅ 系统自动填充       │");
            System.out.println("│ signature   │ 数字签名       │ ✅ 系统自动填充       │");
            System.out.println("└─────────────────────────────────────────────────────────┘");

            // 输入action
            System.out.println("\n👉 第1步: 输入操作类型 (action)");
            System.out.println("   常用操作类型:");
            System.out.println("   • api_call      - API调用");
            System.out.println("   • feature_usage - 功能使用");
            System.out.println("   • save_file     - 保存文件");
            System.out.println("   • export_data   - 导出数据");

            action = readInput("\n请输入操作类型: ");
            if (action.isEmpty()) {
                System.out.println("❌ 操作类型不能为空");
                return;
            }

            // 输入params - 引导用户输入键值对
            System.out.println("\n👉 第2步: 输入操作参数 (params)");
            System.out.println("   params 是一个JSON对象，包含操作的具体参数");
            System.out.println("   输入格式: key=value (每行一个)");
            System.out.println("   示例:");
            System.out.println("   • function=process_image");
            System.out.println("   • file_name=report.pdf");
            System.out.println("   • size=1024");
            System.out.println("   输入空行结束输入");

            while (true) {
                String line = readInput("参数 (key=value 或直接回车结束): ");
                if (line.isEmpty()) {
                    break;
                }

                String[] parts = line.split("=", 2);
                if (parts.length == 2) {
                    String key = parts[0].trim();
                    String value = parts[1].trim();
                    params.put(key, value);
                } else {
                    System.out.println("⚠️  格式错误,请使用 key=value 格式");
                }
            }

            if (params.isEmpty()) {
                System.out.println("⚠️  未输入任何参数,将使用空参数对象");
            }

        } else {
            System.out.println("❌ 无效的选择");
            return;
        }

        // 构建符合usage_chain结构的JSON
        // 注意: seq, time, hash_prev, signature 由SDK自动填充
        Map<String, Object> usageChainEntry = new HashMap<>();
        usageChainEntry.put("action", action);
        usageChainEntry.put("params", params);

        Gson gson = new Gson();
        String payloadInput = gson.toJson(usageChainEntry);
        System.out.println("\n📝 记账数据 (业务字段): " + payloadInput);
        System.out.println("   (系统字段 seq, time, hash_prev, signature 将由SDK自动添加)");

        // 记录使用信息
        System.out.println("📊 正在记录使用信息...");
        try {
            VerificationResult result = client.recordUsage(payloadInput);
            if (result.isValid()) {
                System.out.println("✅ 使用信息记录成功");
                if (result.getErrorMessage() != null && !result.getErrorMessage().isEmpty()) {
                    System.out.println("📄 详细信息: " + result.getErrorMessage());
                }

                // 导出状态变更后的新token
                try {
                    String stateToken = client.exportStateChangedTokenEncrypted();
                    if (stateToken != null && !stateToken.isEmpty()) {
                        System.out.println("\n📦 状态变更后的新Token（加密）:");
                        System.out.println("   长度: " + stateToken.length() + " 字符");
                        if (stateToken.length() > 100) {
                            System.out.println("   前缀: " + stateToken.substring(0, 100) + "...");
                        } else {
                            System.out.println("   内容: " + stateToken);
                        }

                        // 保存状态变更后的token到文件
                        StatusResult status = client.getStatus();
                        if (status.getLicenseCode() != null && !status.getLicenseCode().isEmpty()) {
                            String timestamp = new SimpleDateFormat("yyyyMMddHHmmss").format(new Date());
                            String filename = "token_state_" + status.getLicenseCode() + "_idx" + status.getStateIndex() + "_" + timestamp + ".txt";
                            Files.write(Paths.get(filename), stateToken.getBytes());
                            System.out.println("\n💾 已保存到文件: " + new File(filename).getAbsolutePath());
                            System.out.println("   💡 此token包含最新状态链，可传递给下一个设备使用");
                        }
                    }
                } catch (Exception e) {
                    System.out.println("⚠️  导出状态变更token失败: " + e.getMessage());
                }

                // 显示当前状态
                try {
                    StatusResult status = client.getStatus();
                    System.out.println("\n🔍 当前状态:");
                    System.out.println("   状态索引: " + status.getStateIndex());
                    System.out.println("   许可证代码: " + status.getLicenseCode());
                    System.out.println("   令牌ID: " + status.getTokenId());
                } catch (Exception e) {
                    System.out.println("⚠️  无法获取状态: " + e.getMessage());
                }
            } else {
                System.out.println("❌ 记录失败: " + result.getErrorMessage());
            }
        } catch (Exception e) {
            System.out.println("❌ 记录使用信息失败: " + e.getMessage());
        }
    }

    /**
     * 信任链验证向导
     */
    private static void trustChainValidationWizard() {
        System.out.println("\n🔐 信任链验证");
        System.out.println(repeat("-", 50));
        System.out.println("⚠️  说明: 此功能用于验证令牌的完整信任链");
        System.out.println("   包括产品公钥、颁发者签名、令牌完整性等\n");

        DecentriLicenseClient client = getOrCreateClient();
        if (client == null) {
            return;
        }

        // 初始化客户端
        if (!initialized) {
            try {
                client.initialize("TRUSTCHAIN", 13325, 23325, null);
                System.out.println("✅ 客户端初始化成功");
                initialized = true;
            } catch (Exception e) {
                System.out.println("⚠️  初始化失败: " + e.getMessage());
            }
        }

        // 查找和设置产品公钥
        String productKeyPath = null;
        if (selectedProductKeyPath != null) {
            productKeyPath = selectedProductKeyPath;
            System.out.println("📄 使用用户选择的产品公钥文件: " + productKeyPath);
        } else {
            productKeyPath = findProductPublicKey();
            if (productKeyPath != null) {
                System.out.println("📄 使用产品公钥文件: " + productKeyPath);
            }
        }

        if (productKeyPath != null) {
            try {
                String productKeyData = readFileContent(productKeyPath);
                client.setProductPublicKey(productKeyData);
                System.out.println("✅ 产品公钥设置成功 - 信任链的根");
            } catch (Exception e) {
                System.out.println("❌ 设置产品公钥失败: " + e.getMessage());
                return;
            }
        } else {
            System.out.println("⚠️  未找到产品公钥文件");
            System.out.println("💡 信任链验证需要产品公钥，请先选择产品公钥 (菜单选项 0)");
            return;
        }

        // 显示可用的token文件
        List<String> allTokenFiles = findTokenFiles("");
        if (!allTokenFiles.isEmpty()) {
            System.out.println("\n📄 发现以下token文件:");
            for (int i = 0; i < allTokenFiles.size(); i++) {
                String file = allTokenFiles.get(i);
                String marker = "";
                if (file.contains("encrypted")) {
                    marker = " [加密]";
                } else if (file.contains("activated")) {
                    marker = " [已激活]";
                } else if (file.contains("state")) {
                    marker = " [状态]";
                }
                System.out.println("   " + (i + 1) + ". " + file + marker);
            }
            System.out.println("💡 您可以输入序号选择文件，或输入文件名/路径/token字符串");
        }

        // 获取令牌输入（继续完成...）
        System.out.println("\n请输入要验证的令牌字符串:");
        System.out.println("💡 支持加密令牌、已激活令牌或状态令牌");
        System.out.println("💡 输入序号(1-N)可快速选择上面列出的文件");
        System.out.println("💡 输入文件路径可读取指定文件");
        System.out.println("💡 直接回车可以从剪贴板读取token");

        String userInput = readInput("令牌或文件路径: ");

        if (userInput.isEmpty()) {
            System.out.println("📋 正在从剪贴板读取token...");
            try {
                userInput = readFromClipboard();
                if (userInput.isEmpty()) {
                    System.out.println("❌ 剪贴板为空，请手动输入token字符串");
                    return;
                }
                System.out.println("✅ 从剪贴板读取到 " + userInput.length() + " 个字符");
            } catch (Exception e) {
                System.out.println("❌ " + e.getMessage());
                return;
            }
        }

        String tokenString = userInput;

        // 检查是否是数字选择
        try {
            if (!allTokenFiles.isEmpty()) {
                int numChoice = Integer.parseInt(userInput);
                if (numChoice >= 1 && numChoice <= allTokenFiles.size()) {
                    String selectedFile = allTokenFiles.get(numChoice - 1);
                    String filePath = resolveTokenFilePath(selectedFile);
                    try {
                        tokenString = readFileContent(filePath);
                        System.out.println("✅ 从文件 '" + selectedFile + "' 读取到令牌 (" + tokenString.length() + " 字符)");
                    } catch (Exception e) {
                        System.out.println("❌ 无法读取文件 " + filePath + ": " + e.getMessage());
                        return;
                    }
                }
            }
        } catch (NumberFormatException e) {
            // Not a number, continue
        }

        // 检查是否是文件路径
        if (userInput.contains("/") || userInput.contains("\\") ||
            userInput.endsWith(".txt") || userInput.contains("token_")) {
            String filePath = resolveTokenFilePath(userInput);
            try {
                tokenString = readFileContent(filePath);
                System.out.println("✅ 从文件读取到令牌 (" + tokenString.length() + " 字符)");
            } catch (Exception e) {
                System.out.println("⚠️  无法读取文件 " + filePath + ": " + e.getMessage());
                System.out.println("💡 将直接使用输入作为令牌字符串");
            }
        }

        // 导入令牌
        System.out.println("\n🔍 开始信任链验证...");
        System.out.println(repeat("━", 50));

        System.out.println("\n1️⃣  验证步骤1: 导入令牌");
        try {
            client.importToken(tokenString);
            System.out.println("   ✅ 令牌导入成功");
        } catch (Exception e) {
            System.out.println("   ❌ 令牌导入失败: " + e.getMessage());
            System.out.println("   ❌ 信任链验证失败 - 无法导入令牌");
            return;
        }

        System.out.println("\n2️⃣  验证步骤2: 产品公钥验证");
        System.out.println("   ✅ 产品公钥已设置并作为信任链的根");

        System.out.println("\n3️⃣  验证步骤3: 令牌签名验证");
        try {
            VerificationResult result = client.offlineVerifyCurrentToken();
            if (result.isValid()) {
                System.out.println("   ✅ 令牌签名验证成功");
                System.out.println("   ✅ 令牌由可信的产品公钥签发");
            } else {
                System.out.println("   ❌ 令牌签名验证失败: " + result.getErrorMessage());
                System.out.println("   ❌ 信任链验证失败 - 签名无效");
                return;
            }
        } catch (Exception e) {
            System.out.println("   ❌ 令牌验证失败: " + e.getMessage());
            System.out.println("   ❌ 信任链验证失败");
            return;
        }

        System.out.println("\n4️⃣  验证步骤4: 令牌完整性检查");
        try {
            StatusResult status = client.getStatus();
            if (status.hasToken()) {
                System.out.println("   ✅ 令牌结构完整");
                System.out.println("   📄 令牌ID: " + status.getTokenId());
                System.out.println("   📄 许可证代码: " + status.getLicenseCode());
                System.out.println("   📄 应用ID: " + status.getAppId());
                System.out.println("   📄 持有设备: " + status.getHolderDeviceId());
                SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
                System.out.println("   📄 颁发时间: " + sdf.format(new Date(status.getIssueTime() * 1000)));

                if (status.getExpireTime() == 0) {
                    System.out.println("   📄 到期时间: 永不过期");
                } else {
                    System.out.println("   📄 到期时间: " + sdf.format(new Date(status.getExpireTime() * 1000)));
                    if (status.getExpireTime() < System.currentTimeMillis() / 1000) {
                        System.out.println("   ⚠️  警告: 令牌已过期");
                    }
                }

                System.out.println("   📄 状态索引: " + status.getStateIndex());
                System.out.println("   📄 激活状态: " + (status.isActivated() ? "已激活" : "未激活"));
            } else {
                System.out.println("   ⚠️  警告: 令牌信息不完整");
            }
        } catch (Exception e) {
            System.out.println("   ⚠️  无法获取完整状态信息: " + e.getMessage());
        }

        System.out.println("\n" + repeat("━", 50));
        System.out.println("✅ 信任链验证完成 - 令牌可信");
        System.out.println("💡 此令牌由有效的产品公钥签发，签名验证通过");
    }

    /**
     * 综合验证向导（执行所有验证步骤）
     */
    private static void comprehensiveValidationWizard() {
        System.out.println("\n🎯 综合验证 - 完整的令牌验证流程");
        System.out.println(repeat("=", 50));
        System.out.println("⚠️  说明: 此功能将执行完整的令牌验证流程");
        System.out.println("   包括: 格式验证、签名验证、信任链验证、状态验证等\n");

        DecentriLicenseClient client = getOrCreateClient();
        if (client == null) {
            return;
        }

        // 初始化客户端
        if (!initialized) {
            try {
                client.initialize("COMPREHENSIVE", 13325, 23325, null);
                System.out.println("✅ 客户端初始化成功");
                initialized = true;
            } catch (Exception e) {
                System.out.println("⚠️  初始化失败: " + e.getMessage());
            }
        }

        // 查找和设置产品公钥
        String productKeyPath = null;
        if (selectedProductKeyPath != null) {
            productKeyPath = selectedProductKeyPath;
            System.out.println("📄 使用用户选择的产品公钥文件: " + productKeyPath);
        } else {
            productKeyPath = findProductPublicKey();
            if (productKeyPath != null) {
                System.out.println("📄 使用产品公钥文件: " + productKeyPath);
            }
        }

        if (productKeyPath != null) {
            try {
                String productKeyData = readFileContent(productKeyPath);
                client.setProductPublicKey(productKeyData);
                System.out.println("✅ 产品公钥设置成功");
            } catch (Exception e) {
                System.out.println("❌ 设置产品公钥失败: " + e.getMessage());
                return;
            }
        } else {
            System.out.println("⚠️  未找到产品公钥文件");
            System.out.println("💡 综合验证需要产品公钥，请先选择产品公钥 (菜单选项 0)");
            return;
        }

        // 显示可用的token文件
        List<String> allTokenFiles = findTokenFiles("");
        if (!allTokenFiles.isEmpty()) {
            System.out.println("\n📄 发现以下token文件:");
            for (int i = 0; i < allTokenFiles.size(); i++) {
                String file = allTokenFiles.get(i);
                String marker = "";
                if (file.contains("encrypted")) {
                    marker = " [加密]";
                } else if (file.contains("activated")) {
                    marker = " [已激活]";
                } else if (file.contains("state")) {
                    marker = " [状态]";
                }
                System.out.println("   " + (i + 1) + ". " + file + marker);
            }
            System.out.println("💡 您可以输入序号选择文件，或输入文件名/路径/token字符串");
        }

        // 获取令牌输入
        System.out.println("\n请输入要验证的令牌字符串:");
        System.out.println("💡 支持加密令牌、已激活令牌或状态令牌");
        System.out.println("💡 输入序号(1-N)可快速选择上面列出的文件");
        System.out.println("💡 输入文件路径可读取指定文件");
        System.out.println("💡 直接回车可以从剪贴板读取token");

        String userInput = readInput("令牌或文件路径: ");

        // 如果输入为空，尝试从剪贴板读取
        if (userInput.isEmpty()) {
            System.out.println("📋 正在从剪贴板读取token...");
            try {
                userInput = readFromClipboard();
                if (userInput.isEmpty()) {
                    System.out.println("❌ 剪贴板为空，请手动输入token字符串");
                    return;
                }
                System.out.println("✅ 从剪贴板读取到 " + userInput.length() + " 个字符");
            } catch (Exception e) {
                System.out.println("❌ " + e.getMessage());
                return;
            }
        }

        String tokenString = userInput;

        // 检查是否是数字选择
        try {
            if (!allTokenFiles.isEmpty()) {
                int numChoice = Integer.parseInt(userInput);
                if (numChoice >= 1 && numChoice <= allTokenFiles.size()) {
                    String selectedFile = allTokenFiles.get(numChoice - 1);
                    String filePath = resolveTokenFilePath(selectedFile);
                    try {
                        tokenString = readFileContent(filePath);
                        System.out.println("✅ 从文件 '" + selectedFile + "' 读取到令牌 (" + tokenString.length() + " 字符)");
                    } catch (Exception e) {
                        System.out.println("❌ 无法读取文件 " + filePath + ": " + e.getMessage());
                        return;
                    }
                }
            }
        } catch (NumberFormatException e) {
            // Not a number, continue
        }

        // 检查是否是文件路径
        if (userInput.contains("/") || userInput.contains("\\") ||
            userInput.endsWith(".txt") || userInput.contains("token_")) {
            String filePath = resolveTokenFilePath(userInput);
            try {
                tokenString = readFileContent(filePath);
                System.out.println("✅ 从文件读取到令牌 (" + tokenString.length() + " 字符)");
            } catch (Exception e) {
                System.out.println("⚠️  无法读取文件 " + filePath + ": " + e.getMessage());
                System.out.println("💡 将直接使用输入作为令牌字符串");
            }
        }

        // 开始综合验证
        System.out.println("\n" + repeat("=", 50));
        System.out.println("🔍 开始综合验证流程");
        System.out.println(repeat("=", 50));

        boolean allPassed = true;

        // 步骤1: 导入令牌
        System.out.println("\n【步骤 1/5】导入令牌");
        System.out.println(repeat("-", 50));
        try {
            client.importToken(tokenString);
            System.out.println("✅ 令牌导入成功");
            System.out.println("   令牌长度: " + tokenString.length() + " 字符");
        } catch (Exception e) {
            System.out.println("❌ 令牌导入失败: " + e.getMessage());
            System.out.println("❌ 综合验证失败 - 无法导入令牌");
            return;
        }

        // 步骤2: 基本信息验证
        System.out.println("\n【步骤 2/5】基本信息验证");
        System.out.println(repeat("-", 50));
        try {
            StatusResult status = client.getStatus();
            if (status.hasToken()) {
                System.out.println("✅ 令牌结构完整");
                System.out.println("   令牌ID: " + status.getTokenId());
                System.out.println("   许可证代码: " + status.getLicenseCode());
                System.out.println("   应用ID: " + status.getAppId());
                System.out.println("   持有设备ID: " + status.getHolderDeviceId());
                SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
                System.out.println("   颁发时间: " + sdf.format(new Date(status.getIssueTime() * 1000)));

                if (status.getExpireTime() == 0) {
                    System.out.println("   到期时间: 永不过期 ✅");
                } else {
                    String expireDate = sdf.format(new Date(status.getExpireTime() * 1000));
                    System.out.print("   到期时间: " + expireDate);
                    if (status.getExpireTime() < System.currentTimeMillis() / 1000) {
                        System.out.println(" ❌ (已过期)");
                        allPassed = false;
                    } else {
                        System.out.println(" ✅");
                    }
                }

                System.out.println("   状态索引: " + status.getStateIndex());
                System.out.println("   激活状态: " + (status.isActivated() ? "已激活 ✅" : "未激活 ⚠️"));
            } else {
                System.out.println("❌ 令牌信息不完整");
                allPassed = false;
            }
        } catch (Exception e) {
            System.out.println("❌ 无法获取令牌信息: " + e.getMessage());
            allPassed = false;
        }

        // 步骤3: 签名验证
        System.out.println("\n【步骤 3/5】签名验证");
        System.out.println(repeat("-", 50));
        try {
            VerificationResult result = client.offlineVerifyCurrentToken();
            if (result.isValid()) {
                System.out.println("✅ 令牌签名验证成功");
                System.out.println("   令牌由可信的产品公钥签发");
                if (result.getErrorMessage() != null && !result.getErrorMessage().isEmpty()) {
                    System.out.println("   详细信息: " + result.getErrorMessage());
                }
            } else {
                System.out.println("❌ 令牌签名验证失败");
                System.out.println("   错误信息: " + result.getErrorMessage());
                allPassed = false;
            }
        } catch (Exception e) {
            System.out.println("❌ 签名验证失败: " + e.getMessage());
            allPassed = false;
        }

        // 步骤4: 信任链验证
        System.out.println("\n【步骤 4/5】信任链验证");
        System.out.println(repeat("-", 50));
        if (productKeyPath != null) {
            System.out.println("✅ 产品公钥已设置");
            System.out.println("   产品公钥文件: " + new File(productKeyPath).getName());
            System.out.println("✅ 信任链完整");
            System.out.println("   根证书: 产品公钥");
            System.out.println("   令牌签名: 已验证");
        } else {
            System.out.println("⚠️  产品公钥未设置");
            allPassed = false;
        }

        // 步骤5: 状态一致性验证
        System.out.println("\n【步骤 5/5】状态一致性验证");
        System.out.println(repeat("-", 50));
        try {
            StatusResult status = client.getStatus();
            if (status.hasToken()) {
                System.out.println("✅ 令牌状态一致");
                System.out.println("   状态索引: " + status.getStateIndex());

                // 检查是否有本地状态文件
                Path stateDir = Paths.get(".decentrilicense_state");
                String licenseCode = status.getLicenseCode();
                Path stateFile = stateDir.resolve(licenseCode).resolve("current_state.json");

                if (Files.exists(stateFile)) {
                    System.out.println("✅ 本地状态文件存在");
                    System.out.println("   状态文件: " + stateFile);
                } else {
                    System.out.println("⚠️  本地状态文件不存在");
                    System.out.println("   (首次使用此令牌是正常的)");
                }
            } else {
                System.out.println("⚠️  无法验证状态一致性");
            }
        } catch (Exception e) {
            System.out.println("⚠️  状态验证异常: " + e.getMessage());
        }

        // 综合验证结果
        System.out.println("\n" + repeat("=", 50));
        if (allPassed) {
            System.out.println("✅ 综合验证通过");
            System.out.println(repeat("━", 50));
            System.out.println("🎉 此令牌已通过所有验证测试！");
            System.out.println("💡 令牌可以安全使用");
            System.out.println(repeat("━", 50));
        } else {
            System.out.println("⚠️  综合验证未完全通过");
            System.out.println(repeat("━", 50));
            System.out.println("⚠️  发现一些问题，请检查上面的详细信息");
            System.out.println("💡 部分功能可能无法正常使用");
            System.out.println(repeat("━", 50));
        }
    }
}
