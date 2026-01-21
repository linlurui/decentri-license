#!/usr/bin/env node

/**
 * DecentriLicense Node.js SDK 验证向导
 * ================================
 *
 * 功能完整的交互式验证工具，用于测试DecentriLicense Node.js SDK的所有功能。
 * 参考Go SDK实现，提供统一的用户体验。
 */

const fs = require('fs');
const path = require('path');
const readline = require('readline');
const { execSync } = require('child_process');

// 全局client实例
let g_client = null;
let g_initialized = false;
let selected_product_key_path = null;

// 创建 readline 接口
const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

// 异步输入函数
function question(prompt) {
    return new Promise((resolve) => {
        rl.question(prompt, resolve);
    });
}

/**
 * 获取或创建全局client实例
 */
function getOrCreateClient() {
    if (g_client === null) {
        try {
            const DecentriLicenseClient = require('./index.js');
            g_client = new DecentriLicenseClient();
        } catch (e) {
            console.log(`❌ 创建客户端失败: ${e.message}`);
            return null;
        }
    }
    return g_client;
}

/**
 * 清理全局client
 */
function cleanupClient() {
    if (g_client !== null) {
        try {
            g_client.shutdown();
        } catch (e) {
            // ignore
        }
        g_client = null;
        g_initialized = false;
    }
}

/**
 * 从系统剪贴板读取内容（macOS）
 */
function readFromClipboard() {
    try {
        return execSync('pbpaste', { encoding: 'utf8' });
    } catch (e) {
        throw new Error(`从剪贴板读取失败: ${e.message}`);
    }
}

/**
 * 简单的glob实现（用于查找文件）
 */
function simpleGlob(pattern) {
    const results = [];
    const parts = pattern.split('/');
    let currentDir = '.';

    // 处理绝对路径
    if (pattern.startsWith('/')) {
        currentDir = '/';
    }

    // 处理相对路径
    for (let i = 0; i < parts.length - 1; i++) {
        if (parts[i] === '..') {
            currentDir = path.join(currentDir, '..');
        } else if (parts[i] && parts[i] !== '.') {
            currentDir = path.join(currentDir, parts[i]);
        }
    }

    const filePattern = parts[parts.length - 1];

    try {
        if (!fs.existsSync(currentDir)) {
            return results;
        }

        const files = fs.readdirSync(currentDir);
        for (const file of files) {
            if (filePattern === '*' ||
                filePattern.startsWith('*') && file.endsWith(filePattern.substring(1)) ||
                file === filePattern) {
                results.push(path.join(currentDir, file));
            }
        }
    } catch (e) {
        // ignore errors
    }

    return results;
}

/**
 * 查找所有可用的产品公钥文件
 */
function findAllProductKeys() {
    const patterns = [
        '*.pem',
        '../*.pem',
        '../../*.pem',
        '../../../dl-issuer/*.pem'
    ];

    const candidates = [];
    for (const pattern of patterns) {
        try {
            const matches = simpleGlob(pattern);
            candidates.push(...matches);
        } catch (e) {
            // ignore
        }
    }

    // 去重并只保留产品公钥文件
    const seen = new Set();
    const unique = [];
    for (const file of candidates) {
        const filename = path.basename(file);
        if (!seen.has(filename) &&
            filename.includes('public') &&
            !filename.includes('private') &&
            filename.endsWith('.pem')) {
            seen.add(filename);
            unique.push(filename);
        }
    }

    return unique.sort();
}

/**
 * 查找产品公钥文件
 */
function findProductPublicKey() {
    if (selected_product_key_path) {
        return selected_product_key_path;
    }

    const keys = findAllProductKeys();
    if (keys.length > 0) {
        return resolveProductKeyPath(keys[0]);
    }
    return null;
}

/**
 * 根据文件名找到完整的产品公钥文件路径
 */
function resolveProductKeyPath(filename) {
    const search_paths = [
        filename,
        path.join('.', filename),
        path.join('..', filename),
        path.join('../..', filename),
        path.join('../../../dl-issuer', filename)
    ];

    for (const p of search_paths) {
        if (fs.existsSync(p)) {
            return p;
        }
    }

    return filename;
}

/**
 * 查找token文件
 */
function findTokenFiles(pattern = '*') {
    const patterns = [
        `token_${pattern}.txt`,
        `../token_${pattern}.txt`,
        `../../../dl-issuer/token_${pattern}.txt`
    ];

    const candidates = [];
    for (const pat of patterns) {
        try {
            // 使用通配符查找
            const dir = path.dirname(pat);
            const filePattern = path.basename(pat);

            if (fs.existsSync(dir)) {
                const files = fs.readdirSync(dir);
                for (const file of files) {
                    if (file.startsWith('token_') && file.endsWith('.txt')) {
                        candidates.push(path.join(dir, file));
                    }
                }
            }
        } catch (e) {
            // ignore
        }
    }

    // 去重
    const seen = new Set();
    const unique = [];
    for (const file of candidates) {
        const filename = path.basename(file);
        if (!seen.has(filename) && filename.includes('token_') && filename.endsWith('.txt')) {
            seen.add(filename);
            unique.push(filename);
        }
    }

    return unique.sort();
}

/**
 * 查找加密的token文件
 */
function findEncryptedTokenFiles() {
    const allTokens = findTokenFiles();
    return allTokens.filter(f => f.includes('encrypted'));
}

/**
 * 查找已激活的token文件
 */
function findActivatedTokenFiles() {
    const allTokens = findTokenFiles();
    return allTokens.filter(f => f.includes('activated'));
}

/**
 * 查找状态token文件（用于记账信息）
 */
function findStateTokenFiles() {
    const candidates = [];

    // 查找已激活和状态变更的token文件（照抄Go SDK）
    const patterns = [
        { dir: '.', prefix: 'token_activated_', suffix: '.txt' },
        { dir: '.', prefix: 'token_state_', suffix: '.txt' },
        { dir: '..', prefix: 'token_activated_', suffix: '.txt' },
        { dir: '..', prefix: 'token_state_', suffix: '.txt' },
        { dir: '../../../dl-issuer', prefix: 'token_activated_', suffix: '.txt' },
        { dir: '../../../dl-issuer', prefix: 'token_state_', suffix: '.txt' }
    ];

    for (const pattern of patterns) {
        try {
            if (fs.existsSync(pattern.dir)) {
                const files = fs.readdirSync(pattern.dir);
                for (const file of files) {
                    if (file.startsWith(pattern.prefix) && file.endsWith(pattern.suffix)) {
                        candidates.push(file);
                    }
                }
            }
        } catch (e) {
            // ignore
        }
    }

    // 去重并只保留文件名
    const seen = new Set();
    const unique = [];
    for (const file of candidates) {
        const filename = path.basename(file);
        if (!seen.has(filename)) {
            seen.add(filename);
            unique.push(filename);
        }
    }

    return unique;
}

