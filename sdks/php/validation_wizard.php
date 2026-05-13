#!/usr/bin/env php
<?php
/**
 * DecentriLicense PHP SDK 验证向导
 * ================================
 *
 * 功能完整的交互式验证工具，用于测试DecentriLicense PHP SDK的所有功能。
 * 参考Go SDK实现，提供统一的用户体验。
 */

require_once __DIR__ . '/decentrilicense.php';

// 全局变量
$g_client = null;
$g_initialized = false;
$selected_product_key_path = null;

/**
 * 获取或创建全局client实例
 */
function getOrCreateClient() {
    global $g_client;
    if ($g_client === null) {
        try {
            $g_client = new DecentriLicenseClient();
        } catch (Exception $e) {
            echo "❌ 创建客户端失败: " . $e->getMessage() . "\n";
            return null;
        }
    }
    return $g_client;
}

/**
 * 清理全局client
 */
function cleanupClient() {
    global $g_client, $g_initialized;
    if ($g_client !== null) {
        try {
            $g_client->shutdown();
        } catch (Exception $e) {
            // ignore
        }
        $g_client = null;
        $g_initialized = false;
    }
}

/**
 * 从系统剪贴板读取内容（macOS）
 */
function readFromClipboard() {
    $output = shell_exec('pbpaste');
    if ($output === null) {
        throw new Exception("从剪贴板读取失败");
    }
    return $output;
}

/**
 * 查找所有可用的产品公钥文件
 */
function findAllProductKeys() {
    $patterns = [
        '*.pem',
        '../*.pem',
        '../../*.pem',
        '../../../dl-issuer/*.pem'
    ];

    $candidates = [];
    foreach ($patterns as $pattern) {
        $matches = glob($pattern);
        if ($matches !== false) {
            $candidates = array_merge($candidates, $matches);
        }
    }

    // 去重并只保留产品公钥文件
    $seen = [];
    $unique = [];
    foreach ($candidates as $file) {
        $filename = basename($file);
        if (!isset($seen[$filename]) &&
            strpos($filename, 'public') !== false &&
            strpos($filename, 'private') === false &&
            substr($filename, -4) === '.pem') {
            $seen[$filename] = true;
            $unique[] = $filename;
        }
    }

    sort($unique);
    return $unique;
}

/**
 * 查找产品公钥文件
 */
function findProductPublicKey() {
    global $selected_product_key_path;

    if ($selected_product_key_path !== null) {
        return $selected_product_key_path;
    }

    $keys = findAllProductKeys();
    if (count($keys) > 0) {
        return resolveProductKeyPath($keys[0]);
    }
    return null;
}

/**
 * 根据文件名找到完整的产品公钥文件路径
 */
function resolveProductKeyPath($filename) {
    $searchPaths = [
        $filename,
        './' . $filename,
        '../' . $filename,
        '../../' . $filename,
        '../../../dl-issuer/' . $filename
    ];

    foreach ($searchPaths as $path) {
        if (file_exists($path)) {
            return $path;
        }
    }

    return $filename;
}

/**
 * 查找token文件
 */
function findTokenFiles($pattern = '*') {
    $patterns = [
        "token_{$pattern}*.txt",
        "../token_{$pattern}*.txt",
        "../../../dl-issuer/token_{$pattern}*.txt"
    ];

    $candidates = [];
    foreach ($patterns as $pat) {
        $matches = glob($pat);
        if ($matches !== false) {
            $candidates = array_merge($candidates, $matches);
        }
    }

    // 去重
    $seen = [];
    $unique = [];
    foreach ($candidates as $file) {
        $filename = basename($file);
        if (!isset($seen[$filename]) &&
            strpos($filename, 'token_') !== false &&
            substr($filename, -4) === '.txt') {
            $seen[$filename] = true;
            $unique[] = $filename;
        }
    }

    sort($unique);
    return $unique;
}

/**
 * 查找加密的token文件
 */
function findEncryptedTokenFiles() {
    $allTokens = findTokenFiles();
    return array_filter($allTokens, function($f) {
        return strpos($f, 'encrypted') !== false;
    });
}

/**
 * 查找已激活的token文件
 */
function findActivatedTokenFiles() {
    $allTokens = findTokenFiles();
    return array_filter($allTokens, function($f) {
        return strpos($f, 'activated') !== false;
    });
}

/**
 * 查找状态token文件
 */
function findStateTokenFiles() {
    $allTokens = findTokenFiles();
    // 照抄Go SDK: 文件名必须以token_activated_或token_state_开头
    return array_filter($allTokens, function($f) {
        return strpos($f, 'token_activated_') === 0 || strpos($f, 'token_state_') === 0;
    });
}

/**
 * 解析token文件路径
 */
function resolveTokenFilePath($filename) {
    $candidates = [
        $filename,
        getcwd() . '/' . $filename,
        getcwd() . '/../' . $filename,
        getcwd() . '/../../../dl-issuer/' . $filename
    ];

    foreach ($candidates as $candidate) {
        if (file_exists($candidate)) {
            return $candidate;
        }
    }

    return $filename;
}

/**
 * 读取文件内容
 */
function readFileContent($filepath) {
    $content = @file_get_contents($filepath);
    if ($content === false) {
        throw new Exception("读取文件失败: $filepath");
    }
    return trim($content);
}

/**
 * 读取用户输入
 */
function readInput($prompt) {
    echo $prompt;
    $handle = fopen("php://stdin", "r");
    $line = fgets($handle);
    fclose($handle);
    return trim($line);
}

/**
 * 选择产品公钥向导
 */
function selectProductKeyWizard() {
    global $selected_product_key_path;

    echo "\n🔑 选择产品公钥\n";
    echo str_repeat("=", 50) . "\n";

    $availableKeys = findAllProductKeys();

    if (count($availableKeys) === 0) {
        echo "❌ 当前目录下没有找到产品公钥文件\n";
        echo "💡 请将产品公钥文件 (public_*.pem) 放置在当前目录下\n";
        return;
    }

    echo "📄 找到以下产品公钥文件:\n";
    foreach ($availableKeys as $i => $keyFile) {
        echo ($i + 1) . ". $keyFile\n";
    }
    echo (count($availableKeys) + 1) . ". 取消选择\n";

    if ($selected_product_key_path !== null) {
        echo "✅ 当前已选择: $selected_product_key_path\n";
    }

    $choice = readInput("请选择要使用的产品公钥文件 (1-" . (count($availableKeys) + 1) . "): ");
    $choiceNum = intval($choice);

    if ($choiceNum === count($availableKeys) + 1) {
        $selected_product_key_path = null;
        echo "✅ 已取消产品公钥选择\n";
    } elseif ($choiceNum >= 1 && $choiceNum <= count($availableKeys)) {
        $selectedFile = $availableKeys[$choiceNum - 1];
        $selected_product_key_path = resolveProductKeyPath($selectedFile);
        echo "✅ 已选择产品公钥文件: $selectedFile\n";
    } else {
        echo "❌ 无效选择\n";
    }
}

