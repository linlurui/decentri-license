package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"sort"
	"strconv"
	"strings"
	"time"

	decenlicense "github.com/linlurui/decentrilicense/sdks/go"
)

// Global variable to store selected product key path
var selectedProductKeyPath string

// Global client instance to maintain state across operations
var globalClient *decenlicense.Client
var globalClientInitialized bool

// getOrCreateClient returns the global client instance, creating it if necessary
func getOrCreateClient() (*decenlicense.Client, error) {
	if globalClient == nil {
		var err error
		globalClient, err = decenlicense.NewClient()
		if err != nil {
			return nil, err
		}
		globalClientInitialized = false
	}
	return globalClient, nil
}

// cleanupClient closes the global client on program exit
func cleanupClient() {
	if globalClient != nil {
		globalClient.Close()
		globalClient = nil
		globalClientInitialized = false
	}
}

// readFromClipboard 从系统剪贴板读取内容
func readFromClipboard() (string, error) {
	// 在macOS上使用pbpaste命令
	cmd := exec.Command("pbpaste")
	output, err := cmd.Output()
	if err != nil {
		return "", err
	}
	return string(output), nil
}

// findProductPublicKey 查找当前目录和上级目录下的产品公钥文件
func findProductPublicKey() string {
	// 如果用户已经手动选择了，使用选择的
	if selectedProductKeyPath != "" {
		return selectedProductKeyPath
	}

	// 查找匹配模式的文件
	patterns := []string{
		"*.pem",                    // 当前目录的所有PEM文件
		"../*.pem",                 // 上级目录
		"../../*.pem",              // 上上级目录
		"../../../dl-issuer/*.pem", // dl-issuer目录（修复路径）
	}
	var candidates []string

	for _, pattern := range patterns {
		matches, _ := filepath.Glob(pattern)
		candidates = append(candidates, matches...)
	}

	// 去重并只保留产品公钥文件（排除私钥文件）
	seen := make(map[string]bool)
	var unique []string
	for _, file := range candidates {
		filename := filepath.Base(file)
		// 只选择产品公钥文件：包含"public"且不包含"private"
		if !seen[filename] &&
			strings.Contains(filename, "public") &&
			!strings.Contains(filename, "private") &&
			strings.HasSuffix(filename, ".pem") {
			seen[filename] = true
			unique = append(unique, filename)
		}
	}
	sort.Strings(unique)

	// 如果找到文件，返回第一个（这里返回的是文件名，需要解析完整路径）
	if len(unique) > 0 {
		return resolveProductKeyPath(unique[0])
	}

	return ""
}

// resolveProductKeyPath 根据文件名找到完整的产品公钥文件路径
func resolveProductKeyPath(filename string) string {
	// 可能的搜索路径
	searchPaths := []string{
		"./" + filename,                  // 当前目录
		"../" + filename,                 // 上级目录
		"../../" + filename,              // 上上级目录
		"../../../dl-issuer/" + filename, // dl-issuer目录（修复路径）
	}

	for _, path := range searchPaths {
		if _, err := os.Stat(path); err == nil {
			return path
		}
	}

	return filename // 如果都找不到，返回原文件名
}

// findAllProductKeys 查找所有可用的产品公钥文件
func findAllProductKeys() []string {
	// 使用与findProductPublicKey相同的搜索逻辑
	patterns := []string{
		"*.pem",                    // 当前目录的所有PEM文件
		"../*.pem",                 // 上级目录
		"../../*.pem",              // 上上级目录
		"../../../dl-issuer/*.pem", // dl-issuer目录（修复路径）
	}
	var candidates []string

	for _, pattern := range patterns {
		matches, _ := filepath.Glob(pattern)
		candidates = append(candidates, matches...)
	}

	// 去重并只保留产品公钥文件（排除私钥文件）
	seen := make(map[string]bool)
	var unique []string
	for _, file := range candidates {
		filename := filepath.Base(file)
		// 只选择产品公钥文件：包含"public"且不包含"private"
		if !seen[filename] &&
			strings.Contains(filename, "public") &&
			!strings.Contains(filename, "private") &&
			strings.HasSuffix(filename, ".pem") {
			seen[filename] = true
			unique = append(unique, filename)
		}
	}
	sort.Strings(unique)

	return unique
}

// findTokenFiles 查找当前目录和上级目录下的token文件
func findTokenFiles() []string {
	var candidates []string

	// 在当前目录和上级目录查找token文件
	patterns := []string{
		"token_*.txt",
		"token_*.json",
		"../token_*.txt",
		"../token_*.json",
		"../../../dl-issuer/token_*.txt", // dl-issuer目录（修复路径）
		"../../../dl-issuer/token_*.json",
		"../../../dl-issuer/token_*encrypted.txt", // 加密token文件
	}
	for _, pattern := range patterns {
		matches, _ := filepath.Glob(pattern)
		candidates = append(candidates, matches...)
	}

	// 去重并只保留文件名（不显示路径）
	seen := make(map[string]bool)
	var unique []string
	for _, file := range candidates {
		filename := filepath.Base(file)
		if !seen[filename] && strings.Contains(filename, "token_") && (strings.HasSuffix(filename, ".txt") || strings.HasSuffix(filename, ".json")) {
			seen[filename] = true
			unique = append(unique, filename)
		}
	}

	// 按文件名排序，方便查找
	sort.Strings(unique)

	return unique
}

// findEncryptedTokenFiles 查找加密的token文件（用于激活和验证合法性）
func findEncryptedTokenFiles() []string {
	var candidates []string

	// 在当前目录和上级目录查找加密token文件
	patterns := []string{
		"token_*encrypted.txt",
		"../token_*encrypted.txt",
		"../../../dl-issuer/token_*encrypted.txt",
	}
	for _, pattern := range patterns {
		matches, _ := filepath.Glob(pattern)
		candidates = append(candidates, matches...)
	}

	// 去重并只保留文件名
	seen := make(map[string]bool)
	var unique []string
	for _, file := range candidates {
		filename := filepath.Base(file)
		if !seen[filename] && strings.Contains(filename, "encrypted") {
			seen[filename] = true
			unique = append(unique, filename)
		}
	}

	sort.Strings(unique)
	return unique
}

// findActivatedTokenFiles 查找已激活的token文件
func findActivatedTokenFiles() []string {
	var candidates []string

	// 在当前目录和上级目录查找已激活token文件
	patterns := []string{
		"token_activated_*.txt",
		"../token_activated_*.txt",
		"../../../dl-issuer/token_activated_*.txt",
	}
	for _, pattern := range patterns {
		matches, _ := filepath.Glob(pattern)
		candidates = append(candidates, matches...)
	}

	// 去重并只保留文件名
	seen := make(map[string]bool)
	var unique []string
	for _, file := range candidates {
		filename := filepath.Base(file)
		if !seen[filename] && strings.Contains(filename, "activated") {
			seen[filename] = true
			unique = append(unique, filename)
		}
	}

	sort.Strings(unique)
	return unique
}

// findStateTokenFiles 查找状态token文件（用于记账信息）
func findStateTokenFiles() []string {
	var candidates []string

	// 查找已激活和状态变更的token文件
	patterns := []string{
		"token_activated_*.txt",
		"token_state_*.txt",
		"../token_activated_*.txt",
		"../token_state_*.txt",
		"../../../dl-issuer/token_activated_*.txt",
		"../../../dl-issuer/token_state_*.txt",
	}
	for _, pattern := range patterns {
		matches, _ := filepath.Glob(pattern)
		candidates = append(candidates, matches...)
	}

	// 去重并只保留文件名
	seen := make(map[string]bool)
	var unique []string
	for _, file := range candidates {
		filename := filepath.Base(file)
		if !seen[filename] && (strings.Contains(filename, "activated") || strings.Contains(filename, "state")) {
			seen[filename] = true
			unique = append(unique, filename)
		}
	}

	sort.Strings(unique)
	return unique
}