/**
 * 解析token文件路径
 */
function resolveTokenFilePath(filename) {
    const candidates = [
        filename,
        path.join(process.cwd(), filename),
        path.join(process.cwd(), '..', filename),
        path.join(process.cwd(), '../../../dl-issuer', filename)
    ];

    for (const candidate of candidates) {
        if (fs.existsSync(candidate)) {
            return candidate;
        }
    }

    return filename;
}

/**
 * 读取文件内容
 */
function readFile(filepath) {
    try {
        return fs.readFileSync(filepath, 'utf8').trim();
    } catch (e) {
        throw new Error(`读取文件失败: ${e.message}`);
    }
}

/**
 * 选择产品公钥向导
 */
async function selectProductKeyWizard() {
    console.log("\n🔑 选择产品公钥");
    console.log("=".repeat(50));

    const availableKeys = findAllProductKeys();

    if (availableKeys.length === 0) {
        console.log("❌ 当前目录下没有找到产品公钥文件");
        console.log("💡 请将产品公钥文件 (public_*.pem) 放置在当前目录下");
        return;
    }

    console.log("📄 找到以下产品公钥文件:");
    for (let i = 0; i < availableKeys.length; i++) {
        console.log(`${i + 1}. ${availableKeys[i]}`);
    }
    console.log(`${availableKeys.length + 1}. 取消选择`);

    if (selected_product_key_path) {
        console.log(`✅ 当前已选择: ${selected_product_key_path}`);
    }

    const choice = await question(`请选择要使用的产品公钥文件 (1-${availableKeys.length + 1}): `);
    const choiceNum = parseInt(choice.trim());

    if (choiceNum === availableKeys.length + 1) {
        selected_product_key_path = null;
        console.log("✅ 已取消产品公钥选择");
    } else if (choiceNum >= 1 && choiceNum <= availableKeys.length) {
        const selectedFile = availableKeys[choiceNum - 1];
        selected_product_key_path = resolveProductKeyPath(selectedFile);
        console.log(`✅ 已选择产品公钥文件: ${selectedFile}`);
    } else {
        console.log("❌ 无效选择");
    }
}

/**
 * 激活令牌向导
 */
async function activateTokenWizard() {
    console.log("\n🔓 激活令牌");
    console.log("-".repeat(50));
    console.log("⚠️  重要说明：");
    console.log("   • 加密token（encrypted）：首次从供应商获得，需要激活");
    console.log("   • 已激活token（activated）：激活后生成，可直接使用，不需再次激活");
    console.log("   ⚠️  本功能仅用于【首次激活】加密token");
    console.log("   ⚠️  如需使用已激活token，请直接选择其他功能（如记账、验证）");
    console.log();

    const client = getOrCreateClient();
    if (client === null) {
        return;
    }

    // 显示可用的加密token文件
    const tokenFiles = findEncryptedTokenFiles();
    if (tokenFiles.length > 0) {
        console.log("📄 发现以下加密token文件:");
        for (let i = 0; i < tokenFiles.length; i++) {
            console.log(`   ${i + 1}. ${tokenFiles[i]}`);
        }
        console.log("💡 您可以输入序号选择文件，或输入文件名/路径/token字符串");
    }

    // 获取令牌输入
    console.log("请输入令牌字符串 (仅支持加密令牌):");
    console.log("💡 加密令牌通常从软件提供商处获得");
    console.log("💡 输入序号(1-N)可快速选择上面列出的文件");
    console.log("💡 输入文件路径可读取指定文件");
    console.log("💡 直接回车可以从剪贴板读取token");

    let userInput = (await question("令牌或文件路径: ")).trim();

    // 如果输入为空，尝试从剪贴板读取
    if (!userInput) {
        console.log("📋 正在从剪贴板读取token...");
        try {
            userInput = readFromClipboard().trim();
            if (!userInput) {
                console.log("❌ 剪贴板为空，请手动输入token字符串");
                return;
            }
            console.log(`✅ 从剪贴板读取到 ${userInput.length} 个字符`);
        } catch (e) {
            console.log(`❌ ${e.message}`);
            return;
        }
    }

    let tokenString = userInput;

    // 检查是否输入的是数字（文件序号）
    if (tokenFiles.length > 0) {
        const index = parseInt(userInput);
        if (!isNaN(index) && index >= 1 && index <= tokenFiles.length) {
            const selectedFile = tokenFiles[index - 1];
            const filePath = resolveTokenFilePath(selectedFile);
            try {
                tokenString = readFile(filePath);
                console.log(`✅ 选择文件 '${selectedFile}' 并读取到令牌 (${tokenString.length} 字符)`);
            } catch (e) {
                console.log(`❌ 无法读取文件 ${filePath}: ${e.message}`);
                return;
            }
        }
    }

    // 检查是否是文件路径
    if (userInput.includes('/') || userInput.includes('\\') ||
        userInput.endsWith('.txt') || userInput.includes('token_')) {
        const filePath = resolveTokenFilePath(userInput);
        try {
            tokenString = readFile(filePath);
            console.log(`✅ 从文件读取到令牌 (${tokenString.length} 字符)`);
        } catch (e) {
            console.log(`⚠️  无法读取文件 ${filePath}: ${e.message}`);
            console.log("💡 将直接使用输入作为令牌字符串");
        }
    }

    // 初始化客户端
    if (!g_initialized) {
        try {
            client.initialize({
                licenseCode: "TEMP",
                udpPort: 13325,
                tcpPort: 23325,
                registryServerUrl: ""
            });
            console.log("✅ 客户端初始化成功");
            g_initialized = true;
        } catch (e) {
            console.log(`⚠️  初始化失败 (需要产品公钥): ${e.message}`);
        }
    } else {
        console.log("✅ 客户端已初始化，使用现有实例");
    }

    // 查找和设置产品公钥
    let productKeyPath;
    if (selected_product_key_path) {
        productKeyPath = selected_product_key_path;
        console.log(`📄 使用用户选择的产品公钥文件: ${productKeyPath}`);
    } else {
        productKeyPath = findProductPublicKey();
        if (productKeyPath) {
            console.log(`📄 使用产品公钥文件: ${productKeyPath}`);
        }
    }

    if (productKeyPath) {
        try {
            const productKeyData = readFile(productKeyPath);
            client.setProductPublicKey(productKeyData);
            console.log("✅ 产品公钥设置成功");
        } catch (e) {
            console.log(`❌ 设置产品公钥失败: ${e.message}`);
            return;
        }
    } else {
        console.log("⚠️  未找到产品公钥文件");
        console.log("💡 请先选择产品公钥 (菜单选项 0)，或确保当前目录下有产品公钥文件");
        return;
    }

    // 导入令牌
    console.log("📥 正在导入令牌...");
    try {
        client.importToken(tokenString);
        console.log("✅ 令牌导入成功");
    } catch (e) {
        console.log(`❌ 令牌导入失败: ${e.message}`);
        return;
    }

    // 激活令牌
    console.log("🎯 正在激活令牌...");
    try {
        const result = client.activateBindDevice();
        if (result.valid) {
            console.log("✅ 令牌激活成功！");

            // 导出激活后的新token
            try {
                const activatedToken = client.exportActivatedTokenEncrypted();
                if (activatedToken) {
                    console.log("\n📦 激活后的新Token（加密）:");
                    console.log(`   长度: ${activatedToken.length} 字符`);
                    if (activatedToken.length > 100) {
                        console.log(`   前缀: ${activatedToken.substring(0, 100)}...`);
                    } else {
                        console.log(`   内容: ${activatedToken}`);
                    }

                    // 保存激活后的token到文件
                    const status = client.getStatus();
                    if (status.licenseCode) {
                        const timestamp = new Date().toISOString().replace(/[-:T]/g, '').substring(0, 14);
                        const filename = `token_activated_${status.licenseCode}_${timestamp}.txt`;
                        fs.writeFileSync(filename, activatedToken);
                        const absPath = path.resolve(filename);
                        console.log(`\n💾 已保存到文件: ${absPath}`);
                        console.log("   💡 此token包含设备绑定信息，可传递给下一个设备使用");
                    }
                }
            } catch (e) {
                console.log(`⚠️  导出激活token失败: ${e.message}`);
            }
        } else {
            console.log(`❌ 令牌激活失败: ${result.errorMessage || 'Unknown error'}`);
        }
    } catch (e) {
        console.log(`❌ 激活失败: ${e.message}`);
    }

    // 显示最终状态
    try {
        const activated = client.isActivated();
        if (activated) {
            console.log("🔍 当前状态: 已激活");
            const status = client.getStatus();
            if (status.hasToken) {
                console.log(`🎫 令牌ID: ${status.tokenId}`);
                console.log(`📝 许可证代码: ${status.licenseCode}`);
                console.log(`👤 持有设备: ${status.holderDeviceId}`);
                const issueTime = new Date(status.issueTime * 1000);
                console.log(`📅 颁发时间: ${issueTime.toLocaleString('zh-CN')}`);
            }
        } else {
            console.log("🔍 当前状态: 未激活");
        }
    } catch (e) {
        console.log(`⚠️  无法获取状态: ${e.message}`);
    }
}