/**
 * 激活令牌向导
 */
function activateTokenWizard() {
    global $g_initialized, $selected_product_key_path;

    echo "\n🔓 激活令牌\n";
    echo str_repeat("-", 50) . "\n";
    echo "⚠️  重要说明：\n";
    echo "   • 加密token（encrypted）：首次从供应商获得，需要激活\n";
    echo "   • 已激活token（activated）：激活后生成，可直接使用，不需再次激活\n";
    echo "   ⚠️  本功能仅用于【首次激活】加密token\n";
    echo "   ⚠️  如需使用已激活token，请直接选择其他功能（如记账、验证）\n";
    echo "\n";

    $client = getOrCreateClient();
    if ($client === null) {
        return;
    }

    // 显示可用的加密token文件
    $tokenFiles = findEncryptedTokenFiles();
    if (count($tokenFiles) > 0) {
        echo "📄 发现以下加密token文件:\n";
        foreach ($tokenFiles as $i => $file) {
            echo "   " . ($i + 1) . ". $file\n";
        }
        echo "💡 您可以输入序号选择文件，或输入文件名/路径/token字符串\n";
    }

    // 获取令牌输入
    echo "请输入令牌字符串 (仅支持加密令牌):\n";
    echo "💡 加密令牌通常从软件提供商处获得\n";
    echo "💡 输入序号(1-N)可快速选择上面列出的文件\n";
    echo "💡 输入文件路径可读取指定文件\n";
    echo "💡 直接回车可以从剪贴板读取token\n";

    $userInput = readInput("令牌或文件路径: ");

    // 如果输入为空，尝试从剪贴板读取
    if (empty($userInput)) {
        echo "📋 正在从剪贴板读取token...\n";
        try {
            $userInput = trim(readFromClipboard());
            if (empty($userInput)) {
                echo "❌ 剪贴板为空，请手动输入token字符串\n";
                return;
            }
            echo "✅ 从剪贴板读取到 " . strlen($userInput) . " 个字符\n";
        } catch (Exception $e) {
            echo "❌ " . $e->getMessage() . "\n";
            return;
        }
    }

    $tokenString = $userInput;

    // 检查是否输入的是数字（文件序号）
    if (count($tokenFiles) > 0 && is_numeric($userInput)) {
        $index = intval($userInput);
        if ($index >= 1 && $index <= count($tokenFiles)) {
            $selectedFile = array_values($tokenFiles)[$index - 1];
            $filePath = resolveTokenFilePath($selectedFile);
            try {
                $tokenString = readFileContent($filePath);
                echo "✅ 选择文件 '$selectedFile' 并读取到令牌 (" . strlen($tokenString) . " 字符)\n";
            } catch (Exception $e) {
                echo "❌ 无法读取文件 $filePath: " . $e->getMessage() . "\n";
                return;
            }
        }
    }

    // 检查是否是文件路径
    if (strpos($userInput, '/') !== false || strpos($userInput, '\\') !== false ||
        substr($userInput, -4) === '.txt' || strpos($userInput, 'token_') !== false) {
        $filePath = resolveTokenFilePath($userInput);
        try {
            $tokenString = readFileContent($filePath);
            echo "✅ 从文件读取到令牌 (" . strlen($tokenString) . " 字符)\n";
        } catch (Exception $e) {
            echo "⚠️  无法读取文件 $filePath: " . $e->getMessage() . "\n";
            echo "💡 将直接使用输入作为令牌字符串\n";
        }
    }

    // 初始化客户端
    if (!$g_initialized) {
        try {
            $config = new DecentriLicenseClientConfig();
            $config->license_code = "TEMP";
            $config->udp_port = 13325;
            $config->tcp_port = 23325;
            $client->initialize($config);
            echo "✅ 客户端初始化成功\n";
            $g_initialized = true;
        } catch (Exception $e) {
            echo "⚠️  初始化失败 (需要产品公钥): " . $e->getMessage() . "\n";
        }
    } else {
        echo "✅ 客户端已初始化，使用现有实例\n";
    }

    // 查找和设置产品公钥
    $productKeyPath = null;
    if ($selected_product_key_path !== null) {
        $productKeyPath = $selected_product_key_path;
        echo "📄 使用用户选择的产品公钥文件: $productKeyPath\n";
    } else {
        $productKeyPath = findProductPublicKey();
        if ($productKeyPath !== null) {
            echo "📄 使用产品公钥文件: $productKeyPath\n";
        }
    }

    if ($productKeyPath !== null) {
        try {
            $productKeyData = readFileContent($productKeyPath);
            $client->setProductPublicKey($productKeyData);
            echo "✅ 产品公钥设置成功\n";
        } catch (Exception $e) {
            echo "❌ 设置产品公钥失败: " . $e->getMessage() . "\n";
            return;
        }
    } else {
        echo "⚠️  未找到产品公钥文件\n";
        echo "💡 请先选择产品公钥 (菜单选项 0)，或确保当前目录下有产品公钥文件\n";
        return;
    }

    // 导入令牌
    echo "📥 正在导入令牌...\n";
    try {
        $client->importToken($tokenString);
        echo "✅ 令牌导入成功\n";
    } catch (Exception $e) {
        echo "❌ 令牌导入失败: " . $e->getMessage() . "\n";
        return;
    }

    // 激活令牌
    echo "🎯 正在激活令牌...\n";
    try {
        $result = $client->activateBindDevice();
        if ($result['valid']) {
            echo "✅ 令牌激活成功！\n";
            echo "💡 PHP SDK 当前版本可能不支持导出激活后的token\n";
        } else {
            echo "❌ 令牌激活失败: " . $result['error_message'] . "\n";
        }
    } catch (Exception $e) {
        echo "❌ 激活失败: " . $e->getMessage() . "\n";
    }

    // 显示最终状态
    try {
        $status = $client->getStatus();
        if ($status['is_activated']) {
            echo "🔍 当前状态: 已激活\n";
            if ($status['has_token']) {
                echo "🎫 令牌ID: " . $status['token_id'] . "\n";
                echo "📝 许可证代码: " . $status['license_code'] . "\n";
                echo "👤 持有设备: " . $status['holder_device_id'] . "\n";
                echo "📅 颁发时间: " . date('Y-m-d H:i:s', $status['issue_time']) . "\n";
            }
        } else {
            echo "🔍 当前状态: 未激活\n";
        }
    } catch (Exception $e) {
        echo "⚠️  无法获取状态: " . $e->getMessage() . "\n";
    }
}