// resolveTokenFilePath 根据文件名找到完整的token文件路径
func resolveTokenFilePath(filename string) string {
	// 可能的搜索路径
	searchPaths := []string{
		"./" + filename,                  // 当前目录
		"../" + filename,                 // 上级目录
		"../../../dl-issuer/" + filename, // dl-issuer目录（修复路径）
	}

	for _, path := range searchPaths {
		if _, err := os.Stat(path); err == nil {
			return path
		}
	}

	return filename // 如果都找不到，返回原文件名
}

func main() {
	fmt.Println("==========================================")
	fmt.Println("DecentriLicense Go SDK 验证向导")
	fmt.Println("==========================================")
	fmt.Println()

	// Ensure client cleanup on exit
	defer cleanupClient()

	scanner := bufio.NewScanner(os.Stdin)

	for {
		fmt.Println("请选择要执行的操作:")
		fmt.Println("0. 🔑 选择产品公钥")
		fmt.Println("1. 🔓 激活令牌")
		fmt.Println("2. ✅ 校验已激活令牌")
		fmt.Println("3. 🔍 验证令牌合法性")
		fmt.Println("4. 📊 记账信息")
		fmt.Println("5. 🔗 信任链验证")
		fmt.Println("6. 🎯 综合验证")
		fmt.Println("7. � 恢复通道管理（密码/助记词）")
		fmt.Println("8. � 退出")
		fmt.Print("请输入选项 (0-8): ")

		if !scanner.Scan() {
			break
		}

		choice := strings.TrimSpace(scanner.Text())

		switch choice {
		case "0":
			selectProductKeyWizard(scanner)
		case "1":
			activateTokenWizard(scanner)
		case "2":
			verifyTokenWizard(scanner)
		case "3":
			validateTokenWizard(scanner)
		case "4":
			accountingWizard(scanner)
		case "5":
			trustChainValidationWizard(scanner)
		case "6":
			comprehensiveValidationWizard(scanner)
		case "7":
			recoveryChannelWizard(scanner)
		case "8":
			fmt.Println("感谢使用 DecentriLicense Go SDK 验证向导!")
			return
		default:
			fmt.Println("❌ 无效选项，请重新选择")
		}
		fmt.Println()
	}
}

func activateTokenWizard(scanner *bufio.Scanner) {
	fmt.Println("\n🔓 激活令牌")
	fmt.Println("----------")
	fmt.Println("⚠️  重要说明：")
	fmt.Println("   • 加密token（encrypted）：首次从供应商获得，需要激活")
	fmt.Println("   • 已激活token（activated）：激活后生成，可直接使用，不需再次激活")
	fmt.Println("   ⚠️  本功能仅用于【首次激活】加密token")
	fmt.Println("   ⚠️  如需使用已激活token，请直接选择其他功能（如记账、验证）")
	fmt.Println()

	// Use global client instance
	client, err := getOrCreateClient()
	if err != nil {
		log.Printf("❌ 创建客户端失败: %v", err)
		return
	}

	// 显示可用的加密token文件
	tokenFiles := findEncryptedTokenFiles()
	if len(tokenFiles) > 0 {
		fmt.Println("📄 发现以下加密token文件:")
		for i, file := range tokenFiles {
			fmt.Printf("   %d. %s\n", i+1, file)
		}
		fmt.Println("💡 您可以输入序号选择文件，或输入文件名/路径/token字符串")
	}

	// 获取令牌输入
	fmt.Println("请输入令牌字符串 (仅支持加密令牌):")
	fmt.Println("💡 加密令牌通常从软件提供商处获得")
	fmt.Println("💡 输入序号(1-N)可快速选择上面列出的文件")
	fmt.Println("💡 输入文件路径可读取指定文件")
	fmt.Println("💡 直接回车可以从剪贴板读取token")
	fmt.Print("令牌或文件路径: ")

	if !scanner.Scan() {
		fmt.Println("❌ 输入读取失败")
		return
	}
	input := strings.TrimSpace(scanner.Text())

	// 如果输入为空，尝试从剪贴板读取
	if input == "" {
		fmt.Println("📋 正在从剪贴板读取token...")
		clipboardContent, err := readFromClipboard()
		if err != nil {
			fmt.Printf("❌ 从剪贴板读取失败: %v\n", err)
			fmt.Println("💡 请手动输入token字符串")
			return
		}
		input = strings.TrimSpace(clipboardContent)
		if input == "" {
			fmt.Println("❌ 剪贴板为空，请手动输入token字符串")
			return
		}
		fmt.Printf("✅ 从剪贴板读取到 %d 个字符\n", len(input))
	}

	// 检查是否输入的是数字（文件序号）
	if len(tokenFiles) > 0 {
		if index, err := strconv.Atoi(input); err == nil {
			if index >= 1 && index <= len(tokenFiles) {
				// 用户输入了有效的序号，读取对应文件
				selectedFile := tokenFiles[index-1]
				filePath := resolveTokenFilePath(selectedFile)

				// 尝试读取文件
				if data, err := ioutil.ReadFile(filePath); err == nil {
					input = strings.TrimSpace(string(data))
					fmt.Printf("✅ 选择文件 '%s' 并读取到令牌 (%d 字符)\n", selectedFile, len(input))
				} else {
					fmt.Printf("❌ 无法读取文件 %s: %v\n", filePath, err)
					return
				}
			}
		}
	}

	tokenString := input

	// 检查是否是文件路径（非数字选择的情况）
	if strings.Contains(input, "/") || strings.Contains(input, "\\") || strings.HasSuffix(input, ".txt") || strings.Contains(input, "token_") {
		// 解析文件路径
		filePath := resolveTokenFilePath(input)

		// 尝试读取文件
		if data, err := ioutil.ReadFile(filePath); err == nil {
			tokenString = strings.TrimSpace(string(data))
			fmt.Printf("✅ 从文件读取到令牌 (%d 字符)\n", len(tokenString))
		} else {
			fmt.Printf("⚠️  无法读取文件 %s: %v\n", filePath, err)
			fmt.Println("💡 将直接使用输入作为令牌字符串")
		}
	} else {
		// 直接使用输入作为token字符串
		tokenString = input
	}

	// 首先初始化客户端（使用临时配置）- 只在第一次初始化
	if !globalClientInitialized {
		tempConfig := decenlicense.Config{
			LicenseCode:   "TEMP", // 临时配置，用于解析token
			PreferredMode: decenlicense.ConnectionModeOffline,
			UDPPort:       13325,
			TCPPort:       23325,
		}

		err = client.Initialize(tempConfig)
		if err != nil {
			log.Printf("⚠️  初始化失败 (需要产品公钥): %v", err)
			fmt.Println("正在查找产品公钥文件...")
			// 不设置globalClientInitialized，下次还会尝试
		} else {
			fmt.Println("✅ 客户端初始化成功")
			globalClientInitialized = true
		}
	} else {
		fmt.Println("✅ 客户端已初始化，使用现有实例")
	}

	// 查找和设置产品公钥
	// 优先使用用户通过菜单选项0选择的产品公钥，否则自动查找
	var productKeyPath string
	if selectedProductKeyPath != "" {
		productKeyPath = selectedProductKeyPath
		fmt.Printf("📄 使用用户选择的产品公钥文件: %s\n", productKeyPath)
	} else {
		productKeyPath = findProductPublicKey()
		if productKeyPath != "" {
			fmt.Printf("📄 使用产品公钥文件: %s\n", productKeyPath)
		}
	}

	if productKeyPath != "" {
		productKeyData, err := ioutil.ReadFile(productKeyPath)
		if err != nil {
			log.Printf("❌ 读取产品公钥文件失败: %v", err)
			return
		}

		err = client.SetProductPublicKey(string(productKeyData))
		if err != nil {
			log.Printf("❌ 设置产品公钥失败: %v", err)
			return
		}
		fmt.Println("✅ 产品公钥设置成功")
	} else {
		fmt.Println("⚠️  未找到产品公钥文件")
		fmt.Println("💡 请先选择产品公钥 (菜单选项 0)，或确保当前目录下有产品公钥文件")
		return
	}

	// 先尝试导入令牌
	fmt.Println("📥 正在导入令牌...")
	err = client.ImportToken(tokenString)
	if err != nil {
		log.Printf("❌ 令牌导入失败: %v", err)
		return
	}
	fmt.Println("✅ 令牌导入成功")

	// 然后激活当前导入的令牌
	fmt.Println("🎯 正在激活令牌...")
	result, err := client.ActivateBindDevice()
	if err != nil {
		log.Printf("❌ 激活失败: %v", err)
	} else if result.Valid {
		fmt.Println("✅ 令牌激活成功！")

		// 导出激活后的新token
		activatedToken, err := client.ExportActivatedTokenEncrypted()
		if err != nil {
			log.Printf("⚠️  导出激活token失败: %v", err)
		} else if activatedToken != "" {
			fmt.Println("\n📦 激活后的新Token（加密）:")
			fmt.Printf("   长度: %d 字符\n", len(activatedToken))
			if len(activatedToken) > 100 {
				fmt.Printf("   前缀: %s...\n", activatedToken[:100])
			} else {
				fmt.Printf("   内容: %s\n", activatedToken)
			}

			// 保存激活后的token到文件
			status, err := client.GetStatus()
			if err == nil && status.LicenseCode != "" {
				timestamp := time.Now().Format("20060102150405")
				filename := fmt.Sprintf("token_activated_%s_%s.txt", status.LicenseCode, timestamp)
				err = ioutil.WriteFile(filename, []byte(activatedToken), 0644)
				if err != nil {
					log.Printf("⚠️  保存token文件失败: %v", err)
				} else {
					absPath, _ := filepath.Abs(filename)
					fmt.Printf("\n💾 已保存到文件: %s\n", absPath)
					fmt.Println("   💡 此token包含设备绑定信息，可传递给下一个设备使用")
				}
			}
		}
	} else {
		fmt.Printf("❌ 令牌激活失败: %s\n", result.ErrorMessage)
	}

	// 显示最终状态
	activated, err := client.IsActivated()
	if err == nil && activated {
		fmt.Println("🔍 当前状态: 已激活")
		// 显示令牌信息
		status, err := client.GetStatus()
		if err == nil && status.HasToken {
			fmt.Printf("🎫 令牌ID: %s\n", status.TokenID)
			fmt.Printf("📝 许可证代码: %s\n", status.LicenseCode)
			fmt.Printf("👤 持有设备: %s\n", status.HolderDeviceID)
			issueTime := time.Unix(status.IssueTime, 0)
			fmt.Printf("📅 颁发时间: %s\n", issueTime.Format("2006-01-02 15:04:05"))
		}
	} else {
		fmt.Println("🔍 当前状态: 未激活")
		// 尝试获取状态信息
		status, err := client.GetStatus()
		if err == nil {
			fmt.Printf("📋 状态信息 - HasToken: %v, IsActivated: %v, TokenID: %s, HolderDeviceID: %s\n",
				status.HasToken, status.IsActivated, status.TokenID, status.HolderDeviceID)
		}
	}
}