/**
 * 校验已激活令牌向导
 */
async function verifyActivatedTokenWizard() {
    console.log("\n✅ 校验已激活令牌");
    console.log("-".repeat(50));

    // 扫描所有已激活的令牌
    const stateDir = ".decentrilicense_state";
    if (!fs.existsSync(stateDir)) {
        console.log("⚠️  没有找到已激活的令牌");
        return;
    }

    const entries = fs.readdirSync(stateDir);

    // 列出所有已激活的令牌
    const activatedTokens = [];
    console.log("\n📋 已激活的令牌列表:");
    for (let i = 0; i < entries.length; i++) {
        const entryPath = path.join(stateDir, entries[i]);
        if (fs.statSync(entryPath).isDirectory()) {
            activatedTokens.push(entries[i]);
            const stateFile = path.join(entryPath, "current_state.json");
            if (fs.existsSync(stateFile)) {
                console.log(`${i + 1}. ${entries[i]} ✅`);
            } else {
                console.log(`${i + 1}. ${entries[i]} ⚠️  (无状态文件)`);
            }
        }
    }

    if (activatedTokens.length === 0) {
        console.log("⚠️  没有找到已激活的令牌");
        return;
    }

    // 让用户选择
    const choice = await question(`\n请选择要验证的令牌 (1-${activatedTokens.length}): `);
    const index = parseInt(choice.trim());

    if (isNaN(index) || index < 1 || index > activatedTokens.length) {
        console.log("❌ 无效的选择");
        return;
    }

    const selectedLicenseCode = activatedTokens[index - 1];
    console.log(`\n🔍 正在验证令牌: ${selectedLicenseCode}`);

    const client = getOrCreateClient();
    if (client === null) {
        return;
    }

    // 检查选择的令牌是否是当前激活的令牌
    try {
        const status = client.getStatus();
        if (status.licenseCode === selectedLicenseCode) {
            // 是当前激活的令牌，可以直接验证
            console.log("🔍 正在验证令牌...");
            const result = client.offlineVerify();
            if (result.valid) {
                console.log("✅ 令牌验证成功");
                if (result.error) {
                    console.log(`📄 信息: ${result.error}`);
                }
            } else {
                console.log("❌ 令牌验证失败");
                console.log(`📄 错误信息: ${result.error || 'Unknown error'}`);
            }

            // 显示令牌信息
            if (status.hasToken) {
                console.log("\n🎫 令牌信息:");
                console.log(`   令牌ID: ${status.tokenId}`);
                console.log(`   许可证代码: ${status.licenseCode}`);
                console.log(`   应用ID: ${status.appId}`);
                console.log(`   持有设备ID: ${status.holderDeviceId}`);

                const issueTime = new Date(status.issueTime * 1000);
                console.log(`   颁发时间: ${issueTime.toLocaleString('zh-CN')}`);

                if (status.expireTime === 0) {
                    console.log("   到期时间: 永不过期");
                } else {
                    const expireTime = new Date(status.expireTime * 1000);
                    console.log(`   到期时间: ${expireTime.toLocaleString('zh-CN')}`);
                }

                console.log(`   状态索引: ${status.stateIndex}`);
                console.log(`   激活状态: ${status.isActivated}`);
            }
        } else {
            // 不是当前激活的令牌，读取状态文件显示信息
            console.log("💡 此令牌不是当前激活的令牌，显示已保存的状态信息:");
            const stateFile = path.join(stateDir, selectedLicenseCode, "current_state.json");
            const data = readFile(stateFile);
            console.log("\n🎫 令牌信息 (从状态文件读取):");
            console.log(`   许可证代码: ${selectedLicenseCode}`);
            console.log(`   状态文件: ${stateFile}`);
            console.log(`   文件大小: ${data.length} 字节`);
            console.log("\n💡 提示: 如需完整验证此令牌，请使用选项1重新激活");
        }
    } catch (e) {
        console.log(`❌ 验证失败: ${e.message}`);
    }
}