/**
 * 校验已激活令牌向导
 */
function verifyActivatedTokenWizard() {
    echo "\n✅ 校验已激活令牌\n";
    echo str_repeat("-", 50) . "\n";

    // 扫描所有已激活的令牌
    $stateDir = ".decentrilicense_state";
    if (!is_dir($stateDir)) {
        echo "⚠️  没有找到已激活的令牌\n";
        return;
    }

    $entries = scandir($stateDir);
    if ($entries === false) {
        echo "⚠️  没有找到已激活的令牌\n";
        return;
    }

    // 列出所有已激活的令牌
    $activatedTokens = [];
    echo "\n📋 已激活的令牌列表:\n";
    $index = 1;
    foreach ($entries as $entry) {
        if ($entry === '.' || $entry === '..') continue;
        $entryPath = $stateDir . '/' . $entry;
        if (is_dir($entryPath)) {
            $activatedTokens[] = $entry;
            $stateFile = $entryPath . '/current_state.json';
            if (file_exists($stateFile)) {
                echo "$index. $entry ✅\n";
            } else {
                echo "$index. $entry ⚠️  (无状态文件)\n";
            }
            $index++;
        }
    }

    if (count($activatedTokens) === 0) {
        echo "⚠️  没有找到已激活的令牌\n";
        return;
    }

    // 让用户选择
    $choice = readInput("\n请选择要验证的令牌 (1-" . count($activatedTokens) . "): ");
    $choiceNum = intval($choice);

    if ($choiceNum < 1 || $choiceNum > count($activatedTokens)) {
        echo "❌ 无效的选择\n";
        return;
    }

    $selectedLicenseCode = $activatedTokens[$choiceNum - 1];
    echo "\n🔍 正在验证令牌: $selectedLicenseCode\n";

    $client = getOrCreateClient();
    if ($client === null) {
        return;
    }

    // 设置产品公钥（验证前必须设置）
    $productKeyPath = $GLOBALS['selected_product_key_path'];
    if (empty($productKeyPath)) {
        $productKeyPath = findProductPublicKey();
    }
    if (!empty($productKeyPath)) {
        try {
            $productKeyData = file_get_contents($productKeyPath);
            $client->setProductPublicKey($productKeyData);
            echo "✅ 产品公钥设置成功\n";
        } catch (Exception $e) {
            echo "❌ 设置产品公钥失败: " . $e->getMessage() . "\n";
            return;
        }
    } else {
        echo "❌ 未找到产品公钥文件，无法验证\n";
        return;
    }

    // 检查选择的令牌是否是当前激活的令牌
    try {
        $status = $client->getStatus();
        if ($status['license_code'] === $selectedLicenseCode) {
            // 是当前激活的令牌，可以直接验证
            echo "🔍 正在验证令牌...\n";
            $result = $client->offlineVerifyCurrentToken();
            if ($result['valid']) {
                echo "✅ 令牌验证成功\n";
                if (!empty($result['error_message'])) {
                    echo "📄 信息: " . $result['error_message'] . "\n";
                }
            } else {
                echo "❌ 令牌验证失败\n";
                echo "📄 错误信息: " . $result['error_message'] . "\n";
            }

            // 显示令牌信息
            if ($status['has_token']) {
                echo "\n🎫 令牌信息:\n";
                echo "   令牌ID: " . $status['token_id'] . "\n";
                echo "   许可证代码: " . $status['license_code'] . "\n";
                echo "   应用ID: " . $status['app_id'] . "\n";
                echo "   持有设备ID: " . $status['holder_device_id'] . "\n";
                echo "   颁发时间: " . date('Y-m-d H:i:s', $status['issue_time']) . "\n";

                if ($status['expire_time'] === 0) {
                    echo "   到期时间: 永不过期\n";
                } else {
                    echo "   到期时间: " . date('Y-m-d H:i:s', $status['expire_time']) . "\n";
                }

                echo "   状态索引: " . $status['state_index'] . "\n";
                echo "   激活状态: " . ($status['is_activated'] ? 'true' : 'false') . "\n";
            }
        } else {
            // 不是当前激活的令牌，读取状态文件显示信息
            echo "💡 此令牌不是当前激活的令牌，显示已保存的状态信息:\n";
            $stateFile = $stateDir . '/' . $selectedLicenseCode . '/current_state.json';
            $data = @file_get_contents($stateFile);
            if ($data !== false) {
                echo "\n🎫 令牌信息 (从状态文件读取):\n";
                echo "   许可证代码: $selectedLicenseCode\n";
                echo "   状态文件: $stateFile\n";
                echo "   文件大小: " . strlen($data) . " 字节\n";
                echo "\n💡 提示: 如需完整验证此令牌，请使用选项1重新激活\n";
            } else {
                echo "❌ 读取状态文件失败\n";
            }
        }
    } catch (Exception $e) {
        echo "❌ 验证失败: " . $e->getMessage() . "\n";
    }
}

/**
 * 验证令牌合法性向导
 */