func verifyTokenWizard(scanner *bufio.Scanner) {
	fmt.Println("\n✅ 校验已激活令牌")
	fmt.Println("----------------")

	// 扫描所有已激活的令牌
	stateDir := ".decentrilicense_state"
	entries, err := ioutil.ReadDir(stateDir)
	if err != nil || len(entries) == 0 {
		fmt.Println("⚠️  没有找到已激活的令牌")
		return
	}

	// 列出所有已激活的令牌
	var activatedTokens []string
	fmt.Println("\n📋 已激活的令牌列表:")
	for i, entry := range entries {
		if entry.IsDir() {
			activatedTokens = append(activatedTokens, entry.Name())
			// 尝试读取状态信息
			stateFile := filepath.Join(stateDir, entry.Name(), "current_state.json")
			if _, err := os.Stat(stateFile); err == nil {
				fmt.Printf("%d. %s ✅\n", i+1, entry.Name())
			} else {
				fmt.Printf("%d. %s ⚠️  (无状态文件)\n", i+1, entry.Name())
			}
		}
	}

	if len(activatedTokens) == 0 {
		fmt.Println("⚠️  没有找到已激活的令牌")
		return
	}

	// 让用户选择
	fmt.Printf("\n请选择要验证的令牌 (1-%d): ", len(activatedTokens))
	scanner.Scan()
	choice := strings.TrimSpace(scanner.Text())

	index, err := strconv.Atoi(choice)
	if err != nil || index < 1 || index > len(activatedTokens) {
		fmt.Println("❌ 无效的选择")
		return
	}

	selectedLicenseCode := activatedTokens[index-1]
	fmt.Printf("\n🔍 正在验证令牌: %s\n", selectedLicenseCode)

	// Use global client instance
	client, err := getOrCreateClient()
	if err != nil {
		log.Printf("❌ 获取客户端失败: %v", err)
		return
	}

	// 设置产品公钥（校验签名必须）
	var productKeyPath string
	if selectedProductKeyPath != "" {
		productKeyPath = selectedProductKeyPath
		fmt.Printf("📄 使用产品公钥文件: %s\n", filepath.Base(productKeyPath))
	} else {
		productKeyPath = findProductPublicKey()
		if productKeyPath != "" {
			fmt.Printf("📄 使用产品公钥文件: %s\n", filepath.Base(productKeyPath))
		}
	}
	if productKeyPath != "" {
		productKeyData, err := ioutil.ReadFile(productKeyPath)
		if err != nil {
			log.Printf("❌ 读取产品公钥失败: %v", err)
			return
		}
		err = client.SetProductPublicKey(string(productKeyData))
		if err != nil {
			log.Printf("❌ 设置产品公钥失败: %v", err)
			return
		}
	} else {
		fmt.Println("⚠️  未找到产品公钥，签名验证可能失败")
	}

	// 检查选择的令牌是否是当前激活的令牌
	status, err := client.GetStatus()
	if err == nil && status.LicenseCode == selectedLicenseCode {
		// 是当前激活的令牌，可以直接验证
		fmt.Println("🔍 正在验证令牌...")
		result, err := client.OfflineVerifyCurrentToken()
		if err != nil {
			log.Printf("❌ 令牌验证失败: %v", err)
		} else if result.Valid {
			fmt.Println("✅ 令牌验证成功")
			if result.ErrorMessage != "" {
				fmt.Printf("📄 信息: %s\n", result.ErrorMessage)
			}
		} else {
			fmt.Println("❌ 令牌验证失败")
			fmt.Printf("📄 错误信息: %s\n", result.ErrorMessage)
		}

		// 显示令牌信息
		if status.HasToken {
			fmt.Println("\n🎫 令牌信息:")
			fmt.Printf("   令牌ID: %s\n", status.TokenID)
			fmt.Printf("   许可证代码: %s\n", status.LicenseCode)
			fmt.Printf("   应用ID: %s\n", status.AppID)
			fmt.Printf("   持有设备ID: %s\n", status.HolderDeviceID)

			// 格式化颁发时间
			issueTime := time.Unix(status.IssueTime, 0)
			fmt.Printf("   颁发时间: %s\n", issueTime.Format("2006-01-02 15:04:05"))

			// 格式化到期时间
			if status.ExpireTime == 0 {
				fmt.Println("   到期时间: 永不过期")
			} else {
				expireTime := time.Unix(status.ExpireTime, 0)
				fmt.Printf("   到期时间: %s\n", expireTime.Format("2006-01-02 15:04:05"))
			}

			fmt.Printf("   状态索引: %d\n", status.StateIndex)
			fmt.Printf("   激活状态: %v\n", status.IsActivated)
		}
	} else {
		// 不是当前激活的令牌，读取状态文件显示信息
		fmt.Println("💡 此令牌不是当前激活的令牌，显示已保存的状态信息:")
		stateFile := filepath.Join(stateDir, selectedLicenseCode, "current_state.json")
		data, err := ioutil.ReadFile(stateFile)
		if err != nil {
			log.Printf("❌ 读取状态文件失败: %v", err)
			return
		}

		fmt.Println("\n🎫 令牌信息 (从状态文件读取):")
		fmt.Printf("   许可证代码: %s\n", selectedLicenseCode)
		fmt.Printf("   状态文件: %s\n", stateFile)
		fmt.Printf("   文件大小: %d 字节\n", len(data))
		fmt.Println("\n💡 提示: 如需完整验证此令牌，请使用选项1重新激活")
	}
}