/**
 * 验证令牌合法性向导
 */
async function validateTokenWizard() {
    console.log("\n🔍 验证令牌合法性");
    console.log("-".repeat(50));

    const client = getOrCreateClient();
    if (client === null) {
        return;
    }

    // 初始化客户端
    if (!g_initialized) {
        try {
            client.initialize({
                licenseCode: "VALIDATE",
                udpPort: 13325,
                tcpPort: 23325,
                registryServerUrl: ""
            });
            console.log("✅ 客户端初始化成功");
            g_initialized = true;
        } catch (e) {
            console.log(`⚠️  初始化失败: ${e.message}`);
        }
    }

    // 查找和设置产品公钥
    let productKeyPath;
    if (selected_product_key_path) {
        productKeyPath = selected_product_key_path;
        console.log(`📄 使用用户选择的产品公钥文件: ${productKeyPath}`);
    } else {
        productKeyPath = findProductPublicKey();
        if (productKeyPath) {
            console.log(`📄 使用产品公钥文件: ${productKeyPath}`);
        }
    }

    if (productKeyPath) {
        try {
            const productKeyData = readFile(productKeyPath);
            client.setProductPublicKey(productKeyData);
            console.log("✅ 产品公钥设置成功");
        } catch (e) {
            console.log(`❌ 设置产品公钥失败: ${e.message}`);
            return;
        }
    } else {
        console.log("⚠️  未找到产品公钥文件");
        console.log("💡 请先选择产品公钥 (菜单选项 0)，或确保当前目录下有产品公钥文件");
        return;
    }

    // 显示可用的加密token文件
    const tokenFiles = findEncryptedTokenFiles();
    if (tokenFiles.length > 0) {
        console.log("📄 发现以下加密token文件:");
        for (let i = 0; i < tokenFiles.length; i++) {
            console.log(`   ${i + 1}. ${tokenFiles[i]}`);
        }
        console.log("💡 您可以输入序号选择文件，或输入文件名/路径/token字符串");
    }

    // 获取令牌输入
    console.log("请输入要验证的令牌字符串 (支持加密令牌):");
    console.log("💡 令牌通常从软件提供商处获得，或从加密令牌文件读取");
    console.log("💡 如果是文件路径，请输入完整的文件路径");
    console.log("💡 直接回车可以从剪贴板读取token");

    let userInput = (await question("令牌或文件路径: ")).trim();

    // 如果输入为空，尝试从剪贴板读取
    if (!userInput) {
        console.log("📋 正在从剪贴板读取token...");
        try {
            userInput = readFromClipboard().trim();
            if (!userInput) {
                console.log("❌ 剪贴板为空，请手动输入token字符串");
                return;
            }
            console.log(`✅ 从剪贴板读取到 ${userInput.length} 个字符`);
        } catch (e) {
            console.log(`❌ ${e.message}`);
            return;
        }
    }

    let tokenString = userInput;

    // 检查是否是数字选择
    if (tokenFiles.length > 0) {
        const numChoice = parseInt(userInput);
        if (!isNaN(numChoice) && numChoice >= 1 && numChoice <= tokenFiles.length) {
            const selectedFile = tokenFiles[numChoice - 1];
            const filePath = resolveTokenFilePath(selectedFile);
            try {
                tokenString = readFile(filePath);
                console.log(`✅ 从文件 '${selectedFile}' 读取到令牌 (${tokenString.length} 字符)`);
            } catch (e) {
                console.log(`❌ 无法读取文件 ${filePath}: ${e.message}`);
                return;
            }
        }
    }

    // 检查是否是文件路径
    if (userInput.includes('/') || userInput.includes('\\') ||
        userInput.endsWith('.txt') || userInput.includes('token_')) {
        const filePath = resolveTokenFilePath(userInput);
        try {
            tokenString = readFile(filePath);
            console.log(`✅ 从文件读取到令牌 (${tokenString.length} 字符)`);
        } catch (e) {
            console.log(`⚠️  无法读取文件 ${filePath}: ${e.message}`);
            console.log("💡 将直接使用输入作为令牌字符串");
        }
    }

    // 验证令牌
    console.log("🔍 正在验证令牌合法性...");
    try {
        // 导入令牌
        client.importToken(tokenString);
        console.log("✅ 令牌导入成功");

        // 离线验证
        const result = client.offlineVerify();
        if (result.valid) {
            console.log("✅ 令牌验证成功 - 令牌合法且有效");
            if (result.error) {
                console.log(`📄 详细信息: ${result.error}`);
            }
        } else {
            console.log("❌ 令牌验证失败 - 令牌不合法或无效");
            if (result.error) {
                console.log(`📄 错误信息: ${result.error}`);
            }
        }
    } catch (e) {
        console.log(`❌ 令牌验证失败: ${e.message}`);
    }
}

/**
 * 记账信息向导
 */
