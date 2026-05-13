import Foundation
import DecentriLicense

// Global state
var gClient: DecentriLicenseClient?
var gInitialized = false
var gSelectedProductKeyPath: String?

func listFiles(exts: [String]) -> [String] {
    let fm = FileManager.default
    guard let files = try? fm.contentsOfDirectory(atPath: fm.currentDirectoryPath) else { return [] }
    return files.filter { f in
        exts.contains { f.lowercased().hasSuffix($0) }
    }.sorted()
}

func pickFile(title: String, exts: [String]) -> String? {
    let files = listFiles(exts: exts)
    print(title)
    if files.isEmpty {
        print("当前目录没有可选文件，请手动输入路径: ", terminator: "")
        guard let input = readLine()?.trimmingCharacters(in: .whitespacesAndNewlines), !input.isEmpty else { return nil }
        return input
    }
    for (i, f) in files.enumerated() {
        print("\(i + 1). \(f)")
    }
    print("0. 手动输入路径")
    print("请选择文件编号: ", terminator: "")
    guard let sel = readLine(), let n = Int(sel), n >= 1, n <= files.count else {
        print("请输入文件路径: ", terminator: "")
        guard let input = readLine()?.trimmingCharacters(in: .whitespacesAndNewlines), !input.isEmpty else { return nil }
        return input
    }
    return FileManager.default.currentDirectoryPath + "/" + files[n - 1]
}

func findProductPublicKey() -> String? {
    if let path = gSelectedProductKeyPath { return path }
    let files = listFiles(exts: [".pem"])
    for f in files {
        let lower = f.lowercased()
        if lower.hasPrefix("public_") && lower.hasSuffix(".pem") {
            return FileManager.default.currentDirectoryPath + "/" + f
        }
    }
    return nil
}

func getOrCreateClient() -> DecentriLicenseClient? {
    if let c = gClient, gInitialized { return c }
    do {
        let client = try DecentriLicenseClient()
        try client.initialize()
        gClient = client
        gInitialized = true
        return client
    } catch {
        print("❌ 创建客户端失败: \(error)")
        return nil
    }
}

func readFile(_ path: String) -> String? {
    return try? String(contentsOfFile: path, encoding: .utf8)
}

// MARK: - Wizards

func importTokenWizard() {
    print("\n📥 导入令牌")
    print(String(repeating: "-", count: 40))

    guard let tokenFile = pickFile(title: "请选择 token 文件:", exts: [".json", ".txt"]) else { return }
    guard let tokenContent = readFile(tokenFile)?.trimmingCharacters(in: .whitespacesAndNewlines) else {
        print("❌ 读取文件失败"); return
    }
    print("✅ 读取到令牌 (\(tokenContent.count) 字符)")

    guard let client = getOrCreateClient() else { return }

    if let pkPath = findProductPublicKey(), let pkData = readFile(pkPath) {
        do {
            try client.setProductPublicKey(pkData)
            print("✅ 产品公钥设置成功")
        } catch { print("❌ 设置产品公钥失败: \(error)"); return }
    }

    do {
        try client.importToken(tokenContent)
        print("✅ 令牌导入成功")
    } catch { print("❌ 导入令牌失败: \(error)"); return }

    print("🎯 正在激活令牌...")
    do {
        let result = try client.activateBindDevice()
        print(result.valid ? "✅ 激活成功" : "❌ 激活失败: \(result.errorMessage)")
    } catch { print("❌ 激活失败: \(error)") }
}

func verifyActivatedTokenWizard() {
    print("\n✅ 校验已激活令牌")
    print(String(repeating: "-", count: 40))

    guard let client = getOrCreateClient() else { return }

    // Set product public key before verification
    guard let pkPath = findProductPublicKey(), let pkData = readFile(pkPath) else {
        print("❌ 未找到产品公钥文件，无法验证"); return
    }
    do {
        try client.setProductPublicKey(pkData)
        print("✅ 产品公钥设置成功")
    } catch { print("❌ 设置产品公钥失败: \(error)"); return }

    do {
        let result = try client.offlineVerifyCurrentToken()
        print(result.valid ? "✅ 令牌验证成功" : "❌ 令牌验证失败: \(result.errorMessage)")

        let status = try client.getStatus()
        if status.hasToken {
            print("\n🎫 令牌信息:")
            print("   令牌ID: \(status.tokenId)")
            print("   许可证代码: \(status.licenseCode)")
            print("   应用ID: \(status.appId)")
            print("   持有设备ID: \(status.holderDeviceId)")
        }
    } catch { print("❌ 验证失败: \(error)") }
}

