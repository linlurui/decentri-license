package main

import (
	"fmt"
	"os"
	"path/filepath"

	dc "github.com/linlurui/decentrilicense/sdks/go"
)

func main() {
	client, err := dc.NewClient()
	if err != nil {
		fmt.Printf("❌ 创建客户端失败: %v\n", err)
		os.Exit(1)
	}
	defer client.Shutdown()

	// Initialize
	config := dc.Config{
		PreferredMode: dc.ConnectionModeWANRegistry,
		UDPPort:       13325,
		TCPPort:       23325,
	}
	if err := client.Initialize(config); err != nil {
		fmt.Printf("❌ 初始化失败: %v\n", err)
		os.Exit(1)
	}
	fmt.Println("✅ 客户端初始化成功")

	// Find product public key
	cwd, _ := os.Getwd()
	keyPattern := filepath.Join(cwd, "public_*.pem")
	keyFiles, _ := filepath.Glob(keyPattern)
	if len(keyFiles) == 0 {
		fmt.Println("❌ 未找到产品公钥文件 (public_*.pem)")
		os.Exit(1)
	}
	productKeyFile := keyFiles[0]
	fmt.Printf("🔑 使用产品公钥: %s\n", filepath.Base(productKeyFile))

	// Set product public key
	keyData, err := os.ReadFile(productKeyFile)
	if err != nil {
		fmt.Printf("❌ 读取产品公钥失败: %v\n", err)
		os.Exit(1)
	}
	if err := client.SetProductPublicKey(string(keyData)); err != nil {
		fmt.Printf("❌ 设置产品公钥失败: %v\n", err)
		os.Exit(1)
	}
	fmt.Println("✅ 产品公钥设置成功")

	// Find token file
	tokenPatterns := []string{"token_*.txt", "token_*.json"}
	var tokenFile string
	for _, p := range tokenPatterns {
		files, _ := filepath.Glob(filepath.Join(cwd, p))
		if len(files) > 0 {
			tokenFile = files[0]
			break
		}
	}
	if tokenFile == "" {
		fmt.Println("❌ 未找到令牌文件 (token_*.txt 或 token_*.json)")
		os.Exit(1)
	}
	fmt.Printf("📥 使用令牌文件: %s\n", filepath.Base(tokenFile))

	// Import token
	tokenData, err := os.ReadFile(tokenFile)
	if err != nil {
		fmt.Printf("❌ 读取令牌文件失败: %v\n", err)
		os.Exit(1)
	}
	if err := client.ImportToken(string(tokenData)); err != nil {
		fmt.Printf("❌ 导入令牌失败: %v\n", err)
		os.Exit(1)
	}
	fmt.Println("✅ 令牌导入成功")

	// Activate
	fmt.Println("🎯 正在激活令牌...")
	result, err := client.ActivateBindDevice()
	if err != nil {
		fmt.Printf("❌ 激活失败: %v\n", err)
		os.Exit(1)
	}
	if result.Valid {
		fmt.Println("✅ 激活成功")
	} else {
		fmt.Printf("❌ 激活失败: %s\n", result.ErrorMessage)
		os.Exit(1)
	}

	// Verify
	fmt.Println("🔍 正在验证令牌...")
	verifyResult, err := client.OfflineVerifyCurrentToken()
	if err != nil {
		fmt.Printf("❌ 验证失败: %v\n", err)
		os.Exit(1)
	}
	if verifyResult.Valid {
		fmt.Println("✅ 令牌验证成功！")
	} else {
		fmt.Printf("❌ 令牌验证失败: %s\n", verifyResult.ErrorMessage)
		os.Exit(1)
	}

	// Get status
	status, err := client.GetStatus()
	if err == nil && status.HasToken {
		fmt.Println("\n🎫 令牌信息:")
		fmt.Printf("   令牌ID: %s\n", status.TokenID)
		fmt.Printf("   许可证代码: %s\n", status.LicenseCode)
		fmt.Printf("   应用ID: %s\n", status.AppID)
		fmt.Printf("   持有设备ID: %s\n", status.HolderDeviceID)
	}

	fmt.Println("\n✅ 示例运行完成！")
}