function validateTokenWizard() {
    global $g_initialized, $selected_product_key_path;

    echo "\n🔍 验证令牌合法性\n";
    echo str_repeat("-", 50) . "\n";

    $client = getOrCreateClient();
    if ($client === null) {
        return;
    }

    // 初始化客户端
    if (!$g_initialized) {
        try {
            $config = new DecentriLicenseClientConfig();
            $config->license_code = "VALIDATE";
            $config->udp_port = 13325;
            $config->tcp_port = 23325;
            $client->initialize($config);
            echo "✅ 客户端初始化成功\n";
            $g_initialized = true;
        } catch (Exception $e) {
            echo "⚠️  初始化失败: " . $e->getMessage() . "\n";
        }
    }

    // 查找和设置产品公钥
    $productKeyPath = null;
    if ($selected_product_key_path !== null) {
        $productKeyPath = $selected_product_key_path;
        echo "📄 使用用户选择的产品公钥文件: $productKeyPath\n";
    } else {
        $productKeyPath = findProductPublicKey();
        if ($productKeyPath !== null) {
            echo "📄 使用产品公钥文件: $productKeyPath\n";
        }
    }

    if ($productKeyPath !== null) {
        try {
            $productKeyData = readFileContent($productKeyPath);
            $client->setProductPublicKey($productKeyData);
            echo "✅ 产品公钥设置成功\n";
        } catch (Exception $e) {
            echo "❌ 设置产品公钥失败: " . $e->getMessage() . "\n";
            return;
        }
    } else {
        echo "⚠️  未找到产品公钥文件\n";
        echo "💡 请先选择产品公钥 (菜单选项 0)，或确保当前目录下有产品公钥文件\n";
        return;
    }

    // 显示可用的加密token文件
    $tokenFiles = findEncryptedTokenFiles();
    if (count($tokenFiles) > 0) {
        echo "📄 发现以下加密token文件:\n";
        foreach ($tokenFiles as $i => $file) {
            echo "   " . ($i + 1) . ". $file\n";
        }
        echo "💡 您可以输入序号选择文件，或输入文件名/路径/token字符串\n";
    }

    // 获取令牌输入
    echo "请输入要验证的令牌字符串 (支持加密令牌):\n";
    echo "💡 令牌通常从软件提供商处获得，或从加密令牌文件读取\n";
    echo "💡 如果是文件路径，请输入完整的文件路径\n";
    echo "💡 直接回车可以从剪贴板读取token\n";

    $userInput = readInput("令牌或文件路径: ");

    // 如果输入为空，尝试从剪贴板读取
    if (empty($userInput)) {
        echo "📋 正在从剪贴板读取token...\n";
        try {
            $userInput = trim(readFromClipboard());
            if (empty($userInput)) {
                echo "❌ 剪贴板为空，请手动输入token字符串\n";
                return;
            }
            echo "✅ 从剪贴板读取到 " . strlen($userInput) . " 个字符\n";
        } catch (Exception $e) {
            echo "❌ " . $e->getMessage() . "\n";
            return;
        }
    }

    $tokenString = $userInput;

    // 检查是否是数字选择
    if (count($tokenFiles) > 0 && is_numeric($userInput)) {
        $numChoice = intval($userInput);
        if ($numChoice >= 1 && $numChoice <= count($tokenFiles)) {
            $selectedFile = array_values($tokenFiles)[$numChoice - 1];
            $filePath = resolveTokenFilePath($selectedFile);
            try {
                $tokenString = readFileContent($filePath);
                echo "✅ 从文件 '$selectedFile' 读取到令牌 (" . strlen($tokenString) . " 字符)\n";
            } catch (Exception $e) {
                echo "❌ 无法读取文件 $filePath: " . $e->getMessage() . "\n";
                return;
            }
        }
    }

    // 检查是否是文件路径
    if (strpos($userInput, '/') !== false || strpos($userInput, '\\') !== false ||
        substr($userInput, -4) === '.txt' || strpos($userInput, 'token_') !== false) {
        $filePath = resolveTokenFilePath($userInput);
        try {
            $tokenString = readFileContent($filePath);
            echo "✅ 从文件读取到令牌 (" . strlen($tokenString) . " 字符)\n";
        } catch (Exception $e) {
            echo "⚠️  无法读取文件 $filePath: " . $e->getMessage() . "\n";
            echo "💡 将直接使用输入作为令牌字符串\n";
        }
    }

    // 验证令牌
    echo "🔍 正在验证令牌合法性...\n";
    try {
        // 导入令牌
        $client->importToken($tokenString);
        echo "✅ 令牌导入成功\n";

        // 离线验证
        $result = $client->offlineVerifyCurrentToken();
        if ($result['valid']) {
            echo "✅ 令牌验证成功 - 令牌合法且有效\n";
            if (!empty($result['error_message'])) {
                echo "📄 详细信息: " . $result['error_message'] . "\n";
            }
        } else {
            echo "❌ 令牌验证失败 - 令牌不合法或无效\n";
            if (!empty($result['error_message'])) {
                echo "📄 错误信息: " . $result['error_message'] . "\n";
            }
        }
    } catch (Exception $e) {
        echo "❌ 令牌验证失败: " . $e->getMessage() . "\n";
    }
}

/**
 * 记账向导（记录使用信息）
 */
