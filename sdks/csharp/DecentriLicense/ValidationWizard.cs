using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

namespace DecentriLicense
{
    /// <summary>
    /// DecentriLicense C# SDK 验证向导
    /// 完全按照Go版本重写，支持8个菜单选项 (0-7)
    /// </summary>
    public class ValidationWizard
    {
        // Global client instance to maintain state across operations
        private static DecentriLicenseClient? gClient;
        private static bool gInitialized;
        private static string? gSelectedProductKeyPath;

        private static readonly JsonSerializerOptions JsonOptions = new JsonSerializerOptions
        {
            WriteIndented = true
        };

        public static async Task Main(string[] args)
        {
            Console.WriteLine("==========================================");
            Console.WriteLine("DecentriLicense C# SDK 验证向导");
            Console.WriteLine("==========================================");
            Console.WriteLine();

            // Ensure client cleanup on exit
            AppDomain.CurrentDomain.ProcessExit += (s, e) => CleanupClient();

            while (true)
            {
                PrintMenu();

                var choice = Console.ReadLine()?.Trim();

                switch (choice)
                {
                    case "0":
                        await SelectProductKeyWizard();
                        break;
                    case "1":
                        await ActivateTokenWizard();
                        break;
                    case "2":
                        await VerifyTokenWizard();
                        break;
                    case "3":
                        await ValidateTokenWizard();
                        break;
                    case "4":
                        await AccountingWizard();
                        break;
                    case "5":
                        await TrustChainValidationWizard();
                        break;
                    case "6":
                        await ComprehensiveValidationWizard();
                        break;
                    case "7":
                        Console.WriteLine("感谢使用 DecentriLicense C# SDK 验证向导!");
                        CleanupClient();
                        return;
                    default:
                        Console.WriteLine("❌ 无效选项，请重新选择");
                        break;
                }

                Console.WriteLine();
            }
        }

        private static void PrintMenu()
        {
            Console.WriteLine("请选择要执行的操作:");
            Console.WriteLine("0. 🔑 选择产品公钥");
            Console.WriteLine("1. 🔓 激活令牌");
            Console.WriteLine("2. ✅ 校验已激活令牌");
            Console.WriteLine("3. 🔍 验证令牌合法性");
            Console.WriteLine("4. 📊 记账信息");
            Console.WriteLine("5. 🔗 信任链验证");
            Console.WriteLine("6. 🎯 综合验证");
            Console.WriteLine("7. 🚪 退出");
            Console.Write("请输入选项 (0-7): ");
        }

        #region Helper Functions - File Discovery

        /// <summary>
        /// 查找当前目录及上级目录中的产品公钥文件 (public_*.pem)
        /// </summary>
        private static List<string> FindProductPublicKeys()
        {
            var candidates = new List<string>();
            var patterns = new[]
            {
                "*.pem",
                "../*.pem",
                "../../*.pem",
                "../../../dl-issuer/*.pem"
            };

            foreach (var pattern in patterns)
            {
                try
                {
                    var files = Directory.GetFiles(
                        Path.GetDirectoryName(pattern) ?? ".",
                        Path.GetFileName(pattern)
                    );
                    candidates.AddRange(files);
                }
                catch
                {
                    // Ignore directory access errors
                }
            }

            // Filter for public keys only (exclude private keys)
            var seen = new HashSet<string>();
            var unique = new List<string>();

            foreach (var file in candidates)
            {
                var filename = Path.GetFileName(file);
                if (!seen.Contains(filename) &&
                    filename.Contains("public") &&
                    !filename.Contains("private") &&
                    filename.EndsWith(".pem"))
                {
                    seen.Add(filename);
                    unique.Add(filename);
                }
            }

            unique.Sort();
            return unique;
        }

        /// <summary>
        /// 查找加密的token文件 (token_*encrypted.txt)
        /// </summary>
        private static List<string> FindEncryptedTokenFiles()
        {
            var candidates = new List<string>();
            var patterns = new[]
            {
                "token_*encrypted.txt",
                "../token_*encrypted.txt",
                "../../../dl-issuer/token_*encrypted.txt"
            };

            foreach (var pattern in patterns)
            {
                try
                {
                    var directory = Path.GetDirectoryName(pattern);
                    if (string.IsNullOrEmpty(directory)) directory = ".";

                    var searchPattern = Path.GetFileName(pattern);
                    var files = Directory.GetFiles(directory, searchPattern);
                    candidates.AddRange(files);
                }
                catch
                {
                    // Ignore errors
                }
            }

            var seen = new HashSet<string>();
            var unique = new List<string>();

            foreach (var file in candidates)
            {
                var filename = Path.GetFileName(file);
                if (!seen.Contains(filename) && filename.Contains("encrypted"))
                {
                    seen.Add(filename);
                    unique.Add(filename);
                }
            }

            unique.Sort();
            return unique;
        }

        /// <summary>
        /// 查找已激活的token文件 (token_activated_*.txt)
        /// </summary>
        private static List<string> FindActivatedTokenFiles()
        {
            var candidates = new List<string>();
            var patterns = new[]
            {
                "token_activated_*.txt",
                "../token_activated_*.txt",
                "../../../dl-issuer/token_activated_*.txt"
            };

            foreach (var pattern in patterns)
            {
                try
                {
                    var directory = Path.GetDirectoryName(pattern);
                    if (string.IsNullOrEmpty(directory)) directory = ".";

                    var searchPattern = Path.GetFileName(pattern);
                    var files = Directory.GetFiles(directory, searchPattern);
                    candidates.AddRange(files);
                }
                catch
                {
                    // Ignore errors
                }
            }

            var seen = new HashSet<string>();
            var unique = new List<string>();

            foreach (var file in candidates)
            {
                var filename = Path.GetFileName(file);
                if (!seen.Contains(filename) && filename.Contains("activated"))
                {
                    seen.Add(filename);
                    unique.Add(filename);
                }
            }

            unique.Sort();
            return unique;
        }

        /// <summary>
        /// 查找状态token文件 (token_activated_*.txt 和 token_state_*.txt)
        /// </summary>
        private static List<string> FindStateTokenFiles()
        {
            var candidates = new List<string>();
            var patterns = new[]
            {
                "token_activated_*.txt",
                "token_state_*.txt",
                "../token_activated_*.txt",
                "../token_state_*.txt",
                "../../../dl-issuer/token_activated_*.txt",
                "../../../dl-issuer/token_state_*.txt"
            };

            foreach (var pattern in patterns)
            {
                try
                {
                    var directory = Path.GetDirectoryName(pattern);
                    if (string.IsNullOrEmpty(directory)) directory = ".";

                    var searchPattern = Path.GetFileName(pattern);
                    var files = Directory.GetFiles(directory, searchPattern);
                    candidates.AddRange(files);
                }
                catch
                {
                    // Ignore errors
                }
            }

            var seen = new HashSet<string>();
            var unique = new List<string>();

            foreach (var file in candidates)
            {
                var filename = Path.GetFileName(file);
                if (!seen.Contains(filename) &&
                    (filename.Contains("activated") || filename.Contains("state")))
                {
                    seen.Add(filename);
                    unique.Add(filename);
                }
            }

            unique.Sort();
            return unique;
        }