func validateTokenWizard(scanner *bufio.Scanner) {
	fmt.Println("\n🔍 验证令牌合法性")
	fmt.Println("----------------")

	// Use global client instance
	client, err := getOrCreateClient()
	if err != nil {
		log.Printf("❌ 获取客户端失败: %v", err)
		return
	}

	// 初始化客户端（使用默认配置）
	if !globalClientInitialized {
		config := decenlicense.Config{
			LicenseCode:   "VALIDATE", // 验证模式
			PreferredMode: decenlicense.ConnectionModeOffline,
			UDPPort:       13325,
			TCPPort:       23325,
		}

		err = client.Initialize(config)
		if err != nil {
			log.Printf("⚠️  初始化失败 (需要产品公钥): %v", err)
			fmt.Println("正在查找产品公钥文件...")
		} else {
			fmt.Println("✅ 客户端初始化成功")
			globalClientInitialized = true
		}
	}

	// 查找和设置产品公钥
	// 优先使用用户通过菜单选项0选择的产品公钥，否则自动查找
	var productKeyPath string
	if selectedProductKeyPath != "" {
		productKeyPath = selectedProductKeyPath
		fmt.Printf("📄 使用用户选择的产品公钥文件: %s\n", productKeyPath)
	} else {
		productKeyPath = findProductPublicKey()
		if productKeyPath != "" {
			fmt.Printf("📄 使用产品公钥文件: %s\n", productKeyPath)
		}
	}

	if productKeyPath != "" {
		productKeyData, err := ioutil.ReadFile(productKeyPath)
		if err != nil {
			log.Printf("❌ 读取产品公钥文件失败: %v", err)
			return
		}

		err = client.SetProductPublicKey(string(productKeyData))
		if err != nil {
			log.Printf("❌ 设置产品公钥失败: %v", err)
			return
		}
		fmt.Println("✅ 产品公钥设置成功")
	} else {
		fmt.Println("⚠️  未找到产品公钥文件")
		fmt.Println("💡 请先选择产品公钥 (菜单选项 0)，或确保当前目录下有产品公钥文件")
		return
	}

	// 显示可用的加密token文件
	tokenFiles := findEncryptedTokenFiles()
	if len(tokenFiles) > 0 {
		fmt.Println("📄 发现以下加密token文件:")
		for i, file := range tokenFiles {
			fmt.Printf("   %d. %s\n", i+1, file)
		}
		fmt.Println("💡 您可以输入序号选择文件，或输入文件名/路径/token字符串")
	}

	// 获取令牌输入
	fmt.Println("请输入要验证的令牌字符串 (支持加密令牌):")
	fmt.Println("💡 令牌通常从软件提供商处获得，或从加密令牌文件读取")
	fmt.Println("💡 如果是文件路径，请输入完整的文件路径")
	fmt.Println("💡 直接回车可以从剪贴板读取token")
	fmt.Print("令牌或文件路径: ")

	if !scanner.Scan() {
		fmt.Println("❌ 输入读取失败")
		return
	}
	input := strings.TrimSpace(scanner.Text())

	// 如果输入为空，尝试从剪贴板读取
	if input == "" {
		fmt.Println("📋 正在从剪贴板读取token...")
		clipboardContent, err := readFromClipboard()
		if err != nil {
			fmt.Printf("❌ 从剪贴板读取失败: %v\n", err)
			fmt.Println("💡 请手动输入token字符串")
			return
		}
		input = strings.TrimSpace(clipboardContent)
		if input == "" {
			fmt.Println("❌ 剪贴板为空，请手动输入token字符串")
			return
		}
		fmt.Printf("✅ 从剪贴板读取到 %d 个字符\n", len(input))
	}

	tokenString := input

	// 检查是否是数字选择（对应文件列表中的索引）
	if numChoice, err := strconv.Atoi(input); err == nil && numChoice >= 1 && numChoice <= len(tokenFiles) {
		// 用户选择了文件列表中的一个文件
		selectedFile := tokenFiles[numChoice-1]
		filePath := resolveTokenFilePath(selectedFile)

		// 尝试读取文件
		if data, err := ioutil.ReadFile(filePath); err == nil {
			tokenString = strings.TrimSpace(string(data))
			fmt.Printf("✅ 从文件 '%s' 读取到令牌 (%d 字符)\n", selectedFile, len(tokenString))
		} else {
			fmt.Printf("❌ 无法读取文件 %s: %v\n", filePath, err)
			return
		}
	} else if strings.Contains(input, "/") || strings.Contains(input, "\\") || strings.HasSuffix(input, ".txt") || strings.Contains(input, "token_") {
		// 解析文件路径
		filePath := resolveTokenFilePath(input)

		// 尝试读取文件
		if data, err := ioutil.ReadFile(filePath); err == nil {
			tokenString = strings.TrimSpace(string(data))
			fmt.Printf("✅ 从文件读取到令牌 (%d 字符)\n", len(tokenString))
		} else {
			fmt.Printf("⚠️  无法读取文件 %s: %v\n", filePath, err)
			fmt.Println("💡 将直接使用输入作为令牌字符串")
		}
	} else {
		// 直接使用输入作为token字符串
		tokenString = input
	}

	// 验证令牌
	fmt.Println("🔍 正在验证令牌合法性...")
	result, err := client.ValidateToken(tokenString)
	if err != nil {
		log.Printf("❌ 令牌验证失败: %v", err)
	} else if result.Valid {
		fmt.Println("✅ 令牌验证成功 - 令牌合法且有效")
		if result.ErrorMessage != "" {
			fmt.Printf("📄 详细信息: %s\n", result.ErrorMessage)
		}
	} else {
		fmt.Println("❌ 令牌验证失败 - 令牌不合法或无效")
		if result.ErrorMessage != "" {
			fmt.Printf("📄 错误信息: %s\n", result.ErrorMessage)
		}
	}
}