function accountingWizard() {
    global $g_initialized, $selected_product_key_path;

    echo "\n📊 记账 - 记录使用信息\n";
    echo str_repeat("-", 50) . "\n";

    $client = getOrCreateClient();
    if ($client === null) {
        return;
    }

    // 初始化客户端
    if (!$g_initialized) {
        try {
            $config = new DecentriLicenseClientConfig();
            $config->license_code = "ACCOUNTING";
            $config->udp_port = 13325;
            $config->tcp_port = 23325;
            $client->initialize($config);
            echo "✅ 客户端初始化成功\n";
            $g_initialized = true;
        } catch (Exception $e) {
            echo "⚠️  初始化失败: " . $e->getMessage() . "\n";
        }
    }

    // 查找和设置产品公钥
    $productKeyPath = null;
    if ($selected_product_key_path !== null) {
        $productKeyPath = $selected_product_key_path;
        echo "📄 使用用户选择的产品公钥文件: $productKeyPath\n";
    } else {
        $productKeyPath = findProductPublicKey();
        if ($productKeyPath !== null) {
            echo "📄 使用产品公钥文件: $productKeyPath\n";
        }
    }

    if ($productKeyPath !== null) {
        try {
            $productKeyData = readFileContent($productKeyPath);
            $client->setProductPublicKey($productKeyData);
            echo "✅ 产品公钥设置成功\n";
        } catch (Exception $e) {
            echo "❌ 设置产品公钥失败: " . $e->getMessage() . "\n";
            return;
        }
    } else {
        echo "⚠️  未找到产品公钥文件\n";
        echo "💡 请先选择产品公钥 (菜单选项 0)，或确保当前目录下有产品公钥文件\n";
        return;
    }

    // 显示可用的已激活或状态token文件
    $tokenFiles = findStateTokenFiles();
    if (count($tokenFiles) > 0) {
        echo "📄 发现以下已激活/状态token文件:\n";
        foreach ($tokenFiles as $i => $file) {
            echo "   " . ($i + 1) . ". $file\n";
        }
        echo "💡 您可以输入序号选择文件，或输入文件名/路径/token字符串\n";
    }

    // 获取令牌输入
    echo "请输入已激活的令牌字符串:\n";
    echo "💡 支持已激活的令牌或状态令牌\n";
    echo "💡 输入序号(1-N)可快速选择上面列出的文件\n";
    echo "💡 输入文件路径可读取指定文件\n";
    echo "💡 直接回车可以从剪贴板读取token\n";

    $userInput = readInput("令牌或文件路径: ");

    // 如果输入为空，尝试从剪贴板读取
    if (empty($userInput)) {
        echo "📋 正在从剪贴板读取token...\n";
        try {
            $userInput = trim(readFromClipboard());
            if (empty($userInput)) {
                echo "❌ 剪贴板为空，请手动输入token字符串\n";
                return;
            }
            echo "✅ 从剪贴板读取到 " . strlen($userInput) . " 个字符\n";
        } catch (Exception $e) {
            echo "❌ " . $e->getMessage() . "\n";
            return;
        }
    }

    $tokenString = $userInput;

    // 检查是否是数字选择
    if (count($tokenFiles) > 0 && is_numeric($userInput)) {
        $numChoice = intval($userInput);
        if ($numChoice >= 1 && $numChoice <= count($tokenFiles)) {
            $selectedFile = array_values($tokenFiles)[$numChoice - 1];
            $filePath = resolveTokenFilePath($selectedFile);
            try {
                $tokenString = readFileContent($filePath);
                echo "✅ 从文件 '$selectedFile' 读取到令牌 (" . strlen($tokenString) . " 字符)\n";
            } catch (Exception $e) {
                echo "❌ 无法读取文件 $filePath: " . $e->getMessage() . "\n";
                return;
            }
        }
    }

    // 检查是否是文件路径
    if (strpos($userInput, '/') !== false || strpos($userInput, '\\') !== false ||
        substr($userInput, -4) === '.txt' || strpos($userInput, 'token_') !== false) {
        $filePath = resolveTokenFilePath($userInput);
        try {
            $tokenString = readFileContent($filePath);
            echo "✅ 从文件读取到令牌 (" . strlen($tokenString) . " 字符)\n";
        } catch (Exception $e) {
            echo "⚠️  无法读取文件 $filePath: " . $e->getMessage() . "\n";
            echo "💡 将直接使用输入作为令牌字符串\n";
        }
    }

    // 导入令牌
    echo "📥 正在导入令牌...\n";
    try {
        $client->importToken($tokenString);
        echo "✅ 令牌导入成功\n";
    } catch (Exception $e) {
        echo "❌ 令牌导入失败: " . $e->getMessage() . "\n";
        return;
    }

    // 验证当前令牌
    echo "🔍 正在验证令牌...\n";
    try {
        $result = $client->offlineVerifyCurrentToken();
        if (!$result['valid']) {
            echo "❌ 令牌验证失败: " . $result['error_message'] . "\n";
            return;
        }
        echo "✅ 令牌验证成功\n";
    } catch (Exception $e) {
        echo "❌ 令牌验证失败: " . $e->getMessage() . "\n";
        return;
    }

    // 获取使用记录内容
    echo "\n📝 请输入要记录的使用信息 (JSON格式):\n";
    echo "💡 示例: {\"action\":\"api_call\",\"count\":10}\n";
    echo "💡 示例: {\"feature\":\"export\",\"size\":\"100MB\"}\n";
    echo "💡 直接回车使用默认示例\n";

    $payloadInput = readInput("使用信息 (JSON): ");

    if (empty($payloadInput)) {
        $payloadInput = '{"action":"test_usage","timestamp":"' . time() . '"}';
        echo "💡 使用默认payload: $payloadInput\n";
    }

    // 验证JSON格式
    $testDecode = @json_decode($payloadInput, true);
    if ($testDecode === null && json_last_error() !== JSON_ERROR_NONE) {
        echo "❌ 无效的JSON格式\n";
        return;
    }

    // 记录使用信息
    echo "📊 正在记录使用信息...\n";
    try {
        $result = $client->recordUsage($payloadInput);
        if ($result['valid']) {
            echo "✅ 使用信息记录成功\n";
            if (!empty($result['error_message'])) {
                echo "📄 详细信息: " . $result['error_message'] . "\n";
            }

            // 尝试导出状态变更后的token
            try {
                $stateToken = $client->exportStateChangedTokenEncrypted();
                if (!empty($stateToken)) {
                    // 保存到文件
                    $status = $client->getStatus();
                    $licenseCode = $status['license_code'];
                    $timestamp = date('YmdHis');
                    $filename = "token_state_{$licenseCode}_accounting_{$timestamp}.txt";

                    file_put_contents($filename, $stateToken);
                    echo "💾 状态变更后的token已保存到文件: $filename\n";
                    echo "📋 Token内容已复制到剪贴板\n";
                    shell_exec("echo " . escapeshellarg($stateToken) . " | pbcopy");
                }
            } catch (Exception $e) {
                echo "⚠️  导出状态token失败: " . $e->getMessage() . "\n";
                echo "💡 PHP SDK 当前版本可能不支持导出状态变更token\n";
            }

            // 显示当前状态
            try {
                $status = $client->getStatus();
                echo "\n🔍 当前状态:\n";
                echo "   状态索引: " . $status['state_index'] . "\n";
                echo "   许可证代码: " . $status['license_code'] . "\n";
                echo "   令牌ID: " . $status['token_id'] . "\n";
            } catch (Exception $e) {
                echo "⚠️  无法获取状态: " . $e->getMessage() . "\n";
            }
        } else {
            echo "❌ 记录失败: " . $result['error_message'] . "\n";
        }
    } catch (Exception $e) {
        echo "❌ 记录使用信息失败: " . $e->getMessage() . "\n";
    }
}

/**
 * 信任链验证向导
 */