async function accountingWizard() {
    console.log("\n📊 记账信息");
    console.log("-".repeat(50));

    const client = getOrCreateClient();
    if (client === null) {
        return;
    }

    // 显示可用的状态token文件
    const tokenFiles = findStateTokenFiles();

    // 检查激活状态
    let activated = false;
    try {
        activated = g_initialized ? client.isActivated() : false;
    } catch (e) {
        activated = false;
    }

    // 显示令牌选择选项
    console.log("\n💡 请选择令牌来源:");
    if (activated) {
        console.log("0. 使用当前激活的令牌");
    }

    if (tokenFiles.length > 0) {
        console.log("\n📄 或从以下文件加载令牌:");
        for (let i = 0; i < tokenFiles.length; i++) {
            console.log(`${i + 1}. ${tokenFiles[i]}`);
        }
    }

    if (!activated && tokenFiles.length === 0) {
        console.log("❌ 当前没有激活的令牌，也没有找到可用的token文件");
        console.log("💡 请先使用选项1激活令牌");
        return;
    }

    let prompt = "请选择 (0";
    if (tokenFiles.length > 0) {
        prompt += `-${tokenFiles.length}`;
    }
    prompt += "): ";

    const tokenChoice = await question(prompt);
    const tokenChoiceNum = parseInt(tokenChoice.trim());

    if (isNaN(tokenChoiceNum) || tokenChoiceNum < 0 || tokenChoiceNum > tokenFiles.length) {
        console.log("❌ 无效的选择");
        return;
    }

    // 如果选择从文件加载
    if (tokenChoiceNum > 0) {
        const selectedFile = tokenFiles[tokenChoiceNum - 1];
        const filePath = resolveTokenFilePath(selectedFile);

        console.log(`📂 正在从文件加载令牌: ${selectedFile}`);

        let tokenData;
        try {
            tokenData = readFile(filePath);
            console.log(`✅ 读取到令牌 (${tokenData.length} 字符)`);
        } catch (e) {
            console.log(`❌ 读取文件失败: ${e.message}`);
            return;
        }

        // 初始化客户端
        if (!g_initialized) {
            try {
                client.initialize({
                    licenseCode: "ACCOUNTING",
                    udpPort: 13325,
                    tcpPort: 23325,
                    registryServerUrl: ""
                });
                g_initialized = true;
            } catch (e) {
                console.log(`⚠️  初始化失败: ${e.message}`);
            }
        }

        // 设置产品公钥
        let productKeyPath;
        if (selected_product_key_path) {
            productKeyPath = selected_product_key_path;
        } else {
            productKeyPath = findProductPublicKey();
        }

        if (productKeyPath) {
            try {
                const productKeyData = readFile(productKeyPath);
                client.setProductPublicKey(productKeyData);
                console.log("✅ 产品公钥设置成功");
            } catch (e) {
                console.log(`❌ 设置产品公钥失败: ${e.message}`);
                return;
            }
        }

        // 导入令牌
        console.log("📥 正在导入令牌...");
        try {
            client.importToken(tokenData);
            console.log("✅ 令牌导入成功");
        } catch (e) {
            console.log(`❌ 令牌导入失败: ${e.message}`);
            return;
        }

        // 检查令牌类型
        const isAlreadyActivated = selectedFile.includes('activated') || selectedFile.includes('state');

        if (isAlreadyActivated) {
            console.log("💡 检测到已激活令牌");
            console.log("🔄 正在恢复激活状态...");
        } else {
            console.log("🎯 正在首次激活令牌...");
        }

        // 调用ActivateBindDevice恢复/设置激活状态
        try {
            const result = client.activateBindDevice();
            if (!result.valid) {
                console.log(`❌ 激活失败: ${result.errorMessage || 'Unknown error'}`);
                return;
            }

            if (isAlreadyActivated) {
                console.log("✅ 激活状态已恢复（token未改变）");
            } else {
                console.log("✅ 首次激活成功");
            }
        } catch (e) {
            console.log(`❌ 激活失败: ${e.message}`);
            return;
        }
    }

    // 显示当前令牌信息
    try {
        const status = client.getStatus();
        if (status.hasToken) {
            console.log("\n📋 当前令牌信息:");
            console.log(`   许可证代码: ${status.licenseCode}`);
            console.log(`   应用ID: ${status.appId}`);
            console.log(`   当前状态索引: ${status.stateIndex}`);
            console.log(`   令牌ID: ${status.tokenId}`);
        } else {
            console.log("⚠️  无法获取令牌信息");
            return;
        }
    } catch (e) {
        console.log(`⚠️  无法获取令牌信息: ${e.message}`);
        return;
    }

    // 提供记账选项
    console.log("\n💡 请选择记账方式:");
    console.log("1. 快速测试记账（使用默认测试数据）");
    console.log("2. 记录业务操作（向导式输入）");

    const choice = await question("\n请选择 (1-2): ");

    let action, params;

    if (choice.trim() === "1") {
        // 快速测试
        action = "api_call";
        params = {
            function: "test_function",
            result: "success"
        };
        console.log(`💡 使用测试数据: action=${action}, params=${JSON.stringify(params)}`);
    } else if (choice.trim() === "2") {
        // 业务操作记账
        console.log("\n📝 usage_chain 结构说明:");
        console.log("┌─────────────────────────────────────────────────────────┐");
        console.log("│ 字段名      │ 说明           │ 填写方式              │");
        console.log("├─────────────────────────────────────────────────────────┤");
        console.log("│ seq         │ 序列号         │ ✅ 系统自动填充       │");
        console.log("│ time        │ 时间戳         │ ✅ 系统自动填充       │");
        console.log("│ action      │ 操作类型       │ 👉 需要您输入         │");
        console.log("│ params      │ 操作参数       │ 👉 需要您输入         │");
        console.log("│ hash_prev   │ 前状态哈希     │ ✅ 系统自动填充       │");
        console.log("│ signature   │ 数字签名       │ ✅ 系统自动填充       │");
        console.log("└─────────────────────────────────────────────────────────┘");

        // 输入action
        console.log("\n👉 第1步: 输入操作类型 (action)");
        console.log("   常用操作类型:");
        console.log("   • api_call      - API调用");
        console.log("   • feature_usage - 功能使用");
        console.log("   • save_file     - 保存文件");
        console.log("   • export_data   - 导出数据");
        action = (await question("\n请输入操作类型: ")).trim();
        if (!action) {
            console.log("❌ 操作类型不能为空");
            return;
        }

        // 输入params
        console.log("\n👉 第2步: 输入操作参数 (params)");
        console.log("   params 是一个JSON对象，包含操作的具体参数");
        console.log("   输入格式: key=value (每行一个)");
        console.log("   示例:");
        console.log("   • function=process_image");
        console.log("   • file_name=report.pdf");
        console.log("   • size=1024");
        console.log("   输入空行结束输入");

        params = {};
        while (true) {
            const line = (await question("参数 (key=value 或直接回车结束): ")).trim();
            if (!line) break;

            const parts = line.split('=');
            if (parts.length === 2) {
                const key = parts[0].trim();
                const value = parts[1].trim();
                params[key] = value;
            } else {
                console.log("⚠️  格式错误,请使用 key=value 格式");
            }
        }

        if (Object.keys(params).length === 0) {
            console.log("⚠️  未输入任何参数,将使用空参数对象");
            params = {};
        }
    } else {
        console.log("❌ 无效的选择");
        return;
    }

    // 构建记账数据
    const usageChainEntry = {
        action: action,
        params: params
    };

    const accountingData = JSON.stringify(usageChainEntry);
    console.log(`\n📝 记账数据 (业务字段): ${accountingData}`);
    console.log("   (系统字段 seq, time, hash_prev, signature 将由SDK自动添加)");

    // 记录使用情况
    console.log("📝 正在记录使用情况...");
    try {
        const result = client.recordUsage(accountingData);
        if (result.valid) {
            console.log("✅ 记账成功");
            console.log(`📄 响应: ${result.errorMessage || ''}`);

            // 导出状态变更后的新token
            try {
                const stateToken = client.exportStateChangedTokenEncrypted();
                if (stateToken) {
                    console.log("\n📦 状态变更后的新Token（加密）:");
                    console.log(`   长度: ${stateToken.length} 字符`);
                    if (stateToken.length > 100) {
                        console.log(`   前缀: ${stateToken.substring(0, 100)}...`);
                    } else {
                        console.log(`   内容: ${stateToken}`);
                    }

                    // 保存状态变更后的token到文件
                    const status = client.getStatus();
                    if (status.licenseCode) {
                        const timestamp = new Date().toISOString().replace(/[-:T]/g, '').substring(0, 14);
                        const filename = `token_state_${status.licenseCode}_idx${status.stateIndex}_${timestamp}.txt`;
                        fs.writeFileSync(filename, stateToken);
                        const absPath = path.resolve(filename);
                        console.log(`\n💾 已保存到文件: ${absPath}`);
                        console.log("   💡 此token包含最新状态链，可传递给下一个设备使用");
                    }
                }
            } catch (e) {
                console.log(`⚠️  导出状态变更token失败: ${e.message}`);
            }
        } else {
            console.log("❌ 记账失败");
            console.log(`📄 错误信息: ${result.errorMessage || 'Unknown error'}`);
        }
    } catch (e) {
        console.log(`❌ 记账失败: ${e.message}`);
    }
}