        /// <summary>
        /// 根据文件名解析完整路径
        /// </summary>
        private static string ResolveFilePath(string filename)
        {
            var searchPaths = new[]
            {
                Path.Combine(".", filename),
                Path.Combine("..", filename),
                Path.Combine("../..", filename),
                Path.Combine("../../../dl-issuer", filename)
            };

            foreach (var path in searchPaths)
            {
                if (File.Exists(path))
                {
                    return path;
                }
            }

            return filename;
        }

        #endregion

        #region Menu Option 0: Select Product Key

        private static async Task SelectProductKeyWizard()
        {
            Console.WriteLine("\n🔑 选择产品公钥");
            Console.WriteLine("==============");

            var availableKeys = FindProductPublicKeys();

            if (availableKeys.Count == 0)
            {
                Console.WriteLine("❌ 当前目录下没有找到产品公钥文件");
                Console.WriteLine("💡 请将产品公钥文件 (public_*.pem) 放置在当前目录下");
                return;
            }

            Console.WriteLine("📄 找到以下产品公钥文件:");
            for (int i = 0; i < availableKeys.Count; i++)
            {
                Console.WriteLine($"{i + 1}. {availableKeys[i]}");
            }
            Console.WriteLine($"{availableKeys.Count + 1}. 取消选择");

            if (!string.IsNullOrEmpty(gSelectedProductKeyPath))
            {
                Console.WriteLine($"✅ 当前已选择: {gSelectedProductKeyPath}");
            }

            Console.Write($"请选择要使用的产品公钥文件 (1-{availableKeys.Count + 1}): ");

            var choice = Console.ReadLine()?.Trim();
            if (!int.TryParse(choice, out var choiceNum) ||
                choiceNum < 1 ||
                choiceNum > availableKeys.Count + 1)
            {
                Console.WriteLine("❌ 无效选择");
                return;
            }

            if (choiceNum == availableKeys.Count + 1)
            {
                gSelectedProductKeyPath = null;
                Console.WriteLine("✅ 已取消产品公钥选择");
                return;
            }

            var selectedFile = availableKeys[choiceNum - 1];
            gSelectedProductKeyPath = ResolveFilePath(selectedFile);
            Console.WriteLine($"✅ 已选择产品公钥文件: {selectedFile}");

            await Task.CompletedTask;
        }

        #endregion

        #region Menu Option 1: Activate Token

        private static async Task ActivateTokenWizard()
        {
            Console.WriteLine("\n🔓 激活令牌");
            Console.WriteLine("----------");
            Console.WriteLine("⚠️  重要说明：");
            Console.WriteLine("   • 加密token（encrypted）：首次从供应商获得，需要激活");
            Console.WriteLine("   • 已激活token（activated）：激活后生成，可直接使用，不需再次激活");
            Console.WriteLine("   ⚠️  本功能仅用于【首次激活】加密token");
            Console.WriteLine("   ⚠️  如需使用已激活token，请直接选择其他功能（如记账、验证）");
            Console.WriteLine();

            // Get or create client
            var client = GetOrCreateClient();
            if (client == null)
            {
                Console.WriteLine("❌ 创建客户端失败");
                return;
            }

            // Show available encrypted token files
            var tokenFiles = FindEncryptedTokenFiles();
            if (tokenFiles.Count > 0)
            {
                Console.WriteLine("📄 发现以下加密token文件:");
                for (int i = 0; i < tokenFiles.Count; i++)
                {
                    Console.WriteLine($"   {i + 1}. {tokenFiles[i]}");
                }
                Console.WriteLine("💡 您可以输入序号选择文件，或输入文件名/路径/token字符串");
            }

            // Get token input
            Console.WriteLine("请输入令牌字符串 (仅支持加密令牌):");
            Console.WriteLine("💡 加密令牌通常从软件提供商处获得");
            Console.WriteLine("💡 输入序号(1-N)可快速选择上面列出的文件");
            Console.WriteLine("💡 输入文件路径可读取指定文件");
            Console.Write("令牌或文件路径: ");

            var input = Console.ReadLine()?.Trim() ?? "";
            var tokenString = input;

            // Check if input is a number (file index)
            if (tokenFiles.Count > 0 && int.TryParse(input, out var index))
            {
                if (index >= 1 && index <= tokenFiles.Count)
                {
                    var selectedFile = tokenFiles[index - 1];
                    var filePath = ResolveFilePath(selectedFile);

                    try
                    {
                        tokenString = await File.ReadAllTextAsync(filePath);
                        tokenString = tokenString.Trim();
                        Console.WriteLine($"✅ 选择文件 '{selectedFile}' 并读取到令牌 ({tokenString.Length} 字符)");
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"❌ 无法读取文件 {filePath}: {ex.Message}");
                        return;
                    }
                }
            }
            // Check if input is a file path
            else if (input.Contains("/") || input.Contains("\\") ||
                     input.EndsWith(".txt") || input.Contains("token_"))
            {
                var filePath = ResolveFilePath(input);
                try
                {
                    tokenString = await File.ReadAllTextAsync(filePath);
                    tokenString = tokenString.Trim();
                    Console.WriteLine($"✅ 从文件读取到令牌 ({tokenString.Length} 字符)");
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"⚠️  无法读取文件 {filePath}: {ex.Message}");
                    Console.WriteLine("💡 将直接使用输入作为令牌字符串");
                    tokenString = input;
                }
            }