func accountingWizard(scanner *bufio.Scanner) {
	fmt.Println("\n📊 记账信息")
	fmt.Println("----------")

	// Use global client instance
	client, err := getOrCreateClient()
	if err != nil {
		log.Printf("❌ 获取客户端失败: %v", err)
		return
	}

	// 显示可用的状态token文件
	tokenFiles := findStateTokenFiles()

	// 检查激活状态
	activated, err := client.IsActivated()
	if err != nil {
		log.Printf("❌ 检查激活状态失败: %v", err)
		return
	}

	// 显示令牌选择选项
	fmt.Println("\n💡 请选择令牌来源:")
	if activated {
		fmt.Println("0. 使用当前激活的令牌")
	}

	if len(tokenFiles) > 0 {
		fmt.Println("\n📄 或从以下文件加载令牌:")
		for i, file := range tokenFiles {
			fmt.Printf("%d. %s\n", i+1, file)
		}
	}

	if !activated && len(tokenFiles) == 0 {
		fmt.Println("❌ 当前没有激活的令牌，也没有找到可用的token文件")
		fmt.Println("💡 请先使用选项1激活令牌")
		return
	}

	fmt.Print("\n请选择 (0")
	if len(tokenFiles) > 0 {
		fmt.Printf("-%d", len(tokenFiles))
	}
	fmt.Print("): ")

	scanner.Scan()
	tokenChoice := strings.TrimSpace(scanner.Text())
	tokenChoiceNum, err := strconv.Atoi(tokenChoice)

	// 处理令牌选择
	if err != nil || tokenChoiceNum < 0 || tokenChoiceNum > len(tokenFiles) {
		fmt.Println("❌ 无效的选择")
		return
	}

	// 如果选择从文件加载
	if tokenChoiceNum > 0 {
		selectedFile := tokenFiles[tokenChoiceNum-1]
		filePath := resolveTokenFilePath(selectedFile)

		fmt.Printf("📂 正在从文件加载令牌: %s\n", selectedFile)

		// 读取文件
		tokenData, err := ioutil.ReadFile(filePath)
		if err != nil {
			log.Printf("❌ 读取文件失败: %v", err)
			return
		}

		tokenString := strings.TrimSpace(string(tokenData))
		fmt.Printf("✅ 读取到令牌 (%d 字符)\n", len(tokenString))

		// 初始化客户端（如果还没初始化）
		if !globalClientInitialized {
			tempConfig := decenlicense.Config{
				LicenseCode:   "ACCOUNTING",
				PreferredMode: decenlicense.ConnectionModeOffline,
				UDPPort:       13325,
				TCPPort:       23325,
			}
			err = client.Initialize(tempConfig)
			if err != nil {
				log.Printf("⚠️  初始化失败: %v", err)
			} else {
				globalClientInitialized = true
			}
		}

		// 设置产品公钥
		var productKeyPath string
		if selectedProductKeyPath != "" {
			productKeyPath = selectedProductKeyPath
		} else {
			productKeyPath = findProductPublicKey()
		}

		if productKeyPath != "" {
			productKeyData, err := ioutil.ReadFile(productKeyPath)
			if err != nil {
				log.Printf("❌ 读取产品公钥失败: %v", err)
				return
			}
			err = client.SetProductPublicKey(string(productKeyData))
			if err != nil {
				log.Printf("❌ 设置产品公钥失败: %v", err)
				return
			}
			fmt.Println("✅ 产品公钥设置成功")
		}

		// 导入令牌
		fmt.Println("📥 正在导入令牌...")
		err = client.ImportToken(tokenString)
		if err != nil {
			log.Printf("❌ 令牌导入失败: %v", err)
			return
		}
		fmt.Println("✅ 令牌导入成功")

		// 检查令牌类型
		isAlreadyActivated := strings.Contains(selectedFile, "activated") || strings.Contains(selectedFile, "state")

		if isAlreadyActivated {
			fmt.Println("💡 检测到已激活令牌")
			// 对于已激活token，ActivateBindDevice是幂等操作
			// 它会恢复激活状态，但不会重新生成新的token
			fmt.Println("🔄 正在恢复激活状态...")
		} else {
			// 对于加密token，这是首次激活
			fmt.Println("🎯 正在首次激活令牌...")
		}

		// 调用ActivateBindDevice恢复/设置激活状态
		result, err := client.ActivateBindDevice()
		if err != nil {
			log.Printf("❌ 激活失败: %v", err)
			return
		} else if !result.Valid {
			log.Printf("❌ 激活失败: %s", result.ErrorMessage)
			return
		}

		if isAlreadyActivated {
			fmt.Println("✅ 激活状态已恢复（token未改变）")
		} else {
			fmt.Println("✅ 首次激活成功")
		}
	}

	// 显示当前令牌信息
	status, err := client.GetStatus()
	if err == nil && status.HasToken {
		fmt.Println("\n📋 当前令牌信息:")
		fmt.Printf("   许可证代码: %s\n", status.LicenseCode)
		fmt.Printf("   应用ID: %s\n", status.AppID)
		fmt.Printf("   当前状态索引: %d\n", status.StateIndex)
		fmt.Printf("   令牌ID: %s\n", status.TokenID)
	} else {
		fmt.Println("⚠️  无法获取令牌信息")
		return
	}

	// 提供记账选项 - 遵循usage_chain结构
	fmt.Println("\n💡 请选择记账方式:")
	fmt.Println("1. 快速测试记账（使用默认测试数据）")
	fmt.Println("2. 记录业务操作（向导式输入）")
	fmt.Print("\n请选择 (1-2): ")
	scanner.Scan()
	choice := strings.TrimSpace(scanner.Text())

	var action string
	var params map[string]interface{}

	switch choice {
	case "1":
		// 快速测试 - 使用默认数据
		action = "api_call"
		params = map[string]interface{}{
			"function": "test_function",
			"result":   "success",
		}
		fmt.Printf("💡 使用测试数据: action=%s, params=%v\n", action, params)

	case "2":
		// 业务操作记账 - 向导式输入
		fmt.Println("\n📝 usage_chain 结构说明:")
		fmt.Println("┌─────────────────────────────────────────────────────────┐")
		fmt.Println("│ 字段名      │ 说明           │ 填写方式              │")
		fmt.Println("├─────────────────────────────────────────────────────────┤")
		fmt.Println("│ seq         │ 序列号         │ ✅ 系统自动填充       │")
		fmt.Println("│ time        │ 时间戳         │ ✅ 系统自动填充       │")
		fmt.Println("│ action      │ 操作类型       │ 👉 需要您输入         │")
		fmt.Println("│ params      │ 操作参数       │ 👉 需要您输入         │")
		fmt.Println("│ hash_prev   │ 前状态哈希     │ ✅ 系统自动填充       │")
		fmt.Println("│ signature   │ 数字签名       │ ✅ 系统自动填充       │")
		fmt.Println("└─────────────────────────────────────────────────────────┘")

		// 输入action
		fmt.Println("\n👉 第1步: 输入操作类型 (action)")
		fmt.Println("   常用操作类型:")
		fmt.Println("   • api_call      - API调用")
		fmt.Println("   • feature_usage - 功能使用")
		fmt.Println("   • save_file     - 保存文件")
		fmt.Println("   • export_data   - 导出数据")
		fmt.Print("\n请输入操作类型: ")
		scanner.Scan()
		action = strings.TrimSpace(scanner.Text())
		if action == "" {
			fmt.Println("❌ 操作类型不能为空")
			return
		}

		// 输入params - 引导用户输入键值对
		fmt.Println("\n👉 第2步: 输入操作参数 (params)")
		fmt.Println("   params 是一个JSON对象，包含操作的具体参数")
		fmt.Println("   输入格式: key=value (每行一个)")
		fmt.Println("   示例:")
		fmt.Println("   • function=process_image")
		fmt.Println("   • file_name=report.pdf")
		fmt.Println("   • size=1024")
		fmt.Println("   输入空行结束输入")

		params = make(map[string]interface{})
		for {
			fmt.Print("参数 (key=value 或直接回车结束): ")
			scanner.Scan()
			line := strings.TrimSpace(scanner.Text())
			if line == "" {
				break
			}

			parts := strings.SplitN(line, "=", 2)
			if len(parts) == 2 {
				key := strings.TrimSpace(parts[0])
				value := strings.TrimSpace(parts[1])
				params[key] = value
			} else {
				fmt.Println("⚠️  格式错误,请使用 key=value 格式")
			}
		}

		if len(params) == 0 {
			fmt.Println("⚠️  未输入任何参数,将使用空参数对象")
			params = make(map[string]interface{})
		}

	default:
		fmt.Println("❌ 无效的选择")
		return
	}

	// 构建符合usage_chain结构的JSON
	// 注意: seq, time, hash_prev, signature 由SDK自动填充
	usageChainEntry := map[string]interface{}{
		"action": action,
		"params": params,
	}

	accountingDataBytes, err := json.Marshal(usageChainEntry)
	if err != nil {
		log.Printf("❌ 构建JSON失败: %v", err)
		return
	}
	accountingData := string(accountingDataBytes)
	fmt.Printf("\n📝 记账数据 (业务字段): %s\n", accountingData)
	fmt.Println("   (系统字段 seq, time, hash_prev, signature 将由SDK自动添加)")

	// 记录使用情况
	fmt.Println("📝 正在记录使用情况...")
	result, err := client.RecordUsage(accountingData)
	if err != nil {
		log.Printf("❌ 记账失败: %v", err)
	} else if result.Valid {
		fmt.Println("✅ 记账成功")
		fmt.Printf("📄 响应: %s\n", result.ErrorMessage)

		// 导出状态变更后的新token
		stateToken, err := client.ExportStateChangedTokenEncrypted()
		if err != nil {
			log.Printf("⚠️  导出状态变更token失败: %v", err)
		} else if stateToken != "" {
			fmt.Println("\n📦 状态变更后的新Token（加密）:")
			fmt.Printf("   长度: %d 字符\n", len(stateToken))
			if len(stateToken) > 100 {
				fmt.Printf("   前缀: %s...\n", stateToken[:100])
			} else {
				fmt.Printf("   内容: %s\n", stateToken)
			}

			// 保存状态变更后的token到文件
			status, err := client.GetStatus()
			if err == nil && status.LicenseCode != "" {
				timestamp := time.Now().Format("20060102150405")
				filename := fmt.Sprintf("token_state_%s_idx%d_%s.txt", status.LicenseCode, status.StateIndex, timestamp)
				err = ioutil.WriteFile(filename, []byte(stateToken), 0644)
				if err != nil {
					log.Printf("⚠️  保存token文件失败: %v", err)
				} else {
					absPath, _ := filepath.Abs(filename)
					fmt.Printf("\n💾 已保存到文件: %s\n", absPath)
					fmt.Println("   💡 此token包含最新状态链，可传递给下一个设备使用")
				}
			}
		}
	} else {
		fmt.Println("❌ 记账失败")
		fmt.Printf("📄 错误信息: %s\n", result.ErrorMessage)
	}
}