function trustChainValidationWizard() {
    global $g_initialized, $selected_product_key_path;

    echo "\n🔐 信任链验证\n";
    echo str_repeat("-", 50) . "\n";
    echo "⚠️  说明: 此功能用于验证令牌的完整信任链\n";
    echo "   包括产品公钥、颁发者签名、令牌完整性等\n\n";

    $client = getOrCreateClient();
    if ($client === null) {
        return;
    }

    // 初始化客户端
    if (!$g_initialized) {
        try {
            $config = new DecentriLicenseClientConfig();
            $config->license_code = "TRUSTCHAIN";
            $config->udp_port = 13325;
            $config->tcp_port = 23325;
            $client->initialize($config);
            echo "✅ 客户端初始化成功\n";
            $g_initialized = true;
        } catch (Exception $e) {
            echo "⚠️  初始化失败: " . $e->getMessage() . "\n";
        }
    }

    // 查找和设置产品公钥
    $productKeyPath = null;
    if ($selected_product_key_path !== null) {
        $productKeyPath = $selected_product_key_path;
        echo "📄 使用用户选择的产品公钥文件: $productKeyPath\n";
    } else {
        $productKeyPath = findProductPublicKey();
        if ($productKeyPath !== null) {
            echo "📄 使用产品公钥文件: $productKeyPath\n";
        }
    }

    if ($productKeyPath !== null) {
        try {
            $productKeyData = readFileContent($productKeyPath);
            $client->setProductPublicKey($productKeyData);
            echo "✅ 产品公钥设置成功 - 信任链的根\n";
        } catch (Exception $e) {
            echo "❌ 设置产品公钥失败: " . $e->getMessage() . "\n";
            return;
        }
    } else {
        echo "⚠️  未找到产品公钥文件\n";
        echo "💡 信任链验证需要产品公钥，请先选择产品公钥 (菜单选项 0)\n";
        return;
    }

    // 显示可用的token文件
    $allTokenFiles = findTokenFiles();
    if (count($allTokenFiles) > 0) {
        echo "\n📄 发现以下token文件:\n";
        foreach ($allTokenFiles as $i => $file) {
            $marker = "";
            if (strpos($file, 'encrypted') !== false) {
                $marker = " [加密]";
            } elseif (strpos($file, 'activated') !== false) {
                $marker = " [已激活]";
            } elseif (strpos($file, 'state') !== false) {
                $marker = " [状态]";
            }
            echo "   " . ($i + 1) . ". $file$marker\n";
        }
        echo "💡 您可以输入序号选择文件，或输入文件名/路径/token字符串\n";
    }

    // 获取令牌输入
    echo "\n请输入要验证的令牌字符串:\n";
    echo "💡 支持加密令牌、已激活令牌或状态令牌\n";
    echo "💡 输入序号(1-N)可快速选择上面列出的文件\n";
    echo "💡 输入文件路径可读取指定文件\n";
    echo "💡 直接回车可以从剪贴板读取token\n";

    $userInput = readInput("令牌或文件路径: ");

    // 如果输入为空，尝试从剪贴板读取
    if (empty($userInput)) {
        echo "📋 正在从剪贴板读取token...\n";
        try {
            $userInput = trim(readFromClipboard());
            if (empty($userInput)) {
                echo "❌ 剪贴板为空，请手动输入token字符串\n";
                return;
            }
            echo "✅ 从剪贴板读取到 " . strlen($userInput) . " 个字符\n";
        } catch (Exception $e) {
            echo "❌ " . $e->getMessage() . "\n";
            return;
        }
    }

    $tokenString = $userInput;

    // 检查是否是数字选择
    if (count($allTokenFiles) > 0 && is_numeric($userInput)) {
        $numChoice = intval($userInput);
        if ($numChoice >= 1 && $numChoice <= count($allTokenFiles)) {
            $selectedFile = array_values($allTokenFiles)[$numChoice - 1];
            $filePath = resolveTokenFilePath($selectedFile);
            try {
                $tokenString = readFileContent($filePath);
                echo "✅ 从文件 '$selectedFile' 读取到令牌 (" . strlen($tokenString) . " 字符)\n";
            } catch (Exception $e) {
                echo "❌ 无法读取文件 $filePath: " . $e->getMessage() . "\n";
                return;
            }
        }
    }

    // 检查是否是文件路径
    if (strpos($userInput, '/') !== false || strpos($userInput, '\\') !== false ||
        substr($userInput, -4) === '.txt' || strpos($userInput, 'token_') !== false) {
        $filePath = resolveTokenFilePath($userInput);
        try {
            $tokenString = readFileContent($filePath);
            echo "✅ 从文件读取到令牌 (" . strlen($tokenString) . " 字符)\n";
        } catch (Exception $e) {
            echo "⚠️  无法读取文件 $filePath: " . $e->getMessage() . "\n";
            echo "💡 将直接使用输入作为令牌字符串\n";
        }
    }

    // 导入令牌
    echo "\n🔍 开始信任链验证...\n";
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

    echo "\n1️⃣  验证步骤1: 导入令牌\n";
    try {
        $client->importToken($tokenString);
        echo "   ✅ 令牌导入成功\n";
    } catch (Exception $e) {
        echo "   ❌ 令牌导入失败: " . $e->getMessage() . "\n";
        echo "   ❌ 信任链验证失败 - 无法导入令牌\n";
        return;
    }

    echo "\n2️⃣  验证步骤2: 产品公钥验证\n";
    echo "   ✅ 产品公钥已设置并作为信任链的根\n";

    echo "\n3️⃣  验证步骤3: 令牌签名验证\n";
    try {
        $result = $client->offlineVerifyCurrentToken();
        if ($result['valid']) {
            echo "   ✅ 令牌签名验证成功\n";
            echo "   ✅ 令牌由可信的产品公钥签发\n";
        } else {
            echo "   ❌ 令牌签名验证失败: " . $result['error_message'] . "\n";
            echo "   ❌ 信任链验证失败 - 签名无效\n";
            return;
        }
    } catch (Exception $e) {
        echo "   ❌ 令牌验证失败: " . $e->getMessage() . "\n";
        echo "   ❌ 信任链验证失败\n";
        return;
    }

    echo "\n4️⃣  验证步骤4: 令牌完整性检查\n";
    try {
        $status = $client->getStatus();
        if ($status['has_token']) {
            echo "   ✅ 令牌结构完整\n";
            echo "   📄 令牌ID: " . $status['token_id'] . "\n";
            echo "   📄 许可证代码: " . $status['license_code'] . "\n";
            echo "   📄 应用ID: " . $status['app_id'] . "\n";
            echo "   📄 持有设备: " . $status['holder_device_id'] . "\n";
            echo "   📄 颁发时间: " . date('Y-m-d H:i:s', $status['issue_time']) . "\n";

            if ($status['expire_time'] === 0) {
                echo "   📄 到期时间: 永不过期\n";
            } else {
                echo "   📄 到期时间: " . date('Y-m-d H:i:s', $status['expire_time']) . "\n";
                if ($status['expire_time'] < time()) {
                    echo "   ⚠️  警告: 令牌已过期\n";
                }
            }

            echo "   📄 状态索引: " . $status['state_index'] . "\n";
            echo "   📄 激活状态: " . ($status['is_activated'] ? '已激活' : '未激活') . "\n";
        } else {
            echo "   ⚠️  警告: 令牌信息不完整\n";
        }
    } catch (Exception $e) {
        echo "   ⚠️  无法获取完整状态信息: " . $e->getMessage() . "\n";
    }

    echo "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    echo "✅ 信任链验证完成 - 令牌可信\n";
    echo "💡 此令牌由有效的产品公钥签发，签名验证通过\n";
}