/**
 * 信任链验证向导
 */
async function trustChainValidationWizard() {
    console.log("\n🔗 信任链验证");
    console.log("=".repeat(50));
    console.log("💡 信任链验证检查加密签名的完整性：根密钥 → 产品公钥 → 令牌签名 → 设备绑定");
    console.log();

    const client = getOrCreateClient();
    if (client === null) {
        return;
    }

    // 显示可用的token文件
    const tokenFiles = findStateTokenFiles();

    // 检查激活状态
    let activated = false;
    try {
        activated = g_initialized ? client.isActivated() : false;
    } catch (e) {
        activated = false;
    }

    // 显示令牌选择选项
    console.log("\n💡 请选择令牌来源:");
    if (activated) {
        console.log("0. 使用当前激活的令牌");
    }

    if (tokenFiles.length > 0) {
        console.log("\n📄 或从以下文件加载令牌:");
        for (let i = 0; i < tokenFiles.length; i++) {
            console.log(`${i + 1}. ${tokenFiles[i]}`);
        }
    }

    if (!activated && tokenFiles.length === 0) {
        console.log("❌ 当前没有激活的令牌，也没有找到可用的token文件");
        console.log("💡 请先使用选项1激活令牌");
        return;
    }

    let prompt = "请选择 (0";
    if (tokenFiles.length > 0) {
        prompt += `-${tokenFiles.length}`;
    }
    prompt += "): ";

    const tokenChoice = await question(prompt);
    const tokenChoiceNum = parseInt(tokenChoice.trim());

    if (isNaN(tokenChoiceNum) || tokenChoiceNum < 0 || tokenChoiceNum > tokenFiles.length) {
        console.log("❌ 无效的选择");
        return;
    }

    // 如果选择从文件加载
    if (tokenChoiceNum > 0) {
        const selectedFile = tokenFiles[tokenChoiceNum - 1];
        const filePath = resolveTokenFilePath(selectedFile);

        console.log(`📂 正在从文件加载令牌: ${selectedFile}`);

        let tokenData;
        try {
            tokenData = readFile(filePath);
            console.log(`✅ 读取到令牌 (${tokenData.length} 字符)`);
        } catch (e) {
            console.log(`❌ 读取文件失败: ${e.message}`);
            return;
        }

        // 初始化客户端
        if (!g_initialized) {
            try {
                client.initialize({
                    licenseCode: "TRUST_CHAIN",
                    udpPort: 13325,
                    tcpPort: 23325,
                    registryServerUrl: ""
                });
                g_initialized = true;
            } catch (e) {
                console.log(`⚠️  初始化失败: ${e.message}`);
            }
        }

        // 设置产品公钥
        let productKeyPath;
        if (selected_product_key_path) {
            productKeyPath = selected_product_key_path;
        } else {
            productKeyPath = findProductPublicKey();
        }

        if (productKeyPath) {
            try {
                const productKeyData = readFile(productKeyPath);
                client.setProductPublicKey(productKeyData);
                console.log("✅ 产品公钥设置成功");
            } catch (e) {
                console.log(`❌ 设置产品公钥失败: ${e.message}`);
                return;
            }
        }

        // 导入令牌
        console.log("📥 正在导入令牌...");
        try {
            client.importToken(tokenData);
            console.log("✅ 令牌导入成功");
        } catch (e) {
            console.log(`❌ 令牌导入失败: ${e.message}`);
            return;
        }

        // 检查令牌类型
        const isAlreadyActivated = selectedFile.includes('activated') || selectedFile.includes('state');

        if (isAlreadyActivated) {
            console.log("💡 检测到已激活令牌");
            console.log("🔄 正在恢复激活状态...");
        } else {
            console.log("🎯 正在首次激活令牌...");
        }

        // 调用ActivateBindDevice恢复/设置激活状态
        try {
            const result = client.activateBindDevice();
            if (!result.valid) {
                console.log(`❌ 激活失败: ${result.errorMessage || 'Unknown error'}`);
                return;
            }

            if (isAlreadyActivated) {
                console.log("✅ 激活状态已恢复（token未改变）");
            } else {
                console.log("✅ 首次激活成功");
            }
        } catch (e) {
            console.log(`❌ 激活失败: ${e.message}`);
            return;
        }
    }

    console.log("📋 开始验证信任链...");
    console.log();

    let checksPassed = 0;
    const totalChecks = 4;

    // 1. 基础令牌签名验证
    console.log("🔍 [1/4] 验证令牌签名（根密钥 → 产品公钥 → 令牌）");
    try {
        const result = client.offlineVerify();
        if (result.valid) {
            console.log("   ✅ 通过: 令牌签名有效，信任链完整");
            checksPassed++;
        } else {
            console.log(`   ❌ 失败: ${result.error || 'Unknown error'}`);
        }
    } catch (e) {
        console.log(`   ❌ 失败: ${e.message}`);
    }
    console.log();

    // 2. 检查设备状态
    console.log("🔍 [2/4] 验证设备状态");
    try {
        const state = client.getDeviceState();
        console.log(`   ✅ 通过: 设备状态正常 (状态: ${state})`);
        checksPassed++;
    } catch (e) {
        console.log(`   ⚠️  警告: 无法获取设备状态 - ${e.message}`);
    }
    console.log();

    // 3. 检查令牌持有者匹配
    console.log("🔍 [3/4] 验证令牌持有者与当前设备匹配");
    try {
        const token = client.getCurrentToken();
        if (token) {
            const deviceId = client.getDeviceId();
            if (token.holderDeviceId === deviceId) {
                console.log("   ✅ 通过: 令牌持有者与当前设备匹配");
                console.log(`   📱 设备ID: ${deviceId}`);
                checksPassed++;
            } else {
                console.log("   ⚠️  不匹配: 令牌持有者与当前设备不一致");
                console.log(`   📱 当前设备ID: ${deviceId}`);
                console.log(`   🎫 令牌持有者ID: ${token.holderDeviceId}`);
                console.log("   💡 这可能表示令牌是从其他设备导入的");
            }
        } else {
            console.log("   ⚠️  警告: 无法获取令牌信息");
        }
    } catch (e) {
        console.log(`   ⚠️  警告: 无法获取设备ID - ${e.message}`);
    }
    console.log();

    // 4. 显示令牌详细信息
    console.log("🔍 [4/4] 检查令牌详细信息");
    try {
        const status = client.getStatus();
        if (status.hasToken) {
            console.log("   ✅ 通过: 令牌信息完整");
            console.log(`   🎫 令牌ID: ${status.tokenId}`);
            console.log(`   📝 许可证代码: ${status.licenseCode}`);
            console.log(`   📱 应用ID: ${status.appId}`);
            const issueTime = new Date(status.issueTime * 1000);
            console.log(`   📅 颁发时间: ${issueTime.toLocaleString('zh-CN')}`);
            if (status.expireTime === 0) {
                console.log("   ⏰ 到期时间: 永不过期");
            } else {
                const expireTime = new Date(status.expireTime * 1000);
                console.log(`   ⏰ 到期时间: ${expireTime.toLocaleString('zh-CN')}`);
            }
            checksPassed++;
        } else {
            console.log("   ⚠️  警告: 令牌信息不完整");
        }
    } catch (e) {
        console.log(`   ⚠️  警告: 无法获取状态信息 - ${e.message}`);
    }
    console.log();

    // 结果汇总
    console.log("━".repeat(50));
    console.log(`📊 验证结果: ${checksPassed}/${totalChecks} 项检查通过`);
    if (checksPassed === totalChecks) {
        console.log("🎉 信任链验证完全通过！令牌可信且安全");
    } else if (checksPassed >= 2) {
        console.log("⚠️  部分检查通过，令牌基本可用但存在警告");
    } else {
        console.log("❌ 多项检查失败，请检查令牌和设备状态");
    }
    console.log("━".repeat(50));
}