            // Initialize client if not already initialized
            if (!gInitialized)
            {
                var tempConfig = new ClientConfig
                {
                    LicenseCode = "TEMP",
                    UdpPort = 13325,
                    TcpPort = 23325,
                    RegistryServerUrl = ""
                };

                try
                {
                    client.Initialize(tempConfig);
                    Console.WriteLine("✅ 客户端初始化成功");
                    gInitialized = true;
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"⚠️  初始化失败: {ex.Message}");
                    Console.WriteLine("正在查找产品公钥文件...");
                }
            }
            else
            {
                Console.WriteLine("✅ 客户端已初始化，使用现有实例");
            }

            // Find and set product public key
            string? productKeyPath = gSelectedProductKeyPath;
            if (string.IsNullOrEmpty(productKeyPath))
            {
                var keys = FindProductPublicKeys();
                if (keys.Count > 0)
                {
                    productKeyPath = ResolveFilePath(keys[0]);
                    Console.WriteLine($"📄 使用产品公钥文件: {keys[0]}");
                }
            }
            else
            {
                Console.WriteLine($"📄 使用用户选择的产品公钥文件: {Path.GetFileName(productKeyPath)}");
            }

            if (!string.IsNullOrEmpty(productKeyPath))
            {
                try
                {
                    var productKeyData = await File.ReadAllTextAsync(productKeyPath);
                    client.SetProductPublicKey(productKeyData);
                    Console.WriteLine("✅ 产品公钥设置成功");
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"❌ 设置产品公钥失败: {ex.Message}");
                    return;
                }
            }
            else
            {
                Console.WriteLine("⚠️  未找到产品公钥文件");
                Console.WriteLine("💡 请先选择产品公钥 (菜单选项 0)，或确保当前目录下有产品公钥文件");
                return;
            }

            // Import token
            Console.WriteLine("📥 正在导入令牌...");
            try
            {
                client.ImportToken(tokenString);
                Console.WriteLine("✅ 令牌导入成功");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"❌ 令牌导入失败: {ex.Message}");
                return;
            }

            // Activate token
            Console.WriteLine("🎯 正在激活令牌...");
            try
            {
                var result = client.ActivateBindDevice();
                if (result.Valid)
                {
                    Console.WriteLine("✅ 令牌激活成功！");

                    // Export activated token
                    try
                    {
                        var activatedToken = client.ExportActivatedTokenEncrypted();
                        if (!string.IsNullOrEmpty(activatedToken))
                        {
                            Console.WriteLine("\n📦 激活后的新Token（加密）:");
                            Console.WriteLine($"   长度: {activatedToken.Length} 字符");
                            if (activatedToken.Length > 100)
                            {
                                Console.WriteLine($"   前缀: {activatedToken.Substring(0, 100)}...");
                            }
                            else
                            {
                                Console.WriteLine($"   内容: {activatedToken}");
                            }

                            // Save activated token to file
                            var status = client.GetStatus();
                            if (!string.IsNullOrEmpty(status.LicenseCode))
                            {
                                var timestamp = DateTime.Now.ToString("yyyyMMddHHmmss");
                                var filename = $"token_activated_{status.LicenseCode}_{timestamp}.txt";
                                await File.WriteAllTextAsync(filename, activatedToken);
                                var absPath = Path.GetFullPath(filename);
                                Console.WriteLine($"\n💾 已保存到文件: {absPath}");
                                Console.WriteLine("   💡 此token包含设备绑定信息，可传递给下一个设备使用");
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"⚠️  导出激活token失败: {ex.Message}");
                    }
                }
                else
                {
                    Console.WriteLine($"❌ 令牌激活失败: {result.ErrorMessage}");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"❌ 激活失败: {ex.Message}");
                return;
            }

            // Display final status
            try
            {
                var status = client.GetStatus();
                if (status.IsActivated)
                {
                    Console.WriteLine("🔍 当前状态: 已激活");
                    if (status.HasToken)
                    {
                        Console.WriteLine($"🎫 令牌ID: {status.TokenId}");
                        Console.WriteLine($"📝 许可证代码: {status.LicenseCode}");
                        Console.WriteLine($"👤 持有设备: {status.HolderDeviceId}");
                        var issueTime = DateTimeOffset.FromUnixTimeSeconds(status.IssueTime);
                        Console.WriteLine($"📅 颁发时间: {issueTime:yyyy-MM-dd HH:mm:ss}");
                    }
                }
                else
                {
                    Console.WriteLine("🔍 当前状态: 未激活");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"⚠️  获取状态失败: {ex.Message}");
            }
        }

        #endregion

        #region Menu Option 2: Verify Activated Token

        private static async Task VerifyTokenWizard()
        {
            Console.WriteLine("\n✅ 校验已激活令牌");
            Console.WriteLine("----------------");

            // Scan for activated tokens in state directory
            var stateDir = ".decentrilicense_state";
            if (!Directory.Exists(stateDir))
            {
                Console.WriteLine("⚠️  没有找到已激活的令牌");
                return;
            }

            var entries = Directory.GetDirectories(stateDir);
            if (entries.Length == 0)
            {
                Console.WriteLine("⚠️  没有找到已激活的令牌");
                return;
            }

            // List all activated tokens
            var activatedTokens = new List<string>();
            Console.WriteLine("\n📋 已激活的令牌列表:");
            for (int i = 0; i < entries.Length; i++)
            {
                var dirName = Path.GetFileName(entries[i]);
                activatedTokens.Add(dirName);

                var stateFile = Path.Combine(entries[i], "current_state.json");
                if (File.Exists(stateFile))
                {
                    Console.WriteLine($"{i + 1}. {dirName} ✅");
                }
                else
                {
                    Console.WriteLine($"{i + 1}. {dirName} ⚠️  (无状态文件)");
                }
            }

            // User selection
            Console.Write($"\n请选择要验证的令牌 (1-{activatedTokens.Count}): ");
            var choice = Console.ReadLine()?.Trim();

            if (!int.TryParse(choice, out var index) || index < 1 || index > activatedTokens.Count)
            {
                Console.WriteLine("❌ 无效的选择");
                return;
            }

            var selectedLicenseCode = activatedTokens[index - 1];
            Console.WriteLine($"\n🔍 正在验证令牌: {selectedLicenseCode}");

            var client = GetOrCreateClient();
            if (client == null)
            {
                Console.WriteLine("❌ 获取客户端失败");
                return;
            }

            // Check if this is the currently activated token
            try
            {
                var status = client.GetStatus();
                if (status.LicenseCode == selectedLicenseCode)
                {
                    // This is the current token, verify it
                    Console.WriteLine("🔍 正在验证令牌...");
                    var result = client.OfflineVerifyCurrentToken();

                    if (result.Valid)
                    {
                        Console.WriteLine("✅ 令牌验证成功");
                        if (!string.IsNullOrEmpty(result.ErrorMessage))
                        {
                            Console.WriteLine($"📄 信息: {result.ErrorMessage}");
                        }
                    }
                    else
                    {
                        Console.WriteLine("❌ 令牌验证失败");
                        Console.WriteLine($"📄 错误信息: {result.ErrorMessage}");
                    }

                    // Display token information
                    if (status.HasToken)
                    {
                        Console.WriteLine("\n🎫 令牌信息:");
                        Console.WriteLine($"   令牌ID: {status.TokenId}");
                        Console.WriteLine($"   许可证代码: {status.LicenseCode}");
                        Console.WriteLine($"   应用ID: {status.AppId}");
                        Console.WriteLine($"   持有设备ID: {status.HolderDeviceId}");

                        var issueTime = DateTimeOffset.FromUnixTimeSeconds(status.IssueTime);
                        Console.WriteLine($"   颁发时间: {issueTime:yyyy-MM-dd HH:mm:ss}");

                        if (status.ExpireTime == 0)
                        {
                            Console.WriteLine("   到期时间: 永不过期");
                        }
                        else
                        {
                            var expireTime = DateTimeOffset.FromUnixTimeSeconds(status.ExpireTime);
                            Console.WriteLine($"   到期时间: {expireTime:yyyy-MM-dd HH:mm:ss}");
                        }

                        Console.WriteLine($"   状态索引: {status.StateIndex}");
                        Console.WriteLine($"   激活状态: {status.IsActivated}");
                    }
                }
                else
                {
                    // Not the current token, show saved state info
                    Console.WriteLine("💡 此令牌不是当前激活的令牌，显示已保存的状态信息:");
                    var stateFile = Path.Combine(stateDir, selectedLicenseCode, "current_state.json");

                    if (File.Exists(stateFile))
                    {
                        var data = await File.ReadAllTextAsync(stateFile);
                        Console.WriteLine("\n🎫 令牌信息 (从状态文件读取):");
                        Console.WriteLine($"   许可证代码: {selectedLicenseCode}");
                        Console.WriteLine($"   状态文件: {stateFile}");
                        Console.WriteLine($"   文件大小: {data.Length} 字节");
                        Console.WriteLine("\n💡 提示: 如需完整验证此令牌，请使用选项1重新激活");
                    }
                    else
                    {
                        Console.WriteLine("❌ 读取状态文件失败");
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"❌ 验证失败: {ex.Message}");
            }
        }

        #endregion

        #region Menu Option 3: Validate Token Legitimacy

        private static async Task ValidateTokenWizard()
        {
            Console.WriteLine("\n🔍 验证令牌合法性");
            Console.WriteLine("----------------");

            var client = GetOrCreateClient();
            if (client == null)
            {
                Console.WriteLine("❌ 获取客户端失败");
                return;
            }

            // Initialize client if not already initialized
            if (!gInitialized)
            {
                var config = new ClientConfig
                {
                    LicenseCode = "VALIDATE",
                    UdpPort = 13325,
                    TcpPort = 23325,
                    RegistryServerUrl = ""
                };

                try
                {
                    client.Initialize(config);
                    Console.WriteLine("✅ 客户端初始化成功");
                    gInitialized = true;
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"⚠️  初始化失败: {ex.Message}");
                }
            }

            // Find and set product public key
            string? productKeyPath = gSelectedProductKeyPath;
            if (string.IsNullOrEmpty(productKeyPath))
            {
                var keys = FindProductPublicKeys();
                if (keys.Count > 0)
                {
                    productKeyPath = ResolveFilePath(keys[0]);
                    Console.WriteLine($"📄 使用产品公钥文件: {keys[0]}");
                }
            }
            else
            {
                Console.WriteLine($"📄 使用用户选择的产品公钥文件: {Path.GetFileName(productKeyPath)}");
            }

            if (!string.IsNullOrEmpty(productKeyPath))
            {
                try
                {
                    var productKeyData = await File.ReadAllTextAsync(productKeyPath);
                    client.SetProductPublicKey(productKeyData);
                    Console.WriteLine("✅ 产品公钥设置成功");
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"❌ 设置产品公钥失败: {ex.Message}");
                    return;
                }
            }
            else
            {
                Console.WriteLine("⚠️  未找到产品公钥文件");
                Console.WriteLine("💡 请先选择产品公钥 (菜单选项 0)，或确保当前目录下有产品公钥文件");
                return;
            }

            // Show available encrypted token files
            var tokenFiles = FindEncryptedTokenFiles();
            if (tokenFiles.Count > 0)
            {
                Console.WriteLine("📄 发现以下加密token文件:");
                for (int i = 0; i < tokenFiles.Count; i++)
                {
                    Console.WriteLine($"   {i + 1}. {tokenFiles[i]}");
                }
                Console.WriteLine("💡 您可以输入序号选择文件，或输入文件名/路径/token字符串");
            }

            // Get token input
            Console.WriteLine("请输入要验证的令牌字符串 (支持加密令牌):");
            Console.WriteLine("💡 令牌通常从软件提供商处获得，或从加密令牌文件读取");
            Console.WriteLine("💡 如果是文件路径，请输入完整的文件路径");
            Console.Write("令牌或文件路径: ");

            var input = Console.ReadLine()?.Trim() ?? "";
            var tokenString = input;

            // Check if input is a number (file selection)
            if (tokenFiles.Count > 0 && int.TryParse(input, out var numChoice) &&
                numChoice >= 1 && numChoice <= tokenFiles.Count)
            {
                var selectedFile = tokenFiles[numChoice - 1];
                var filePath = ResolveFilePath(selectedFile);

                try
                {
                    tokenString = await File.ReadAllTextAsync(filePath);
                    tokenString = tokenString.Trim();
                    Console.WriteLine($"✅ 从文件 '{selectedFile}' 读取到令牌 ({tokenString.Length} 字符)");
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"❌ 无法读取文件 {filePath}: {ex.Message}");
                    return;
                }
            }
            else if (input.Contains("/") || input.Contains("\\") ||
                     input.EndsWith(".txt") || input.Contains("token_"))
            {
                var filePath = ResolveFilePath(input);
                try
                {
                    tokenString = await File.ReadAllTextAsync(filePath);
                    tokenString = tokenString.Trim();
                    Console.WriteLine($"✅ 从文件读取到令牌 ({tokenString.Length} 字符)");
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"⚠️  无法读取文件 {filePath}: {ex.Message}");
                    Console.WriteLine("💡 将直接使用输入作为令牌字符串");
                    tokenString = input;
                }
            }

            // Import and validate token
            Console.WriteLine("🔍 正在验证令牌合法性...");
            try
            {
                client.ImportToken(tokenString);
                var result = client.OfflineVerifyCurrentToken();

                if (result.Valid)
                {
                    Console.WriteLine("✅ 令牌验证成功 - 令牌合法且有效");
                    if (!string.IsNullOrEmpty(result.ErrorMessage))
                    {
                        Console.WriteLine($"📄 详细信息: {result.ErrorMessage}");
                    }
                }
                else
                {
                    Console.WriteLine("❌ 令牌验证失败 - 令牌不合法或无效");
                    if (!string.IsNullOrEmpty(result.ErrorMessage))
                    {
                        Console.WriteLine($"📄 错误信息: {result.ErrorMessage}");
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"❌ 令牌验证失败: {ex.Message}");
            }
        }

        #endregion

        #region Menu Option 4: Accounting Information

        private static async Task AccountingWizard()
        {
            Console.WriteLine("\n📊 记账信息");
            Console.WriteLine("----------");

            var client = GetOrCreateClient();
            if (client == null)
            {
                Console.WriteLine("❌ 获取客户端失败");
                return;
            }

            // Show available state token files
            var tokenFiles = FindStateTokenFiles();

            // Check activation status
            bool activated = false;
            try
            {
                var status = client.GetStatus();
                activated = status.IsActivated;
            }
            catch
            {
                // Ignore
            }

            // Show token selection options
            Console.WriteLine("\n💡 请选择令牌来源:");
            if (activated)
            {
                Console.WriteLine("0. 使用当前激活的令牌");
            }

            if (tokenFiles.Count > 0)
            {
                Console.WriteLine("\n📄 或从以下文件加载令牌:");
                for (int i = 0; i < tokenFiles.Count; i++)
                {
                    Console.WriteLine($"{i + 1}. {tokenFiles[i]}");
                }
            }

            if (!activated && tokenFiles.Count == 0)
            {
                Console.WriteLine("❌ 当前没有激活的令牌，也没有找到可用的token文件");
                Console.WriteLine("💡 请先使用选项1激活令牌");
                return;
            }

            Console.Write($"\n请选择 (0-{tokenFiles.Count}): ");
            var tokenChoice = Console.ReadLine()?.Trim();

            if (!int.TryParse(tokenChoice, out var tokenChoiceNum) ||
                tokenChoiceNum < 0 || tokenChoiceNum > tokenFiles.Count)
            {
                Console.WriteLine("❌ 无效的选择");
                return;
            }

            // Load token from file if selected
            if (tokenChoiceNum > 0)
            {
                var selectedFile = tokenFiles[tokenChoiceNum - 1];
                var filePath = ResolveFilePath(selectedFile);

                Console.WriteLine($"📂 正在从文件加载令牌: {selectedFile}");

                try
                {
                    var tokenData = await File.ReadAllTextAsync(filePath);
                    var tokenString = tokenData.Trim();
                    Console.WriteLine($"✅ 读取到令牌 ({tokenString.Length} 字符)");

                    // Initialize if not already
                    if (!gInitialized)
                    {
                        var tempConfig = new ClientConfig
                        {
                            LicenseCode = "ACCOUNTING",
                            UdpPort = 13325,
                            TcpPort = 23325,
                            RegistryServerUrl = ""
                        };
                        client.Initialize(tempConfig);
                        gInitialized = true;
                    }

                    // Set product public key
                    string? productKeyPath = gSelectedProductKeyPath;
                    if (string.IsNullOrEmpty(productKeyPath))
                    {
                        var keys = FindProductPublicKeys();
                        if (keys.Count > 0)
                        {
                            productKeyPath = ResolveFilePath(keys[0]);
                        }
                    }

                    if (!string.IsNullOrEmpty(productKeyPath))
                    {
                        var productKeyData = await File.ReadAllTextAsync(productKeyPath);
                        client.SetProductPublicKey(productKeyData);
                        Console.WriteLine("✅ 产品公钥设置成功");
                    }

                    // Import token
                    Console.WriteLine("📥 正在导入令牌...");
                    client.ImportToken(tokenString);
                    Console.WriteLine("✅ 令牌导入成功");

                    // Check if already activated
                    var isAlreadyActivated = selectedFile.Contains("activated") ||
                                           selectedFile.Contains("state");

                    if (isAlreadyActivated)
                    {
                        Console.WriteLine("💡 检测到已激活令牌");
                        Console.WriteLine("🔄 正在恢复激活状态...");
                    }
                    else
                    {
                        Console.WriteLine("🎯 正在首次激活令牌...");
                    }

                    var result = client.ActivateBindDevice();
                    if (!result.Valid)
                    {
                        Console.WriteLine($"❌ 激活失败: {result.ErrorMessage}");
                        return;
                    }

                    if (isAlreadyActivated)
                    {
                        Console.WriteLine("✅ 激活状态已恢复（token未改变）");
                    }
                    else
                    {
                        Console.WriteLine("✅ 首次激活成功");
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"❌ 加载令牌失败: {ex.Message}");
                    return;
                }
            }

            // Display current token info
            try
            {
                var status = client.GetStatus();
                if (status.HasToken)
                {
                    Console.WriteLine("\n📋 当前令牌信息:");
                    Console.WriteLine($"   许可证代码: {status.LicenseCode}");
                    Console.WriteLine($"   应用ID: {status.AppId}");
                    Console.WriteLine($"   当前状态索引: {status.StateIndex}");
                    Console.WriteLine($"   令牌ID: {status.TokenId}");
                }
                else
                {
                    Console.WriteLine("⚠️  无法获取令牌信息");
                    return;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"⚠️  无法获取令牌信息: {ex.Message}");
                return;
            }

            // Provide accounting options
            Console.WriteLine("\n💡 请选择记账方式:");
            Console.WriteLine("1. 快速测试记账（使用默认测试数据）");
            Console.WriteLine("2. 记录业务操作（向导式输入）");
            Console.Write("\n请选择 (1-2): ");

            var choice = Console.ReadLine()?.Trim();

            string action;
            Dictionary<string, object> parameters;

            switch (choice)
            {
                case "1":
                    // Quick test
                    action = "api_call";
                    parameters = new Dictionary<string, object>
                    {
                        { "function", "test_function" },
                        { "result", "success" }
                    };
                    Console.WriteLine($"💡 使用测试数据: action={action}, params={JsonSerializer.Serialize(parameters)}");
                    break;

                case "2":
                    // Guided input
                    Console.WriteLine("\n📝 usage_chain 结构说明:");
                    Console.WriteLine("┌─────────────────────────────────────────────────────────┐");
                    Console.WriteLine("│ 字段名      │ 说明           │ 填写方式              │");
                    Console.WriteLine("├─────────────────────────────────────────────────────────┤");
                    Console.WriteLine("│ seq         │ 序列号         │ ✅ 系统自动填充       │");
                    Console.WriteLine("│ time        │ 时间戳         │ ✅ 系统自动填充       │");
                    Console.WriteLine("│ action      │ 操作类型       │ 👉 需要您输入         │");
                    Console.WriteLine("│ params      │ 操作参数       │ 👉 需要您输入         │");
                    Console.WriteLine("│ hash_prev   │ 前状态哈希     │ ✅ 系统自动填充       │");
                    Console.WriteLine("│ signature   │ 数字签名       │ ✅ 系统自动填充       │");
                    Console.WriteLine("└─────────────────────────────────────────────────────────┘");

                    Console.WriteLine("\n👉 第1步: 输入操作类型 (action)");
                    Console.WriteLine("   常用操作类型:");
                    Console.WriteLine("   • api_call      - API调用");
                    Console.WriteLine("   • feature_usage - 功能使用");
                    Console.WriteLine("   • save_file     - 保存文件");
                    Console.WriteLine("   • export_data   - 导出数据");
                    Console.Write("\n请输入操作类型: ");

                    action = Console.ReadLine()?.Trim() ?? "";
                    if (string.IsNullOrEmpty(action))
                    {
                        Console.WriteLine("❌ 操作类型不能为空");
                        return;
                    }

                    Console.WriteLine("\n👉 第2步: 输入操作参数 (params)");
                    Console.WriteLine("   params 是一个JSON对象，包含操作的具体参数");
                    Console.WriteLine("   输入格式: key=value (每行一个)");
                    Console.WriteLine("   示例:");
                    Console.WriteLine("   • function=process_image");
                    Console.WriteLine("   • file_name=report.pdf");
                    Console.WriteLine("   • size=1024");
                    Console.WriteLine("   输入空行结束输入");

                    parameters = new Dictionary<string, object>();
                    while (true)
                    {
                        Console.Write("参数 (key=value 或直接回车结束): ");
                        var line = Console.ReadLine()?.Trim() ?? "";
                        if (string.IsNullOrEmpty(line))
                        {
                            break;
                        }

                        var parts = line.Split('=', 2);
                        if (parts.Length == 2)
                        {
                            var key = parts[0].Trim();
                            var value = parts[1].Trim();
                            parameters[key] = value;
                        }
                        else
                        {
                            Console.WriteLine("⚠️  格式错误,请使用 key=value 格式");
                        }
                    }

                    if (parameters.Count == 0)
                    {
                        Console.WriteLine("⚠️  未输入任何参数,将使用空参数对象");
                        parameters = new Dictionary<string, object>();
                    }
                    break;

                default:
                    Console.WriteLine("❌ 无效的选择");
                    return;
            }

            // Build usage chain entry
            var usageChainEntry = new Dictionary<string, object>
            {
                { "action", action },
                { "params", parameters }
            };

            var accountingData = JsonSerializer.Serialize(usageChainEntry);
            Console.WriteLine($"\n📝 记账数据 (业务字段): {accountingData}");
            Console.WriteLine("   (系统字段 seq, time, hash_prev, signature 将由SDK自动添加)");

            // Record usage
            Console.WriteLine("📝 正在记录使用情况...");
            try
            {
                var result = client.RecordUsage(accountingData);
                if (result.Valid)
                {
                    Console.WriteLine("✅ 记账成功");
                    Console.WriteLine($"📄 响应: {result.ErrorMessage}");

                    // Export state changed token
                    try
                    {
                        var stateToken = client.ExportStateChangedTokenEncrypted();
                        if (!string.IsNullOrEmpty(stateToken))
                        {
                            Console.WriteLine("\n📦 状态变更后的新Token（加密）:");
                            Console.WriteLine($"   长度: {stateToken.Length} 字符");
                            if (stateToken.Length > 100)
                            {
                                Console.WriteLine($"   前缀: {stateToken.Substring(0, 100)}...");
                            }
                            else
                            {
                                Console.WriteLine($"   内容: {stateToken}");
                            }

                            // Save state changed token to file
                            var status = client.GetStatus();
                            if (!string.IsNullOrEmpty(status.LicenseCode))
                            {
                                var timestamp = DateTime.Now.ToString("yyyyMMddHHmmss");
                                var filename = $"token_state_{status.LicenseCode}_idx{status.StateIndex}_{timestamp}.txt";
                                await File.WriteAllTextAsync(filename, stateToken);
                                var absPath = Path.GetFullPath(filename);
                                Console.WriteLine($"\n💾 已保存到文件: {absPath}");
                                Console.WriteLine("   💡 此token包含最新状态链，可传递给下一个设备使用");
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"⚠️  导出状态变更token失败: {ex.Message}");
                    }
                }
                else
                {
                    Console.WriteLine("❌ 记账失败");
                    Console.WriteLine($"📄 错误信息: {result.ErrorMessage}");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"❌ 记账失败: {ex.Message}");
            }
        }

        #endregion

        #region Menu Option 5: Trust Chain Verification

        private static async Task TrustChainValidationWizard()
        {
            Console.WriteLine("\n🔗 信任链验证");
            Console.WriteLine("============");
            Console.WriteLine("💡 信任链验证检查加密签名的完整性：根密钥 → 产品公钥 → 令牌签名 → 设备绑定");
            Console.WriteLine();

            var client = GetOrCreateClient();
            if (client == null)
            {
                Console.WriteLine("❌ 获取客户端失败");
                return;
            }

            // Show available token files
            var tokenFiles = FindStateTokenFiles();

            // Check activation status
            bool activated = false;
            try
            {
                var status = client.GetStatus();
                activated = status.IsActivated;
            }
            catch
            {
                // Ignore
            }

            // Show token selection options
            Console.WriteLine("\n💡 请选择令牌来源:");
            if (activated)
            {
                Console.WriteLine("0. 使用当前激活的令牌");
            }

            if (tokenFiles.Count > 0)
            {
                Console.WriteLine("\n📄 或从以下文件加载令牌:");
                for (int i = 0; i < tokenFiles.Count; i++)
                {
                    Console.WriteLine($"{i + 1}. {tokenFiles[i]}");
                }
            }

            if (!activated && tokenFiles.Count == 0)
            {
                Console.WriteLine("❌ 当前没有激活的令牌，也没有找到可用的token文件");
                Console.WriteLine("💡 请先使用选项1激活令牌");
                return;
            }

            Console.Write($"\n请选择 (0-{tokenFiles.Count}): ");
            var tokenChoice = Console.ReadLine()?.Trim();

            if (!int.TryParse(tokenChoice, out var tokenChoiceNum) ||
                tokenChoiceNum < 0 || tokenChoiceNum > tokenFiles.Count)
            {
                Console.WriteLine("❌ 无效的选择");
                return;
            }

            // Load token from file if selected
            if (tokenChoiceNum > 0)
            {
                var selectedFile = tokenFiles[tokenChoiceNum - 1];
                var filePath = ResolveFilePath(selectedFile);

                Console.WriteLine($"📂 正在从文件加载令牌: {selectedFile}");

                try
                {
                    var tokenData = await File.ReadAllTextAsync(filePath);
                    var tokenString = tokenData.Trim();
                    Console.WriteLine($"✅ 读取到令牌 ({tokenString.Length} 字符)");

                    // Initialize if not already
                    if (!gInitialized)
                    {
                        var tempConfig = new ClientConfig
                        {
                            LicenseCode = "TRUST_CHAIN",
                            UdpPort = 13325,
                            TcpPort = 23325,
                            RegistryServerUrl = ""
                        };
                        client.Initialize(tempConfig);
                        gInitialized = true;
                    }

                    // Set product public key
                    string? productKeyPath = gSelectedProductKeyPath;
                    if (string.IsNullOrEmpty(productKeyPath))
                    {
                        var keys = FindProductPublicKeys();
                        if (keys.Count > 0)
                        {
                            productKeyPath = ResolveFilePath(keys[0]);
                        }
                    }

                    if (!string.IsNullOrEmpty(productKeyPath))
                    {
                        var productKeyData = await File.ReadAllTextAsync(productKeyPath);
                        client.SetProductPublicKey(productKeyData);
                        Console.WriteLine("✅ 产品公钥设置成功");
                    }

                    // Import token
                    Console.WriteLine("📥 正在导入令牌...");
                    client.ImportToken(tokenString);
                    Console.WriteLine("✅ 令牌导入成功");

                    // Activate/restore
                    var isAlreadyActivated = selectedFile.Contains("activated") ||
                                           selectedFile.Contains("state");

                    if (isAlreadyActivated)
                    {
                        Console.WriteLine("💡 检测到已激活令牌");
                        Console.WriteLine("🔄 正在恢复激活状态...");
                    }
                    else
                    {
                        Console.WriteLine("🎯 正在首次激活令牌...");
                    }

                    var result = client.ActivateBindDevice();
                    if (!result.Valid)
                    {
                        Console.WriteLine($"❌ 激活失败: {result.ErrorMessage}");
                        return;
                    }

                    if (isAlreadyActivated)
                    {
                        Console.WriteLine("✅ 激活状态已恢复（token未改变）");
                    }
                    else
                    {
                        Console.WriteLine("✅ 首次激活成功");
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"❌ 加载令牌失败: {ex.Message}");
                    return;
                }
            }

            Console.WriteLine("📋 开始验证信任链...");
            Console.WriteLine();

            int checksPassed = 0;
            const int totalChecks = 4;

            // Check 1: Token signature verification
            Console.WriteLine("🔍 [1/4] 验证令牌签名（根密钥 → 产品公钥 → 令牌）");
            try
            {
                var result = client.OfflineVerifyCurrentToken();
                if (result.Valid)
                {
                    Console.WriteLine("   ✅ 通过: 令牌签名有效，信任链完整");
                    checksPassed++;
                }
                else
                {
                    Console.WriteLine($"   ❌ 失败: {result.ErrorMessage}");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"   ❌ 失败: {ex.Message}");
            }
            Console.WriteLine();

            // Check 2: Device state verification
            Console.WriteLine("🔍 [2/4] 验证设备状态");
            try
            {
                var deviceId = client.GetDeviceId();
                if (!string.IsNullOrEmpty(deviceId))
                {
                    Console.WriteLine($"   ✅ 通过: 设备状态正常 (设备ID: {deviceId})");
                    checksPassed++;
                }
                else
                {
                    Console.WriteLine("   ⚠️  警告: 无法获取设备ID");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"   ⚠️  警告: 无法获取设备状态 - {ex.Message}");
            }
            Console.WriteLine();

            // Check 3: Token holder matching device
            Console.WriteLine("🔍 [3/4] 验证令牌持有者与当前设备匹配");
            try
            {
                var status = client.GetStatus();
                var deviceId = client.GetDeviceId();

                if (status.HolderDeviceId == deviceId)
                {
                    Console.WriteLine("   ✅ 通过: 令牌持有者与当前设备匹配");
                    Console.WriteLine($"   📱 设备ID: {deviceId}");
                    checksPassed++;
                }
                else
                {
                    Console.WriteLine("   ⚠️  不匹配: 令牌持有者与当前设备不一致");
                    Console.WriteLine($"   📱 当前设备ID: {deviceId}");
                    Console.WriteLine($"   🎫 令牌持有者ID: {status.HolderDeviceId}");
                    Console.WriteLine("   💡 这可能表示令牌是从其他设备导入的");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"   ⚠️  警告: 无法获取令牌信息 - {ex.Message}");
            }
            Console.WriteLine();

            // Check 4: Token detailed information
            Console.WriteLine("🔍 [4/4] 检查令牌详细信息");
            try
            {
                var status = client.GetStatus();
                if (status.HasToken)
                {
                    Console.WriteLine("   ✅ 通过: 令牌信息完整");
                    Console.WriteLine($"   🎫 令牌ID: {status.TokenId}");
                    Console.WriteLine($"   📝 许可证代码: {status.LicenseCode}");
                    Console.WriteLine($"   📱 应用ID: {status.AppId}");

                    var issueTime = DateTimeOffset.FromUnixTimeSeconds(status.IssueTime);
                    Console.WriteLine($"   📅 颁发时间: {issueTime:yyyy-MM-dd HH:mm:ss}");

                    if (status.ExpireTime == 0)
                    {
                        Console.WriteLine("   ⏰ 到期时间: 永不过期");
                    }
                    else
                    {
                        var expireTime = DateTimeOffset.FromUnixTimeSeconds(status.ExpireTime);
                        Console.WriteLine($"   ⏰ 到期时间: {expireTime:yyyy-MM-dd HH:mm:ss}");
                    }
                    checksPassed++;
                }
                else
                {
                    Console.WriteLine("   ⚠️  警告: 令牌信息不完整");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"   ⚠️  警告: 无法获取状态信息 - {ex.Message}");
            }
            Console.WriteLine();

            // Summary
            Console.WriteLine("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
            Console.WriteLine($"📊 验证结果: {checksPassed}/{totalChecks} 项检查通过");
            if (checksPassed == totalChecks)
            {
                Console.WriteLine("🎉 信任链验证完全通过！令牌可信且安全");
            }
            else if (checksPassed >= 2)
            {
                Console.WriteLine("⚠️  部分检查通过，令牌基本可用但存在警告");
            }
            else
            {
                Console.WriteLine("❌ 多项检查失败，请检查令牌和设备状态");
            }
            Console.WriteLine("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        }

        #endregion

        #region Menu Option 6: Comprehensive Verification

        private static async Task ComprehensiveValidationWizard()
        {
            Console.WriteLine("\n🎯 综合验证");
            Console.WriteLine("----------");

            var client = GetOrCreateClient();
            if (client == null)
            {
                Console.WriteLine("❌ 获取客户端失败");
                return;
            }

            // Show available token files
            var tokenFiles = FindStateTokenFiles();

            // Check activation status
            bool activated = false;
            try
            {
                var status = client.GetStatus();
                activated = status.IsActivated;
            }
            catch
            {
                // Ignore
            }

            // Show token selection options
            Console.WriteLine("\n💡 请选择令牌来源:");
            if (activated)
            {
                Console.WriteLine("0. 使用当前激活的令牌");
            }

            if (tokenFiles.Count > 0)
            {
                Console.WriteLine("\n📄 或从以下文件加载令牌:");
                for (int i = 0; i < tokenFiles.Count; i++)
                {
                    Console.WriteLine($"{i + 1}. {tokenFiles[i]}");
                }
            }

            if (!activated && tokenFiles.Count == 0)
            {
                Console.WriteLine("❌ 当前没有激活的令牌，也没有找到可用的token文件");
                Console.WriteLine("💡 请先使用选项1激活令牌");
                return;
            }

            Console.Write($"\n请选择 (0-{tokenFiles.Count}): ");
            var tokenChoice = Console.ReadLine()?.Trim();

            if (!int.TryParse(tokenChoice, out var tokenChoiceNum) ||
                tokenChoiceNum < 0 || tokenChoiceNum > tokenFiles.Count)
            {
                Console.WriteLine("❌ 无效的选择");
                return;
            }

            // Load token from file if selected
            if (tokenChoiceNum > 0)
            {
                var selectedFile = tokenFiles[tokenChoiceNum - 1];
                var filePath = ResolveFilePath(selectedFile);

                Console.WriteLine($"📂 正在从文件加载令牌: {selectedFile}");

                try
                {
                    var tokenData = await File.ReadAllTextAsync(filePath);
                    var tokenString = tokenData.Trim();
                    Console.WriteLine($"✅ 读取到令牌 ({tokenString.Length} 字符)");

                    // Initialize if not already
                    if (!gInitialized)
                    {
                        var tempConfig = new ClientConfig
                        {
                            LicenseCode = "COMPREHENSIVE",
                            UdpPort = 13325,
                            TcpPort = 23325,
                            RegistryServerUrl = ""
                        };
                        client.Initialize(tempConfig);
                        gInitialized = true;
                    }

                    // Set product public key
                    string? productKeyPath = gSelectedProductKeyPath;
                    if (string.IsNullOrEmpty(productKeyPath))
                    {
                        var keys = FindProductPublicKeys();
                        if (keys.Count > 0)
                        {
                            productKeyPath = ResolveFilePath(keys[0]);
                        }
                    }

                    if (!string.IsNullOrEmpty(productKeyPath))
                    {
                        var productKeyData = await File.ReadAllTextAsync(productKeyPath);
                        client.SetProductPublicKey(productKeyData);
                        Console.WriteLine("✅ 产品公钥设置成功");
                    }

                    // Import token
                    Console.WriteLine("📥 正在导入令牌...");
                    client.ImportToken(tokenString);
                    Console.WriteLine("✅ 令牌导入成功");

                    // Activate/restore
                    var isAlreadyActivated = selectedFile.Contains("activated") ||
                                           selectedFile.Contains("state");

                    if (isAlreadyActivated)
                    {
                        Console.WriteLine("💡 检测到已激活令牌");
                        Console.WriteLine("🔄 正在恢复激活状态...");
                    }
                    else
                    {
                        Console.WriteLine("🎯 正在首次激活令牌...");
                    }

                    var result = client.ActivateBindDevice();
                    if (!result.Valid)
                    {
                        Console.WriteLine($"❌ 激活失败: {result.ErrorMessage}");
                        return;
                    }

                    if (isAlreadyActivated)
                    {
                        Console.WriteLine("✅ 激活状态已恢复（token未改变）");
                    }
                    else
                    {
                        Console.WriteLine("✅ 首次激活成功");
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"❌ 加载令牌失败: {ex.Message}");
                    return;
                }
            }

            Console.WriteLine("📋 执行综合验证流程...");
            int checkCount = 0;
            int passCount = 0;

            // Check 1: Initialized
            checkCount++;
            if (gInitialized)
            {
                passCount++;
                Console.WriteLine($"✅ 检查{checkCount}通过: 客户端已初始化");
            }
            else
            {
                Console.WriteLine($"❌ 检查{checkCount}失败: 客户端未初始化");
            }

            // Check 2: Has token
            checkCount++;
            try
            {
                var status = client.GetStatus();
                if (status.HasToken)
                {
                    passCount++;
                    Console.WriteLine($"✅ 检查{checkCount}通过: 已加载令牌");
                }
                else
                {
                    Console.WriteLine($"❌ 检查{checkCount}失败: 未加载令牌");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"❌ 检查{checkCount}失败: {ex.Message}");
            }

            // Check 3: Activation status
            checkCount++;
            try
            {
                var status = client.GetStatus();
                if (status.IsActivated)
                {
                    passCount++;
                    Console.WriteLine($"✅ 检查{checkCount}通过: 许可证已激活");
                }
                else
                {
                    Console.WriteLine($"⚠️  检查{checkCount}: 许可证未激活");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"❌ 检查{checkCount}失败: {ex.Message}");
            }

            // Check 4: Offline verification
            checkCount++;
            try
            {
                var result = client.OfflineVerifyCurrentToken();
                if (result.Valid)
                {
                    passCount++;
                    Console.WriteLine($"✅ 检查{checkCount}通过: 离线验证成功");
                }
                else
                {
                    Console.WriteLine($"❌ 检查{checkCount}失败: 离线验证失败 - {result.ErrorMessage}");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"❌ 检查{checkCount}失败: {ex.Message}");
            }

            // Check 5: Test accounting functionality
            checkCount++;
            try
            {
                var testData = "{\"action\":\"comprehensive_test\",\"timestamp\":1234567890}";
                var result = client.RecordUsage(testData);
                if (result.Valid)
                {
                    passCount++;
                    Console.WriteLine($"✅ 检查{checkCount}通过: 记账功能正常");

                    // Export state changed token
                    try
                    {
                        var stateToken = client.ExportStateChangedTokenEncrypted();
                        if (!string.IsNullOrEmpty(stateToken))
                        {
                            Console.WriteLine("   📦 状态变更后的新Token已生成");
                            Console.WriteLine($"   Token长度: {stateToken.Length} 字符");

                            // Save to file
                            var status = client.GetStatus();
                            if (!string.IsNullOrEmpty(status.LicenseCode))
                            {
                                var timestamp = DateTime.Now.ToString("yyyyMMddHHmmss");
                                var filename = $"token_state_{status.LicenseCode}_idx{status.StateIndex}_{timestamp}.txt";
                                await File.WriteAllTextAsync(filename, stateToken);
                                var absPath = Path.GetFullPath(filename);
                                Console.WriteLine($"   💾 已保存到: {absPath}");
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"   ⚠️  导出状态变更token失败: {ex.Message}");
                    }
                }
                else
                {
                    Console.WriteLine($"❌ 检查{checkCount}失败: 记账功能异常 - {result.ErrorMessage}");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"❌ 检查{checkCount}失败: {ex.Message}");
            }

            // Summary
            Console.WriteLine("\n📊 综合验证结果:");
            Console.WriteLine($"   总检查项: {checkCount}");
            Console.WriteLine($"   通过项目: {passCount}");
            Console.WriteLine($"   成功率: {(double)passCount / checkCount * 100:F1}%");

            if (passCount == checkCount)
            {
                Console.WriteLine("🎉 所有检查均通过！系统运行正常");
            }
            else if (passCount >= checkCount / 2)
            {
                Console.WriteLine("⚠️  大部分检查通过，系统基本正常");
            }
            else
            {
                Console.WriteLine("❌ 多项检查失败，请检查系统配置");
            }
        }

        #endregion

        #region Client Management

        private static DecentriLicenseClient? GetOrCreateClient()
        {
            if (gClient == null)
            {
                try
                {
                    gClient = new DecentriLicenseClient();
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"❌ 创建客户端失败: {ex.Message}");
                    return null;
                }
            }
            return gClient;
        }

        private static void CleanupClient()
        {
            if (gClient != null)
            {
                try
                {
                    gClient.Shutdown();
                }
                catch
                {
                    // Ignore shutdown errors
                }
                try
                {
                    gClient.Dispose();
                }
                catch
                {
                    // Ignore disposal errors
                }
                gClient = null;
                gInitialized = false;
            }
        }

        #endregion
    }
}