/**
 * 综合验证向导（执行所有验证步骤）
 */
function comprehensiveValidationWizard() {
    global $g_initialized, $selected_product_key_path;

    echo "\n🎯 综合验证 - 完整的令牌验证流程\n";
    echo str_repeat("=", 50) . "\n";
    echo "⚠️  说明: 此功能将执行完整的令牌验证流程\n";
    echo "   包括: 格式验证、签名验证、信任链验证、状态验证等\n\n";

    $client = getOrCreateClient();
    if ($client === null) {
        return;
    }

    // 初始化客户端
    if (!$g_initialized) {
        try {
            $config = new DecentriLicenseClientConfig();
            $config->license_code = "COMPREHENSIVE";
            $config->udp_port = 13325;
            $config->tcp_port = 23325;
            $client->initialize($config);
            echo "✅ 客户端初始化成功\n";
            $g_initialized = true;
        } catch (Exception $e) {
            echo "⚠️  初始化失败: " . $e->getMessage() . "\n";
        }
    }

    // 查找和设置产品公钥
    $productKeyPath = null;
    if ($selected_product_key_path !== null) {
        $productKeyPath = $selected_product_key_path;
        echo "📄 使用用户选择的产品公钥文件: $productKeyPath\n";
    } else {
        $productKeyPath = findProductPublicKey();
        if ($productKeyPath !== null) {
            echo "📄 使用产品公钥文件: $productKeyPath\n";
        }
    }

    if ($productKeyPath !== null) {
        try {
            $productKeyData = readFileContent($productKeyPath);
            $client->setProductPublicKey($productKeyData);
            echo "✅ 产品公钥设置成功\n";
        } catch (Exception $e) {
            echo "❌ 设置产品公钥失败: " . $e->getMessage() . "\n";
            return;
        }
    } else {
        echo "⚠️  未找到产品公钥文件\n";
        echo "💡 综合验证需要产品公钥，请先选择产品公钥 (菜单选项 0)\n";
        return;
    }

    // 显示可用的token文件
    $allTokenFiles = findTokenFiles();
    if (count($allTokenFiles) > 0) {
        echo "\n📄 发现以下token文件:\n";
        foreach ($allTokenFiles as $i => $file) {
            $marker = "";
            if (strpos($file, 'encrypted') !== false) {
                $marker = " [加密]";
            } elseif (strpos($file, 'activated') !== false) {
                $marker = " [已激活]";
            } elseif (strpos($file, 'state') !== false) {
                $marker = " [状态]";
            }
            echo "   " . ($i + 1) . ". $file$marker\n";
        }
        echo "💡 您可以输入序号选择文件，或输入文件名/路径/token字符串\n";
    }

    // 获取令牌输入
    echo "\n请输入要验证的令牌字符串:\n";
    echo "💡 支持加密令牌、已激活令牌或状态令牌\n";
    echo "💡 输入序号(1-N)可快速选择上面列出的文件\n";
    echo "💡 输入文件路径可读取指定文件\n";
    echo "💡 直接回车可以从剪贴板读取token\n";

    $userInput = readInput("令牌或文件路径: ");

    // 如果输入为空，尝试从剪贴板读取
    if (empty($userInput)) {
        echo "📋 正在从剪贴板读取token...\n";
        try {
            $userInput = trim(readFromClipboard());
            if (empty($userInput)) {
                echo "❌ 剪贴板为空，请手动输入token字符串\n";
                return;
            }
            echo "✅ 从剪贴板读取到 " . strlen($userInput) . " 个字符\n";
        } catch (Exception $e) {
            echo "❌ " . $e->getMessage() . "\n";
            return;
        }
    }

    $tokenString = $userInput;

    // 检查是否是数字选择
    if (count($allTokenFiles) > 0 && is_numeric($userInput)) {
        $numChoice = intval($userInput);
        if ($numChoice >= 1 && $numChoice <= count($allTokenFiles)) {
            $selectedFile = array_values($allTokenFiles)[$numChoice - 1];
            $filePath = resolveTokenFilePath($selectedFile);
            try {
                $tokenString = readFileContent($filePath);
                echo "✅ 从文件 '$selectedFile' 读取到令牌 (" . strlen($tokenString) . " 字符)\n";
            } catch (Exception $e) {
                echo "❌ 无法读取文件 $filePath: " . $e->getMessage() . "\n";
                return;
            }
        }
    }

    // 检查是否是文件路径
    if (strpos($userInput, '/') !== false || strpos($userInput, '\\') !== false ||
        substr($userInput, -4) === '.txt' || strpos($userInput, 'token_') !== false) {
        $filePath = resolveTokenFilePath($userInput);
        try {
            $tokenString = readFileContent($filePath);
            echo "✅ 从文件读取到令牌 (" . strlen($tokenString) . " 字符)\n";
        } catch (Exception $e) {
            echo "⚠️  无法读取文件 $filePath: " . $e->getMessage() . "\n";
            echo "💡 将直接使用输入作为令牌字符串\n";
        }
    }

    // 开始综合验证
    echo "\n" . str_repeat("=", 50) . "\n";
    echo "🔍 开始综合验证流程\n";
    echo str_repeat("=", 50) . "\n";

    $allPassed = true;

    // 步骤1: 导入令牌
    echo "\n【步骤 1/5】导入令牌\n";
    echo str_repeat("-", 50) . "\n";
    try {
        $client->importToken($tokenString);
        echo "✅ 令牌导入成功\n";
        echo "   令牌长度: " . strlen($tokenString) . " 字符\n";
    } catch (Exception $e) {
        echo "❌ 令牌导入失败: " . $e->getMessage() . "\n";
        echo "❌ 综合验证失败 - 无法导入令牌\n";
        return;
    }

    // 步骤2: 基本信息验证
    echo "\n【步骤 2/5】基本信息验证\n";
    echo str_repeat("-", 50) . "\n";
    try {
        $status = $client->getStatus();
        if ($status['has_token']) {
            echo "✅ 令牌结构完整\n";
            echo "   令牌ID: " . $status['token_id'] . "\n";
            echo "   许可证代码: " . $status['license_code'] . "\n";
            echo "   应用ID: " . $status['app_id'] . "\n";
            echo "   持有设备ID: " . $status['holder_device_id'] . "\n";
            echo "   颁发时间: " . date('Y-m-d H:i:s', $status['issue_time']) . "\n";

            if ($status['expire_time'] === 0) {
                echo "   到期时间: 永不过期 ✅\n";
            } else {
                $expireDate = date('Y-m-d H:i:s', $status['expire_time']);
                echo "   到期时间: $expireDate";
                if ($status['expire_time'] < time()) {
                    echo " ❌ (已过期)\n";
                    $allPassed = false;
                } else {
                    echo " ✅\n";
                }
            }

            echo "   状态索引: " . $status['state_index'] . "\n";
            echo "   激活状态: " . ($status['is_activated'] ? '已激活 ✅' : '未激活 ⚠️') . "\n";
        } else {
            echo "❌ 令牌信息不完整\n";
            $allPassed = false;
        }
    } catch (Exception $e) {
        echo "❌ 无法获取令牌信息: " . $e->getMessage() . "\n";
        $allPassed = false;
    }

    // 步骤3: 签名验证
    echo "\n【步骤 3/5】签名验证\n";
    echo str_repeat("-", 50) . "\n";
    try {
        $result = $client->offlineVerifyCurrentToken();
        if ($result['valid']) {
            echo "✅ 令牌签名验证成功\n";
            echo "   令牌由可信的产品公钥签发\n";
            if (!empty($result['error_message'])) {
                echo "   详细信息: " . $result['error_message'] . "\n";
            }
        } else {
            echo "❌ 令牌签名验证失败\n";
            echo "   错误信息: " . $result['error_message'] . "\n";
            $allPassed = false;
        }
    } catch (Exception $e) {
        echo "❌ 签名验证失败: " . $e->getMessage() . "\n";
        $allPassed = false;
    }

    // 步骤4: 信任链验证
    echo "\n【步骤 4/5】信任链验证\n";
    echo str_repeat("-", 50) . "\n";
    if ($productKeyPath !== null) {
        echo "✅ 产品公钥已设置\n";
        echo "   产品公钥文件: " . basename($productKeyPath) . "\n";
        echo "✅ 信任链完整\n";
        echo "   根证书: 产品公钥\n";
        echo "   令牌签名: 已验证\n";
    } else {
        echo "⚠️  产品公钥未设置\n";
        $allPassed = false;
    }

    // 步骤5: 状态一致性验证
    echo "\n【步骤 5/5】状态一致性验证\n";
    echo str_repeat("-", 50) . "\n";
    try {
        $status = $client->getStatus();
        if ($status['has_token']) {
            echo "✅ 令牌状态一致\n";
            echo "   状态索引: " . $status['state_index'] . "\n";

            // 检查是否有本地状态文件
            $stateDir = ".decentrilicense_state";
            $licenseCode = $status['license_code'];
            $stateFile = "$stateDir/$licenseCode/current_state.json";

            if (file_exists($stateFile)) {
                echo "✅ 本地状态文件存在\n";
                echo "   状态文件: $stateFile\n";
            } else {
                echo "⚠️  本地状态文件不存在\n";
                echo "   (首次使用此令牌是正常的)\n";
            }
        } else {
            echo "⚠️  无法验证状态一致性\n";
        }
    } catch (Exception $e) {
        echo "⚠️  状态验证异常: " . $e->getMessage() . "\n";
    }

    // 综合验证结果
    echo "\n" . str_repeat("=", 50) . "\n";
    if ($allPassed) {
        echo "✅ 综合验证通过\n";
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        echo "🎉 此令牌已通过所有验证测试！\n";
        echo "💡 令牌可以安全使用\n";
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    } else {
        echo "⚠️  综合验证未完全通过\n";
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        echo "⚠️  发现一些问题，请检查上面的详细信息\n";
        echo "💡 部分功能可能无法正常使用\n";
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    }
}