/**
 * 综合验证向导
 */
async function comprehensiveValidationWizard() {
    console.log("\n🎯 综合验证");
    console.log("-".repeat(50));

    const client = getOrCreateClient();
    if (client === null) {
        return;
    }

    // 显示可用的token文件
    const tokenFiles = findStateTokenFiles();

    // 检查激活状态
    let activated = false;
    try {
        activated = g_initialized ? client.isActivated() : false;
    } catch (e) {
        activated = false;
    }

    // 显示令牌选择选项
    console.log("\n💡 请选择令牌来源:");
    if (activated) {
        console.log("0. 使用当前激活的令牌");
    }

    if (tokenFiles.length > 0) {
        console.log("\n📄 或从以下文件加载令牌:");
        for (let i = 0; i < tokenFiles.length; i++) {
            console.log(`${i + 1}. ${tokenFiles[i]}`);
        }
    }

    if (!activated && tokenFiles.length === 0) {
        console.log("❌ 当前没有激活的令牌，也没有找到可用的token文件");
        console.log("💡 请先使用选项1激活令牌");
        return;
    }

    let prompt = "请选择 (0";
    if (tokenFiles.length > 0) {
        prompt += `-${tokenFiles.length}`;
    }
    prompt += "): ";

    const tokenChoice = await question(prompt);
    const tokenChoiceNum = parseInt(tokenChoice.trim());

    if (isNaN(tokenChoiceNum) || tokenChoiceNum < 0 || tokenChoiceNum > tokenFiles.length) {
        console.log("❌ 无效的选择");
        return;
    }

    // 如果选择从文件加载
    if (tokenChoiceNum > 0) {
        const selectedFile = tokenFiles[tokenChoiceNum - 1];
        const filePath = resolveTokenFilePath(selectedFile);

        console.log(`📂 正在从文件加载令牌: ${selectedFile}`);

        let tokenData;
        try {
            tokenData = readFile(filePath);
            console.log(`✅ 读取到令牌 (${tokenData.length} 字符)`);
        } catch (e) {
            console.log(`❌ 读取文件失败: ${e.message}`);
            return;
        }

        // 初始化客户端
        if (!g_initialized) {
            try {
                client.initialize({
                    licenseCode: "COMPREHENSIVE",
                    udpPort: 13325,
                    tcpPort: 23325,
                    registryServerUrl: ""
                });
                g_initialized = true;
            } catch (e) {
                console.log(`⚠️  初始化失败: ${e.message}`);
            }
        }

        // 设置产品公钥
        let productKeyPath;
        if (selected_product_key_path) {
            productKeyPath = selected_product_key_path;
        } else {
            productKeyPath = findProductPublicKey();
        }

        if (productKeyPath) {
            try {
                const productKeyData = readFile(productKeyPath);
                client.setProductPublicKey(productKeyData);
                console.log("✅ 产品公钥设置成功");
            } catch (e) {
                console.log(`❌ 设置产品公钥失败: ${e.message}`);
                return;
            }
        }

        // 导入令牌
        console.log("📥 正在导入令牌...");
        try {
            client.importToken(tokenData);
            console.log("✅ 令牌导入成功");
        } catch (e) {
            console.log(`❌ 令牌导入失败: ${e.message}`);
            return;
        }

        // 检查令牌类型
        const isAlreadyActivated = selectedFile.includes('activated') || selectedFile.includes('state');

        if (isAlreadyActivated) {
            console.log("💡 检测到已激活令牌");
            console.log("🔄 正在恢复激活状态...");
        } else {
            console.log("🎯 正在首次激活令牌...");
        }

        // 调用ActivateBindDevice恢复/设置激活状态
        try {
            const result = client.activateBindDevice();
            if (!result.valid) {
                console.log(`❌ 激活失败: ${result.errorMessage || 'Unknown error'}`);
                return;
            }

            if (isAlreadyActivated) {
                console.log("✅ 激活状态已恢复（token未改变）");
            } else {
                console.log("✅ 首次激活成功");
            }
        } catch (e) {
            console.log(`❌ 激活失败: ${e.message}`);
            return;
        }
    }

    console.log("📋 执行综合验证流程...");
    let checkCount = 0;
    let passCount = 0;

    // 1. 检查激活状态
    checkCount++;
    try {
        activated = client.isActivated();
        passCount++;
        if (activated) {
            console.log(`✅ 检查${checkCount}通过: 许可证已激活`);
        } else {
            console.log(`⚠️  检查${checkCount}: 许可证未激活`);
        }
    } catch (e) {
        console.log(`❌ 检查${checkCount}失败: 激活状态查询失败 - ${e.message}`);
    }

    // 2. 验证当前令牌
    if (activated) {
        checkCount++;
        try {
            const result = client.offlineVerify();
            if (result.valid) {
                passCount++;
                console.log(`✅ 检查${checkCount}通过: 令牌验证成功`);
            } else {
                console.log(`❌ 检查${checkCount}失败: 令牌验证失败 - ${result.error || 'Unknown'}`);
            }
        } catch (e) {
            console.log(`❌ 检查${checkCount}失败: 令牌验证失败 - ${e.message}`);
        }
    }

    // 3. 检查设备状态
    checkCount++;
    try {
        const state = client.getDeviceState();
        passCount++;
        console.log(`✅ 检查${checkCount}通过: 设备状态正常 (状态: ${state})`);
    } catch (e) {
        console.log(`❌ 检查${checkCount}失败: 设备状态查询失败 - ${e.message}`);
    }

    // 4. 检查令牌信息
    checkCount++;
    try {
        const token = client.getCurrentToken();
        if (token) {
            passCount++;
            const tokenId = token.tokenId;
            if (tokenId && tokenId.length >= 16) {
                console.log(`✅ 检查${checkCount}通过: 令牌信息完整 (ID: ${tokenId.substring(0, 16)}...)`);
            } else if (tokenId && tokenId.length > 0) {
                console.log(`✅ 检查${checkCount}通过: 令牌信息完整 (ID: ${tokenId})`);
            } else {
                console.log(`✅ 检查${checkCount}通过: 令牌对象存在`);
            }
        } else {
            if (activated) {
                console.log(`❌ 检查${checkCount}失败: 令牌信息查询失败`);
            } else {
                console.log(`⚠️  检查${checkCount}: 无令牌信息 (未激活)`);
            }
        }
    } catch (e) {
        console.log(`❌ 检查${checkCount}失败: 令牌信息查询失败 - ${e.message}`);
    }

    // 5. 测试记账功能
    if (activated) {
        checkCount++;
        const testData = JSON.stringify({ action: "comprehensive_test", timestamp: 1234567890 });
        try {
            const result = client.recordUsage(testData);
            if (result.valid) {
                passCount++;
                console.log(`✅ 检查${checkCount}通过: 记账功能正常`);

                // 导出状态变更后的新token
                try {
                    const stateToken = client.exportStateChangedTokenEncrypted();
                    if (stateToken) {
                        console.log("   📦 状态变更后的新Token已生成");
                        console.log(`   Token长度: ${stateToken.length} 字符`);

                        // 保存状态变更后的token到文件
                        const status = client.getStatus();
                        if (status.licenseCode) {
                            const timestamp = new Date().toISOString().replace(/[-:T]/g, '').substring(0, 14);
                            const filename = `token_state_${status.licenseCode}_idx${status.stateIndex}_${timestamp}.txt`;
                            fs.writeFileSync(filename, stateToken);
                            const absPath = path.resolve(filename);
                            console.log(`   💾 已保存到: ${absPath}`);
                        }
                    }
                } catch (e) {
                    console.log(`   ⚠️  导出状态变更token失败: ${e.message}`);
                }
            } else {
                console.log(`❌ 检查${checkCount}失败: 记账功能异常 - ${result.errorMessage || 'Unknown'}`);
            }
        } catch (e) {
            console.log(`❌ 检查${checkCount}失败: 记账功能测试失败 - ${e.message}`);
        }
    }

    // 结果汇总
    console.log("\n📊 综合验证结果:");
    console.log(`   总检查项: ${checkCount}`);
    console.log(`   通过项目: ${passCount}`);
    console.log(`   成功率: ${(passCount / checkCount * 100).toFixed(1)}%`);

    if (passCount === checkCount) {
        console.log("🎉 所有检查均通过！系统运行正常");
    } else if (passCount >= Math.floor(checkCount / 2)) {
        console.log("⚠️  大部分检查通过，系统基本正常");
    } else {
        console.log("❌ 多项检查失败，请检查系统配置");
    }
}