func trustChainValidationWizard(scanner *bufio.Scanner) {
	fmt.Println("\n🔗 信任链验证")
	fmt.Println("============")
	fmt.Println("💡 信任链验证检查加密签名的完整性：根密钥 → 产品公钥 → 令牌签名 → 设备绑定")
	fmt.Println()

	// Use global client instance
	client, err := getOrCreateClient()
	if err != nil {
		log.Printf("❌ 获取客户端失败: %v", err)
		return
	}

	// 显示可用的token文件
	tokenFiles := findStateTokenFiles()

	// 检查激活状态
	activated, err := client.IsActivated()
	if err != nil {
		log.Printf("❌ 检查激活状态失败: %v", err)
		return
	}

	// 显示令牌选择选项
	fmt.Println("\n💡 请选择令牌来源:")
	if activated {
		fmt.Println("0. 使用当前激活的令牌")
	}

	if len(tokenFiles) > 0 {
		fmt.Println("\n📄 或从以下文件加载令牌:")
		for i, file := range tokenFiles {
			fmt.Printf("%d. %s\n", i+1, file)
		}
	}

	if !activated && len(tokenFiles) == 0 {
		fmt.Println("❌ 当前没有激活的令牌，也没有找到可用的token文件")
		fmt.Println("💡 请先使用选项1激活令牌")
		return
	}

	fmt.Print("\n请选择 (0")
	if len(tokenFiles) > 0 {
		fmt.Printf("-%d", len(tokenFiles))
	}
	fmt.Print("): ")

	scanner.Scan()
	tokenChoice := strings.TrimSpace(scanner.Text())
	tokenChoiceNum, err := strconv.Atoi(tokenChoice)

	// 处理令牌选择
	if err != nil || tokenChoiceNum < 0 || tokenChoiceNum > len(tokenFiles) {
		fmt.Println("❌ 无效的选择")
		return
	}

	// 如果选择从文件加载
	if tokenChoiceNum > 0 {
		selectedFile := tokenFiles[tokenChoiceNum-1]
		filePath := resolveTokenFilePath(selectedFile)

		fmt.Printf("📂 正在从文件加载令牌: %s\n", selectedFile)

		// 读取文件
		tokenData, err := ioutil.ReadFile(filePath)
		if err != nil {
			log.Printf("❌ 读取文件失败: %v", err)
			return
		}

		tokenString := strings.TrimSpace(string(tokenData))
		fmt.Printf("✅ 读取到令牌 (%d 字符)\n", len(tokenString))

		// 初始化客户端（如果还没初始化）
		if !globalClientInitialized {
			tempConfig := decenlicense.Config{
				LicenseCode:   "TRUST_CHAIN",
				PreferredMode: decenlicense.ConnectionModeOffline,
				UDPPort:       13325,
				TCPPort:       23325,
			}
			err = client.Initialize(tempConfig)
			if err != nil {
				log.Printf("⚠️  初始化失败: %v", err)
			} else {
				globalClientInitialized = true
			}
		}

		// 设置产品公钥
		var productKeyPath string
		if selectedProductKeyPath != "" {
			productKeyPath = selectedProductKeyPath
		} else {
			productKeyPath = findProductPublicKey()
		}

		if productKeyPath != "" {
			productKeyData, err := ioutil.ReadFile(productKeyPath)
			if err != nil {
				log.Printf("❌ 读取产品公钥失败: %v", err)
				return
			}
			err = client.SetProductPublicKey(string(productKeyData))
			if err != nil {
				log.Printf("❌ 设置产品公钥失败: %v", err)
				return
			}
			fmt.Println("✅ 产品公钥设置成功")
		}

		// 导入令牌
		fmt.Println("📥 正在导入令牌...")
		err = client.ImportToken(tokenString)
		if err != nil {
			log.Printf("❌ 令牌导入失败: %v", err)
			return
		}
		fmt.Println("✅ 令牌导入成功")

		// 检查令牌类型
		isAlreadyActivated := strings.Contains(selectedFile, "activated") || strings.Contains(selectedFile, "state")

		if isAlreadyActivated {
			fmt.Println("💡 检测到已激活令牌")
			fmt.Println("🔄 正在恢复激活状态...")
		} else {
			fmt.Println("🎯 正在首次激活令牌...")
		}

		// 调用ActivateBindDevice恢复/设置激活状态
		result, err := client.ActivateBindDevice()
		if err != nil {
			log.Printf("❌ 激活失败: %v", err)
			return
		} else if !result.Valid {
			log.Printf("❌ 激活失败: %s", result.ErrorMessage)
			return
		}

		if isAlreadyActivated {
			fmt.Println("✅ 激活状态已恢复（token未改变）")
		} else {
			fmt.Println("✅ 首次激活成功")
		}
	}

	fmt.Println("📋 开始验证信任链...")
	fmt.Println()

	checksPassed := 0
	totalChecks := 4

	// 1. 基础令牌签名验证
	fmt.Println("🔍 [1/4] 验证令牌签名（根密钥 → 产品公钥 → 令牌）")
	result, err := client.OfflineVerifyCurrentToken()
	if err != nil {
		fmt.Printf("   ❌ 失败: %v\n", err)
	} else if !result.Valid {
		fmt.Printf("   ❌ 失败: %s\n", result.ErrorMessage)
	} else {
		fmt.Println("   ✅ 通过: 令牌签名有效，信任链完整")
		checksPassed++
	}
	fmt.Println()

	// 2. 检查设备状态
	fmt.Println("🔍 [2/4] 验证设备状态")
	state, err := client.GetDeviceState()
	if err != nil {
		fmt.Printf("   ⚠️  警告: 无法获取设备状态 - %v\n", err)
	} else {
		fmt.Printf("   ✅ 通过: 设备状态正常 (状态码: %d)\n", state)
		checksPassed++
	}
	fmt.Println()

	// 3. 检查令牌持有者匹配
	fmt.Println("🔍 [3/4] 验证令牌持有者与当前设备匹配")
	token, err := client.GetCurrentToken()
	if err != nil {
		fmt.Printf("   ⚠️  警告: 无法获取令牌信息 - %v\n", err)
	} else if token != nil {
		deviceID, err := client.GetDeviceID()
		if err != nil {
			fmt.Printf("   ⚠️  警告: 无法获取设备ID - %v\n", err)
		} else if token.HolderDeviceID == deviceID {
			fmt.Println("   ✅ 通过: 令牌持有者与当前设备匹配")
			fmt.Printf("   📱 设备ID: %s\n", deviceID)
			checksPassed++
		} else {
			fmt.Println("   ⚠️  不匹配: 令牌持有者与当前设备不一致")
			fmt.Printf("   📱 当前设备ID: %s\n", deviceID)
			fmt.Printf("   🎫 令牌持有者ID: %s\n", token.HolderDeviceID)
			fmt.Println("   💡 这可能表示令牌是从其他设备导入的")
		}
	}
	fmt.Println()

	// 4. 显示令牌详细信息
	fmt.Println("🔍 [4/4] 检查令牌详细信息")
	status, err := client.GetStatus()
	if err != nil {
		fmt.Printf("   ⚠️  警告: 无法获取状态信息 - %v\n", err)
	} else if status.HasToken {
		fmt.Println("   ✅ 通过: 令牌信息完整")
		fmt.Printf("   🎫 令牌ID: %s\n", status.TokenID)
		fmt.Printf("   📝 许可证代码: %s\n", status.LicenseCode)
		fmt.Printf("   📱 应用ID: %s\n", status.AppID)
		issueTime := time.Unix(status.IssueTime, 0)
		fmt.Printf("   📅 颁发时间: %s\n", issueTime.Format("2006-01-02 15:04:05"))
		if status.ExpireTime == 0 {
			fmt.Println("   ⏰ 到期时间: 永不过期")
		} else {
			expireTime := time.Unix(status.ExpireTime, 0)
			fmt.Printf("   ⏰ 到期时间: %s\n", expireTime.Format("2006-01-02 15:04:05"))
		}
		checksPassed++
	}
	fmt.Println()

	// 结果汇总
	fmt.Println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
	fmt.Printf("📊 验证结果: %d/%d 项检查通过\n", checksPassed, totalChecks)
	if checksPassed == totalChecks {
		fmt.Println("🎉 信任链验证完全通过！令牌可信且安全")
	} else if checksPassed >= 2 {
		fmt.Println("⚠️  部分检查通过，令牌基本可用但存在警告")
	} else {
		fmt.Println("❌ 多项检查失败，请检查令牌和设备状态")
	}
	fmt.Println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
}