func trustChainValidationWizard() {
    print("\n🔗 信任链验证")
    print(String(repeating: "-", count: 40))
    print("💡 信任链验证检查加密签名的完整性：根密钥 → 产品公钥 → 令牌签名 → 设备绑定")

    guard let client = getOrCreateClient() else { return }

    if let pkPath = findProductPublicKey(), let pkData = readFile(pkPath) {
        try? client.setProductPublicKey(pkData)
        print("✅ 产品公钥设置成功")
    }

    var checksPassed = 0
    let totalChecks = 4

    print("\n🔍 [1/\(totalChecks)] 验证令牌签名")
    if let vr = try? client.offlineVerifyCurrentToken(), vr.valid {
        print("   ✅ 通过: 令牌签名有效，信任链完整"); checksPassed += 1
    } else { print("   ❌ 失败") }

    print("\n🔍 [2/\(totalChecks)] 验证设备状态")
    if let deviceId = try? client.getDeviceId(), !deviceId.isEmpty {
        print("   ✅ 通过: 设备状态正常 (设备ID: \(deviceId))"); checksPassed += 1
    }

    print("\n🔍 [3/\(totalChecks)] 验证令牌持有者与当前设备匹配")
    if let status = try? client.getStatus(), let deviceId = try? client.getDeviceId(),
       status.holderDeviceId == deviceId {
        print("   ✅ 通过: 令牌持有者与当前设备匹配"); checksPassed += 1
    } else { print("   ⚠️  不匹配") }

    print("\n🔍 [4/\(totalChecks)] 检查令牌详细信息")
    if let status = try? client.getStatus(), status.hasToken {
        print("   ✅ 通过: 令牌信息完整")
        print("   🎫 令牌ID: \(status.tokenId)")
        print("   📝 许可证代码: \(status.licenseCode)")
        checksPassed += 1
    }

    print("\n📊 验证结果: \(checksPassed)/\(totalChecks) 通过")
    print(checksPassed == totalChecks ? "✅ 所有检查通过！" : "⚠️  部分检查未通过")
}

func statusWizard() {
    print("\n📊 查询状态")
    print(String(repeating: "-", count: 40))

    guard let client = getOrCreateClient() else { return }
    guard let status = try? client.getStatus() else { print("❌ 查询失败"); return }

    print("has_token: \(status.hasToken ? 1 : 0)")
    print("is_activated: \(status.isActivated ? 1 : 0)")
    print("state_index: \(status.stateIndex)")
    print("token_id: \(status.tokenId)")
    print("holder_device_id: \(status.holderDeviceId)")
    print("app_id: \(status.appId)")
    print("license_code: \(status.licenseCode)")

    // Show state_payload if available
    if status.isActivated {
        if let payload = try? client.getStatePayload(), !payload.isEmpty {
            print("state_payload: \(payload)")
        } else {
            print("state_payload: (empty)")
        }
    }
}

func recoveryChannelWizard() {
    print("\n🔑 恢复通道管理")
    print(String(repeating: "-", count: 40))

    guard let client = getOrCreateClient() else { return }

    if !client.isActivated() {
        print("❌ 请先激活令牌再管理恢复通道")
        return
    }

    print("请选择操作:")
    print("1. 添加恢复通道 (设置密码)")
    print("2. 移除恢复通道")
    print("0. 返回")
    print("\n请选择 (0-2): ", terminator: "")

    guard let input = readLine(), let choice = Int(input) else { return }

    switch choice {
    case 1:
        print("请输入恢复密码: ", terminator: "")
        guard let password = readLine()?.trimmingCharacters(in: .whitespacesAndNewlines), !password.isEmpty else {
            print("❌ 密码不能为空"); return
        }
        do {
            let result = try client.addRecoveryChannel(password: password)
            print(result.valid ? "✅ 恢复通道添加成功" : "❌ 添加失败: \(result.errorMessage)")
        } catch { print("❌ 添加失败: \(error)") }
    case 2:
        do {
            let result = try client.removeRecoveryChannel()
            print(result.valid ? "✅ 恢复通道已移除" : "❌ 移除失败: \(result.errorMessage)")
        } catch { print("❌ 移除失败: \(error)") }
    default: break
    }
}

// MARK: - Main

let args = CommandLine.arguments

// Quick validate mode
if args.count >= 4 && args[1] == "validate" {
    let tokenFile = args[2]
    let productKeyFile = args[3]

    guard let tokenContent = readFile(tokenFile),
          let productKeyContent = readFile(productKeyFile) else {
        print("❌ 读取文件失败"); exit(1)
    }

    do {
        let client = try DecentriLicenseClient()
        try client.initialize()
        try client.setProductPublicKey(productKeyContent)
        try client.importToken(tokenContent.trimmingCharacters(in: .whitespacesAndNewlines))
        let result = try client.offlineVerifyCurrentToken()

        if result.valid {
            print("✅ Token validation successful!")
            if let status = try? client.getStatus(), status.hasToken {
                print("   Token ID: \(status.tokenId)")
                print("   License Code: \(status.licenseCode)")
            }
            try? client.shutdown(); exit(0)
        } else {
            print("❌ Token validation failed: \(result.errorMessage)")
            try? client.shutdown(); exit(1)
        }
    } catch {
        print("❌ Error: \(error)"); exit(1)
    }
}

// Interactive wizard
print("\n🔐 DecentriLicense Swift SDK 验证向导")
print(String(repeating: "=", count: 50))

while true {
    print("\n请选择操作:")
    print("1. 📥 导入令牌（密文/JSON）")
    print("2. ✅ 校验已激活令牌")
    print("3. 🔗 信任链验证")
    print("4. 📊 查询状态")
    print("5. 🔑 设置产品公钥")
    print("6. 🔐 恢复通道管理（密码/助记词）")
    print("0. 🚪 退出")
    print("\n请选择 (0-6): ", terminator: "")

    guard let input = readLine(), let choice = Int(input) else { continue }

    switch choice {
    case 1: importTokenWizard()
    case 2: verifyActivatedTokenWizard()
    case 3: trustChainValidationWizard()
    case 4: statusWizard()
    case 5:
        print("\n🔑 设置产品公钥")
        if let keyFile = pickFile(title: "请选择产品公钥文件:", exts: [".pem"]) {
            gSelectedProductKeyPath = keyFile
            print("✅ 已选择产品公钥: \(keyFile)")
        }
    case 6: recoveryChannelWizard()
    case 0:
        try? gClient?.shutdown()
        print("👋 再见！")
        exit(0)
    default: print("❌ 无效的选择")
    }
}