/**
 * 主程序
 */
async function main() {
    while (true) {
        console.log("\n" + "=".repeat(50));
        console.log("DecentriLicense Node.js SDK 验证向导");
        console.log("=".repeat(50));
        console.log();
        console.log("请选择要执行的操作:");
        console.log("0. 🔑 选择产品公钥");
        console.log("1. 🔓 激活令牌");
        console.log("2. ✅ 校验已激活令牌");
        console.log("3. 🔍 验证令牌合法性");
        console.log("4. 📊 记账信息");
        console.log("5. 🔗 信任链验证");
        console.log("6. 🎯 综合验证");
        console.log("7. 🚪 退出");

        try {
            const choice = await question("请输入选项 (0-7): ");
            console.log();

            switch (choice.trim()) {
                case "0":
                    await selectProductKeyWizard();
                    break;
                case "1":
                    await activateTokenWizard();
                    break;
                case "2":
                    await verifyActivatedTokenWizard();
                    break;
                case "3":
                    await validateTokenWizard();
                    break;
                case "4":
                    await accountingWizard();
                    break;
                case "5":
                    await trustChainValidationWizard();
                    break;
                case "6":
                    await comprehensiveValidationWizard();
                    break;
                case "7":
                    console.log("感谢使用 DecentriLicense Node.js SDK 验证向导!");
                    cleanupClient();
                    rl.close();
                    return;
                default:
                    console.log("❌ 无效选项，请重新选择");
            }
        } catch (error) {
            console.error(`❌ 操作失败: ${error.message}`);
        }

        console.log("\n" + "-".repeat(50));
    }
}

// 运行主程序
if (require.main === module) {
    main().catch((err) => {
        console.error(err);
        cleanupClient();
        rl.close();
        process.exit(1);
    });
}

module.exports = { main };