func selectProductKeyWizard(scanner *bufio.Scanner) {
	fmt.Println("\n🔑 选择产品公钥")
	fmt.Println("==============")

	// 查找所有可用的产品公钥文件
	availableKeys := findAllProductKeys()

	if len(availableKeys) == 0 {
		fmt.Println("❌ 当前目录下没有找到产品公钥文件")
		fmt.Println("💡 请将产品公钥文件 (public_*.pem) 放置在当前目录下")
		return
	}

	fmt.Println("📄 找到以下产品公钥文件:")
	for i, keyFile := range availableKeys {
		fmt.Printf("%d. %s\n", i+1, keyFile)
	}
	fmt.Printf("%d. 取消选择\n", len(availableKeys)+1)

	if selectedProductKeyPath != "" {
		fmt.Printf("✅ 当前已选择: %s\n", selectedProductKeyPath)
	}

	fmt.Printf("请选择要使用的产品公钥文件 (1-%d): ", len(availableKeys)+1)

	if !scanner.Scan() {
		return
	}

	choice := strings.TrimSpace(scanner.Text())
	choiceNum, err := strconv.Atoi(choice)
	if err != nil || choiceNum < 1 || choiceNum > len(availableKeys)+1 {
		fmt.Println("❌ 无效选择")
		return
	}

	if choiceNum == len(availableKeys)+1 {
		// 取消选择
		selectedProductKeyPath = ""
		fmt.Println("✅ 已取消产品公钥选择")
		return
	}

	// 选择指定的文件
	selectedFile := availableKeys[choiceNum-1]
	selectedProductKeyPath = resolveProductKeyPath(selectedFile)
	fmt.Printf("✅ 已选择产品公钥文件: %s\n", selectedFile)
}