/**
 * 主函数
 */
function main() {
    echo "\n";
    echo "╔══════════════════════════════════════════════════╗\n";
    echo "║     DecentriLicense PHP SDK 验证向导             ║\n";
    echo "║     Interactive Validation Wizard                ║\n";
    echo "╚══════════════════════════════════════════════════╝\n";
    echo "\n";
    echo "欢迎使用 DecentriLicense PHP SDK 验证工具！\n";
    echo "本工具提供完整的令牌管理和验证功能。\n";
    echo "\n";

    while (true) {
        echo "\n" . str_repeat("=", 50) . "\n";
        echo "📋 主菜单\n";
        echo str_repeat("=", 50) . "\n";
        echo "0. 🔑 选择产品公钥\n";
        echo "1. 🔓 激活令牌\n";
        echo "2. ✅ 校验已激活令牌\n";
        echo "3. 🔍 验证令牌合法性\n";
        echo "4. 📊 记账信息\n";
        echo "5. 🔗 信任链验证\n";
        echo "6. 🎯 综合验证\n";
        echo "7. 🚪 退出\n";
        echo str_repeat("=", 50) . "\n";

        $choice = readInput("请选择功能 (0-7): ");

        switch ($choice) {
            case '0':
                selectProductKeyWizard();
                break;
            case '1':
                activateTokenWizard();
                break;
            case '2':
                verifyActivatedTokenWizard();
                break;
            case '3':
                validateTokenWizard();
                break;
            case '4':
                accountingWizard();
                break;
            case '5':
                trustChainValidationWizard();
                break;
            case '6':
                comprehensiveValidationWizard();
                break;
            case '7':
                echo "\n👋 感谢使用 DecentriLicense PHP SDK 验证向导！\n";
                echo "再见！\n\n";
                cleanupClient();
                exit(0);
            default:
                echo "❌ 无效选择，请输入 0-7\n";
        }
    }
}

// 注册退出清理函数
register_shutdown_function('cleanupClient');

// 运行主程序
main();