func comprehensiveValidationWizard(scanner *bufio.Scanner) {
	fmt.Println("\n🎯 综合验证")
	fmt.Println("----------")

	// Use global client instance
	client, err := getOrCreateClient()
	if err != nil {
		log.Printf("❌ 获取客户端失败: %v", err)
		return
	}

	// 显示可用的token文件
	tokenFiles := findStateTokenFiles()

	// 检查激活状态
	activated, err := client.IsActivated()
	if err != nil {
		log.Printf("❌ 检查激活状态失败: %v", err)
		return
	}

	// 显示令牌选择选项
	fmt.Println("\n💡 请选择令牌来源:")
	if activated {
		fmt.Println("0. 使用当前激活的令牌")
	}

	if len(tokenFiles) > 0 {
		fmt.Println("\n📄 或从以下文件加载令牌:")
		for i, file := range tokenFiles {
			fmt.Printf("%d. %s\n", i+1, file)
		}
	}

	if !activated && len(tokenFiles) == 0 {
		fmt.Println("❌ 当前没有激活的令牌，也没有找到可用的token文件")
		fmt.Println("💡 请先使用选项1激活令牌")
		return
	}

	fmt.Print("\n请选择 (0")
	if len(tokenFiles) > 0 {
		fmt.Printf("-%d", len(tokenFiles))
	}
	fmt.Print("): ")

	scanner.Scan()
	tokenChoice := strings.TrimSpace(scanner.Text())
	tokenChoiceNum, err := strconv.Atoi(tokenChoice)

	// 处理令牌选择
	if err != nil || tokenChoiceNum < 0 || tokenChoiceNum > len(tokenFiles) {
		fmt.Println("❌ 无效的选择")
		return
	}

	// 如果选择从文件加载
	if tokenChoiceNum > 0 {
		selectedFile := tokenFiles[tokenChoiceNum-1]
		filePath := resolveTokenFilePath(selectedFile)

		fmt.Printf("📂 正在从文件加载令牌: %s\n", selectedFile)

		// 读取文件
		tokenData, err := ioutil.ReadFile(filePath)
		if err != nil {
			log.Printf("❌ 读取文件失败: %v", err)
			return
		}

		tokenString := strings.TrimSpace(string(tokenData))
		fmt.Printf("✅ 读取到令牌 (%d 字符)\n", len(tokenString))

		// 初始化客户端（如果还没初始化）
		if !globalClientInitialized {
			tempConfig := decenlicense.Config{
				LicenseCode:   "COMPREHENSIVE",
				PreferredMode: decenlicense.ConnectionModeOffline,
				UDPPort:       13325,
				TCPPort:       23325,
			}
			err = client.Initialize(tempConfig)
			if err != nil {
				log.Printf("⚠️  初始化失败: %v", err)
			} else {
				globalClientInitialized = true
			}
		}

		// 设置产品公钥
		var productKeyPath string
		if selectedProductKeyPath != "" {
			productKeyPath = selectedProductKeyPath
		} else {
			productKeyPath = findProductPublicKey()
		}

		if productKeyPath != "" {
			productKeyData, err := ioutil.ReadFile(productKeyPath)
			if err != nil {
				log.Printf("❌ 读取产品公钥失败: %v", err)
				return
			}
			err = client.SetProductPublicKey(string(productKeyData))
			if err != nil {
				log.Printf("❌ 设置产品公钥失败: %v", err)
				return
			}
			fmt.Println("✅ 产品公钥设置成功")
		}

		// 导入令牌
		fmt.Println("📥 正在导入令牌...")
		err = client.ImportToken(tokenString)
		if err != nil {
			log.Printf("❌ 令牌导入失败: %v", err)
			return
		}
		fmt.Println("✅ 令牌导入成功")

		// 检查令牌类型
		isAlreadyActivated := strings.Contains(selectedFile, "activated") || strings.Contains(selectedFile, "state")

		if isAlreadyActivated {
			fmt.Println("💡 检测到已激活令牌")
			fmt.Println("🔄 正在恢复激活状态...")
		} else {
			fmt.Println("🎯 正在首次激活令牌...")
		}

		// 调用ActivateBindDevice恢复/设置激活状态
		result, err := client.ActivateBindDevice()
		if err != nil {
			log.Printf("❌ 激活失败: %v", err)
			return
		} else if !result.Valid {
			log.Printf("❌ 激活失败: %s", result.ErrorMessage)
			return
		}

		if isAlreadyActivated {
			fmt.Println("✅ 激活状态已恢复（token未改变）")
		} else {
			fmt.Println("✅ 首次激活成功")
		}
	}

	fmt.Println("📋 执行综合验证流程...")
	checkCount := 0
	passCount := 0

	// 1. 检查激活状态
	checkCount++
	activated, err = client.IsActivated()
	if err != nil {
		fmt.Printf("❌ 检查%d失败: 激活状态查询失败 - %v\n", checkCount, err)
	} else {
		passCount++
		if activated {
			fmt.Printf("✅ 检查%d通过: 许可证已激活\n", checkCount)
		} else {
			fmt.Printf("⚠️  检查%d: 许可证未激活\n", checkCount)
		}
	}

	// 2. 验证当前令牌
	if activated {
		checkCount++
		result, err := client.OfflineVerifyCurrentToken()
		if err != nil {
			fmt.Printf("❌ 检查%d失败: 令牌验证失败 - %v\n", checkCount, err)
		} else if result.Valid {
			passCount++
			fmt.Printf("✅ 检查%d通过: 令牌验证成功\n", checkCount)
		} else {
			fmt.Printf("❌ 检查%d失败: 令牌验证失败 - %s\n", checkCount, result.ErrorMessage)
		}
	}

	// 3. 检查设备状态
	checkCount++
	state, err := client.GetDeviceState()
	if err != nil {
		fmt.Printf("❌ 检查%d失败: 设备状态查询失败 - %v\n", checkCount, err)
	} else {
		passCount++
		fmt.Printf("✅ 检查%d通过: 设备状态正常 (状态码: %d)\n", checkCount, state)
	}

	// 4. 检查令牌信息
	checkCount++
	token, err := client.GetCurrentToken()
	if err != nil {
		if activated {
			fmt.Printf("❌ 检查%d失败: 令牌信息查询失败 - %v\n", checkCount, err)
		} else {
			fmt.Printf("⚠️  检查%d: 无令牌信息 (未激活)\n", checkCount)
		}
	} else if token != nil {
		passCount++
		if len(token.TokenID) >= 16 {
			fmt.Printf("✅ 检查%d通过: 令牌信息完整 (ID: %s...)\n", checkCount, token.TokenID[:16])
		} else if len(token.TokenID) > 0 {
			fmt.Printf("✅ 检查%d通过: 令牌信息完整 (ID: %s)\n", checkCount, token.TokenID)
		} else {
			fmt.Printf("✅ 检查%d通过: 令牌对象存在\n", checkCount)
		}
	} else {
		fmt.Printf("⚠️  检查%d: 无令牌信息\n", checkCount)
	}

	// 5. 测试记账功能
	if activated {
		checkCount++
		testData := `{"action":"comprehensive_test","timestamp":1234567890}`
		result, err := client.RecordUsage(testData)
		if err != nil {
			fmt.Printf("❌ 检查%d失败: 记账功能测试失败 - %v\n", checkCount, err)
		} else if result.Valid {
			passCount++
			fmt.Printf("✅ 检查%d通过: 记账功能正常\n", checkCount)

			// 导出状态变更后的新token
			stateToken, err := client.ExportStateChangedTokenEncrypted()
			if err != nil {
				log.Printf("⚠️  导出状态变更token失败: %v", err)
			} else if stateToken != "" {
				fmt.Println("   📦 状态变更后的新Token已生成")
				fmt.Printf("   Token长度: %d 字符\n", len(stateToken))

				// 保存状态变更后的token到文件
				status, err := client.GetStatus()
				if err == nil && status.LicenseCode != "" {
					timestamp := time.Now().Format("20060102150405")
					filename := fmt.Sprintf("token_state_%s_idx%d_%s.txt", status.LicenseCode, status.StateIndex, timestamp)
					err = ioutil.WriteFile(filename, []byte(stateToken), 0644)
					if err != nil {
						log.Printf("⚠️  保存token文件失败: %v", err)
					} else {
						absPath, _ := filepath.Abs(filename)
						fmt.Printf("   💾 已保存到: %s\n", absPath)
					}
				}
			}
		} else {
			fmt.Printf("❌ 检查%d失败: 记账功能异常 - %s\n", checkCount, result.ErrorMessage)
		}
	}

	// 结果汇总
	fmt.Println("\n📊 综合验证结果:")
	fmt.Printf("   总检查项: %d\n", checkCount)
	fmt.Printf("   通过项目: %d\n", passCount)
	fmt.Printf("   成功率: %.1f%%\n", float64(passCount)/float64(checkCount)*100)

	if passCount == checkCount {
		fmt.Println("🎉 所有检查均通过！系统运行正常")
	} else if passCount >= checkCount/2 {
		fmt.Println("⚠️  大部分检查通过，系统基本正常")
	} else {
		fmt.Println("❌ 多项检查失败，请检查系统配置")
	}
}

func recoveryChannelWizard(scanner *bufio.Scanner) {
	fmt.Println("\n🔑 恢复通道管理")
	fmt.Println("----------")

	client, err := getOrCreateClient()
	if err != nil {
		log.Printf("❌ 获取客户端失败: %v", err)
		return
	}

	activated, err := client.IsActivated()
	if err != nil || !activated {
		fmt.Println("❌ 请先激活令牌再管理恢复通道")
		return
	}

	fmt.Println("请选择操作:")
	fmt.Println("1. 添加恢复通道 (设置密码)")
	fmt.Println("2. 移除恢复通道")
	fmt.Println("0. 返回")
	fmt.Print("\n请选择 (0-2): ")

	if !scanner.Scan() {
		return
	}
	choice := strings.TrimSpace(scanner.Text())

	switch choice {
	case "1":
		fmt.Print("请输入恢复密码: ")
		if !scanner.Scan() {
			return
		}
		password := strings.TrimSpace(scanner.Text())
		if password == "" {
			fmt.Println("❌ 密码不能为空")
			return
		}
		result, err := client.AddRecoveryChannel(password)
		if err != nil {
			log.Printf("❌ 添加失败: %v", err)
		} else if result.Valid {
			fmt.Println("✅ 恢复通道添加成功")
		} else {
			fmt.Printf("❌ 添加失败: %s\n", result.ErrorMessage)
		}
	case "2":
		result, err := client.RemoveRecoveryChannel()
		if err != nil {
			log.Printf("❌ 移除失败: %v", err)
		} else if result.Valid {
			fmt.Println("✅ 恢复通道已移除")
		} else {
			fmt.Printf("❌ 移除失败: %s\n", result.ErrorMessage)
		}
	}
}
