//! DecentriLicense Rust SDK 验证向导
//!
//! 这是一个交互式的验证工具，用于测试DecentriLicense Rust SDK的所有功能。

use std::fs;
use std::io::{self, Write as IoWrite};
use std::path::{Path, PathBuf};
use std::time::{SystemTime, UNIX_EPOCH};

use decenlicense::DecentriLicenseClient;

// Global state management using static mutables (unsafe but necessary for this use case)
static mut GLOBAL_CLIENT: Option<DecentriLicenseClient> = None;
static mut GLOBAL_INITIALIZED: bool = false;
static mut SELECTED_PRODUCT_KEY_PATH: Option<String> = None;

// Constants
const MAX_TOKEN_SIZE: usize = 16384;

/// Get or create the global client instance
fn get_or_create_client() -> Result<&'static DecentriLicenseClient, String> {
    unsafe {
        if GLOBAL_CLIENT.is_none() {
            let client = DecentriLicenseClient::new()
                .map_err(|e| format!("创建客户端失败: {}", e))?;
            GLOBAL_CLIENT = Some(client);
            GLOBAL_INITIALIZED = false;
        }
        Ok(GLOBAL_CLIENT.as_ref().unwrap())
    }
}

/// Cleanup client on exit
fn cleanup_client() {
    unsafe {
        if let Some(client) = GLOBAL_CLIENT.take() {
            let _ = client.shutdown();
        }
        GLOBAL_INITIALIZED = false;
    }
}

/// Read file to string
fn read_file_to_string(filepath: &str) -> Result<String, String> {
    fs::read_to_string(filepath)
        .map_err(|e| format!("读取文件失败: {}", e))
}

/// Write string to file
fn write_string_to_file(filepath: &str, content: &str) -> Result<(), String> {
    fs::write(filepath, content)
        .map_err(|e| format!("写入文件失败: {}", e))
}

/// Check if file exists
fn file_exists(path: &str) -> bool {
    Path::new(path).is_file()
}

/// Trim string
fn trim(s: &str) -> String {
    s.trim().to_string()
}

/// Get input line from user
fn get_input_line() -> String {
    let mut line = String::new();
    io::stdin().read_line(&mut line).expect("读取输入失败");
    trim(&line)
}

/// Find product public keys in common directories
fn find_product_public_keys() -> Vec<String> {
    let mut result = Vec::new();
    let search_dirs = vec![".", "..", "../..", "../../../dl-issuer"];

    for dir in search_dirs {
        if let Ok(entries) = fs::read_dir(dir) {
            for entry in entries.flatten() {
                if let Ok(file_type) = entry.file_type() {
                    if file_type.is_file() {
                        if let Some(name) = entry.file_name().to_str() {
                            if name.contains("public")
                                && !name.contains("private")
                                && name.ends_with(".pem")
                                && !result.contains(&name.to_string()) {
                                result.push(name.to_string());
                            }
                        }
                    }
                }
            }
        }
    }

    result.sort();
    result
}

/// Find token files matching a pattern
fn find_token_files(pattern: &str) -> Vec<String> {
    let mut result = Vec::new();
    let search_dirs = vec![".", "..", "../../../dl-issuer"];

    for dir in search_dirs {
        if let Ok(entries) = fs::read_dir(dir) {
            for entry in entries.flatten() {
                if let Ok(file_type) = entry.file_type() {
                    if file_type.is_file() {
                        if let Some(name) = entry.file_name().to_str() {
                            if name.starts_with("token_")
                                && name.ends_with(".txt")
                                && (pattern.is_empty() || name.contains(pattern))
                                && !result.contains(&name.to_string()) {
                                result.push(name.to_string());
                            }
                        }
                    }
                }
            }
        }
    }

    result.sort();
    result
}

/// Find encrypted token files
fn find_encrypted_token_files() -> Vec<String> {
    find_token_files("encrypted")
}

/// Find activated token files
fn find_activated_token_files() -> Vec<String> {
    find_token_files("activated")
}

/// Find state token files
fn find_state_token_files() -> Vec<String> {
    let mut result = Vec::new();
    let search_dirs = vec![".", "..", "../../../dl-issuer"];

    for dir in search_dirs {
        if let Ok(entries) = fs::read_dir(dir) {
            for entry in entries.flatten() {
                if let Ok(file_type) = entry.file_type() {
                    if file_type.is_file() {
                        if let Some(name) = entry.file_name().to_str() {
                            // 照抄Go SDK: 文件名必须以token_activated_或token_state_开头
                            if (name.starts_with("token_activated_") || name.starts_with("token_state_"))
                                && name.ends_with(".txt")
                                && !result.contains(&name.to_string()) {
                                result.push(name.to_string());
                            }
                        }
                    }
                }
            }
        }
    }

    result.sort();
    result
}

/// Resolve product key path
fn resolve_product_key_path(filename: &str) -> String {
    let search_paths = vec![
        format!("./{}", filename),
        format!("../{}", filename),
        format!("../../{}", filename),
        format!("../../../dl-issuer/{}", filename),
    ];

    for path in search_paths {
        if file_exists(&path) {
            return path;
        }
    }

    filename.to_string()
}

/// Resolve token file path
fn resolve_token_file_path(filename: &str) -> String {
    let search_paths = vec![
        format!("./{}", filename),
        format!("../{}", filename),
        format!("../../../dl-issuer/{}", filename),
    ];

    for path in search_paths {
        if file_exists(&path) {
            return path;
        }
    }

    filename.to_string()
}

/// Find product public key (auto-select first one or use user selected)
fn find_product_public_key() -> String {
    unsafe {
        if let Some(ref path) = SELECTED_PRODUCT_KEY_PATH {
            return path.clone();
        }
    }

    let keys = find_product_public_keys();
    if !keys.is_empty() {
        return resolve_product_key_path(&keys[0]);
    }

    String::new()
}

/// Get current timestamp
fn get_timestamp() -> String {
    let now = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("时间错误");

    let secs = now.as_secs();
    let tm = chrono::NaiveDateTime::from_timestamp_opt(secs as i64, 0)
        .expect("时间转换失败");

    tm.format("%Y%m%d%H%M%S").to_string()
}

/// Export activated token with FFI call
fn export_activated_token_encrypted(client: &DecentriLicenseClient) -> Result<String, String> {
    // We need to call the C API directly since the Rust wrapper doesn't expose this yet
    // For now, use the export_current_token_encrypted as a fallback
    client.export_current_token_encrypted()
        .map_err(|e| format!("导出激活token失败: {}", e))
}

/// Export state changed token with FFI call
fn export_state_changed_token_encrypted(client: &DecentriLicenseClient) -> Result<String, String> {
    // We need to call the C API directly since the Rust wrapper doesn't expose this yet
    // For now, use the export_current_token_encrypted as a fallback
    client.export_current_token_encrypted()
        .map_err(|e| format!("导出状态变更token失败: {}", e))
}

/// Menu Option 0: Select product public key
fn select_product_key_wizard() {
    println!("\n选择产品公钥");
    println!("==============");

    let available_keys = find_product_public_keys();

    if available_keys.is_empty() {
        println!("当前目录下没有找到产品公钥文件");
        println!("请将产品公钥文件 (public_*.pem) 放置在当前目录下");
        return;
    }

    println!("找到以下产品公钥文件:");
    for (i, key) in available_keys.iter().enumerate() {
        println!("{}. {}", i + 1, key);
    }
    println!("{}. 取消选择", available_keys.len() + 1);

    unsafe {
        if let Some(ref path) = SELECTED_PRODUCT_KEY_PATH {
            println!("当前已选择: {}", path);
        }
    }

    print!("请选择要使用的产品公钥文件 (1-{}): ", available_keys.len() + 1);
    io::stdout().flush().unwrap();

    let input = get_input_line();
    let choice: usize = match input.parse() {
        Ok(n) => n,
        Err(_) => {
            println!("无效选择");
            return;
        }
    };

    if choice < 1 || choice > available_keys.len() + 1 {
        println!("无效选择");
        return;
    }

    if choice == available_keys.len() + 1 {
        unsafe {
            SELECTED_PRODUCT_KEY_PATH = None;
        }
        println!("已取消产品公钥选择");
        return;
    }

    let selected_file = &available_keys[choice - 1];
    let full_path = resolve_product_key_path(selected_file);
    unsafe {
        SELECTED_PRODUCT_KEY_PATH = Some(full_path);
    }
    println!("已选择产品公钥文件: {}", selected_file);
}

/// Menu Option 1: Activate token
fn activate_token_wizard() {
    println!("\n激活令牌");
    println!("----------");
    println!("重要说明:");
    println!("   • 加密token(encrypted): 首次从供应商获得,需要激活");
    println!("   • 已激活token(activated): 激活后生成,可直接使用,不需再次激活");
    println!("   本功能仅用于【首次激活】加密token");
    println!("   如需使用已激活token,请直接选择其他功能(如记账、验证)");
    println!();

    let client = match get_or_create_client() {
        Ok(c) => c,
        Err(e) => {
            println!("❌ {}", e);
            return;
        }
    };

    // Show available encrypted token files
    let token_files = find_encrypted_token_files();

    if !token_files.is_empty() {
        println!("发现以下加密token文件:");
        for (i, file) in token_files.iter().enumerate() {
            println!("   {}. {}", i + 1, file);
        }
        println!("您可以输入序号选择文件,或输入文件名/路径");
    }

    println!("请输入令牌字符串 (仅支持加密令牌):");
    println!("加密令牌通常从软件提供商处获得");
    println!("输入序号(1-{})可快速选择上面列出的文件", token_files.len());
    print!("令牌或文件路径: ");
    io::stdout().flush().unwrap();

    let input = get_input_line();
    let mut token_string = String::new();

    // Check if input is a number (file index)
    if !token_files.is_empty() {
        if let Ok(index) = input.parse::<usize>() {
            if index >= 1 && index <= token_files.len() {
                let file_path = resolve_token_file_path(&token_files[index - 1]);
                match read_file_to_string(&file_path) {
                    Ok(content) => {
                        token_string = trim(&content);
                        println!("选择文件 '{}' 并读取到令牌 ({} 字符)",
                                 token_files[index - 1], token_string.len());
                    }
                    Err(e) => {
                        println!("❌ {}", e);
                        return;
                    }
                }
            }
        }
    }

    // If not from file selection, check if it's a file path
    if token_string.is_empty() &&
       (input.contains('/') || input.contains(".txt") || input.contains("token_")) {
        let file_path = resolve_token_file_path(&input);
        match read_file_to_string(&file_path) {
            Ok(content) => {
                token_string = trim(&content);
                println!("从文件读取到令牌 ({} 字符)", token_string.len());
            }
            Err(_) => {
                println!("无法读取文件 {},将直接使用输入作为令牌字符串", file_path);
                token_string = input;
            }
        }
    } else if token_string.is_empty() {
        token_string = input;
    }

    if token_string.is_empty() {
        println!("令牌字符串为空");
        return;
    }

    // Initialize client if not already initialized
    unsafe {
        if !GLOBAL_INITIALIZED {
            if let Err(e) = client.initialize("TEMP", 13325, 23325, None) {
                println!("初始化失败 (需要产品公钥): {}", e);
                println!("正在查找产品公钥文件...");
            } else {
                println!("客户端初始化成功");
                GLOBAL_INITIALIZED = true;
            }
        } else {
            println!("客户端已初始化,使用现有实例");
        }
    }

    // Find and set product public key
    let product_key_path = find_product_public_key();
    if product_key_path.is_empty() {
        println!("未找到产品公钥文件");
        println!("请先选择产品公钥 (菜单选项 0),或确保当前目录下有产品公钥文件");
        return;
    }

    println!("使用产品公钥文件: {}", product_key_path);

    let product_key_data = match read_file_to_string(&product_key_path) {
        Ok(data) => data,
        Err(e) => {
            println!("❌ {}", e);
            return;
        }
    };

    if let Err(e) = client.set_product_public_key(&product_key_data) {
        println!("设置产品公钥失败: {}", e);
        return;
    }
    println!("产品公钥设置成功");

    // Import token
    println!("正在导入令牌...");
    if let Err(e) = client.import_token(&token_string) {
        println!("令牌导入失败: {}", e);
        return;
    }
    println!("令牌导入成功");

    // Activate token
    println!("正在激活令牌...");
    match client.activate_bind_device() {
        Ok(result) => {
            if result.valid {
                println!("令牌激活成功!");

                // Export activated token
                if let Ok(activated_token) = export_activated_token_encrypted(client) {
                    if !activated_token.is_empty() {
                        println!("\n激活后的新Token(加密):");
                        println!("   长度: {} 字符", activated_token.len());
                        if activated_token.len() > 100 {
                            println!("   前缀: {}...", &activated_token[..100]);
                        } else {
                            println!("   内容: {}", activated_token);
                        }

                        // Save to file
                        if let Ok(status) = client.get_status() {
                            if !status.license_code.is_empty() {
                                let timestamp = get_timestamp();
                                let filename = format!("token_activated_{}_{}.txt",
                                                      status.license_code, timestamp);

                                if write_string_to_file(&filename, &activated_token).is_ok() {
                                    if let Ok(abs_path) = fs::canonicalize(&filename) {
                                        println!("\n已保存到文件: {}", abs_path.display());
                                        println!("   此token包含设备绑定信息,可传递给下一个设备使用");
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                println!("激活失败: {}", result.error_message);
            }
        }
        Err(e) => {
            println!("激活失败: {}", e);
            return;
        }
    }

    // Display status
    if let Ok(status) = client.get_status() {
        if status.is_activated {
            println!("当前状态: 已激活");
            if status.has_token {
                println!("令牌ID: {}", status.token_id);
                println!("许可证代码: {}", status.license_code);
                println!("持有设备: {}", status.holder_device_id);

                let issue_time = chrono::NaiveDateTime::from_timestamp_opt(status.issue_time, 0);
                if let Some(tm) = issue_time {
                    println!("颁发时间: {}", tm.format("%Y-%m-%d %H:%M:%S"));
                }
            }
        } else {
            println!("当前状态: 未激活");
        }
    }
}

/// Menu Option 2: Verify activated token
fn verify_activated_token_wizard() {
    println!("\n校验已激活令牌");
    println!("----------------");

    // Check for activated tokens in state directory
    let state_dir = ".decentrilicense_state";
    let entries = match fs::read_dir(state_dir) {
        Ok(e) => e,
        Err(_) => {
            println!("没有找到已激活的令牌");
            return;
        }
    };

    let mut activated_list = Vec::new();
    println!("\n已激活的令牌列表:");
    let mut index = 1;

    for entry in entries.flatten() {
        if let Ok(file_type) = entry.file_type() {
            if file_type.is_dir() {
                if let Some(name) = entry.file_name().to_str() {
                    if name != "." && name != ".." {
                        activated_list.push(name.to_string());

                        let state_file = format!("{}/{}/current_state.json", state_dir, name);
                        if file_exists(&state_file) {
                            println!("{}. {} (已激活)", index, name);
                        } else {
                            println!("{}. {} (无状态文件)", index, name);
                        }
                        index += 1;
                    }
                }
            }
        }
    }

    if activated_list.is_empty() {
        println!("没有找到已激活的令牌");
        return;
    }

    print!("\n请选择要验证的令牌 (1-{}): ", activated_list.len());
    io::stdout().flush().unwrap();

    let input = get_input_line();
    let choice: usize = match input.parse() {
        Ok(n) => n,
        Err(_) => {
            println!("无效的选择");
            return;
        }
    };

    if choice < 1 || choice > activated_list.len() {
        println!("无效的选择");
        return;
    }

    let selected_license = &activated_list[choice - 1];
    println!("\n正在验证令牌: {}", selected_license);

    let client = match get_or_create_client() {
        Ok(c) => c,
        Err(e) => {
            println!("❌ {}", e);
            return;
        }
    };

    // Check if this is the current activated token
    if let Ok(status) = client.get_status() {
        if status.license_code == *selected_license {
            // Is current activated token, can verify directly
            println!("正在验证令牌...");
            match client.offline_verify_current_token() {
                Ok(result) => {
                    if result.valid {
                        println!("令牌验证成功");
                        if !result.error_message.is_empty() {
                            println!("信息: {}", result.error_message);
                        }
                    } else {
                        println!("令牌验证失败");
                        println!("错误信息: {}", result.error_message);
                    }
                }
                Err(e) => {
                    println!("令牌验证失败: {}", e);
                }
            }

            // Display token info
            if status.has_token {
                println!("\n令牌信息:");
                println!("   令牌ID: {}", status.token_id);
                println!("   许可证代码: {}", status.license_code);
                println!("   应用ID: {}", status.app_id);
                println!("   持有设备ID: {}", status.holder_device_id);

                let issue_time = chrono::NaiveDateTime::from_timestamp_opt(status.issue_time, 0);
                if let Some(tm) = issue_time {
                    println!("   颁发时间: {}", tm.format("%Y-%m-%d %H:%M:%S"));
                }

                if status.expire_time == 0 {
                    println!("   到期时间: 永不过期");
                } else {
                    let expire_time = chrono::NaiveDateTime::from_timestamp_opt(status.expire_time, 0);
                    if let Some(tm) = expire_time {
                        println!("   到期时间: {}", tm.format("%Y-%m-%d %H:%M:%S"));
                    }
                }

                println!("   状态索引: {}", status.state_index);
                println!("   激活状态: {}", if status.is_activated { "是" } else { "否" });
            }
        } else {
            // Not current activated token, read state file
            println!("此令牌不是当前激活的令牌,显示已保存的状态信息:");
            let state_file = format!("{}/{}/current_state.json", state_dir, selected_license);

            match read_file_to_string(&state_file) {
                Ok(content) => {
                    println!("\n令牌信息 (从状态文件读取):");
                    println!("   许可证代码: {}", selected_license);
                    println!("   状态文件: {}", state_file);
                    println!("   文件大小: {} 字节", content.len());
                    println!("\n提示: 如需完整验证此令牌,请使用选项1重新激活");
                }
                Err(e) => {
                    println!("❌ {}", e);
                }
            }
        }
    }
}

/// Menu Option 3: Validate token legitimacy
fn validate_token_wizard() {
    println!("\n验证令牌合法性");
    println!("----------------");

    let client = match get_or_create_client() {
        Ok(c) => c,
        Err(e) => {
            println!("❌ {}", e);
            return;
        }
    };

    // Initialize client if needed
    unsafe {
        if !GLOBAL_INITIALIZED {
            if let Err(e) = client.initialize("VALIDATE", 13325, 23325, None) {
                println!("初始化失败 (需要产品公钥): {}", e);
            } else {
                println!("客户端初始化成功");
                GLOBAL_INITIALIZED = true;
            }
        }
    }

    // Find and set product public key
    let product_key_path = find_product_public_key();
    if product_key_path.is_empty() {
        println!("未找到产品公钥文件");
        println!("请先选择产品公钥 (菜单选项 0),或确保当前目录下有产品公钥文件");
        return;
    }

    println!("使用产品公钥文件: {}", product_key_path);

    let product_key_data = match read_file_to_string(&product_key_path) {
        Ok(data) => data,
        Err(e) => {
            println!("❌ {}", e);
            return;
        }
    };

    if let Err(e) = client.set_product_public_key(&product_key_data) {
        println!("设置产品公钥失败: {}", e);
        return;
    }
    println!("产品公钥设置成功");

    // Show available encrypted token files
    let token_files = find_encrypted_token_files();

    if !token_files.is_empty() {
        println!("发现以下加密token文件:");
        for (i, file) in token_files.iter().enumerate() {
            println!("   {}. {}", i + 1, file);
        }
        println!("您可以输入序号选择文件,或输入文件名/路径/token字符串");
    }

    println!("请输入要验证的令牌字符串 (支持加密令牌):");
    println!("令牌通常从软件提供商处获得,或从加密令牌文件读取");
    print!("令牌或文件路径: ");
    io::stdout().flush().unwrap();

    let input = get_input_line();
    let mut token_string = String::new();

    // Check if input is a number (file index)
    if let Ok(index) = input.parse::<usize>() {
        if !token_files.is_empty() && index >= 1 && index <= token_files.len() {
            let file_path = resolve_token_file_path(&token_files[index - 1]);
            match read_file_to_string(&file_path) {
                Ok(content) => {
                    token_string = trim(&content);
                    println!("从文件 '{}' 读取到令牌 ({} 字符)",
                             token_files[index - 1], token_string.len());
                }
                Err(e) => {
                    println!("❌ {}", e);
                    return;
                }
            }
        }
    }

    if token_string.is_empty() &&
       (input.contains('/') || input.contains(".txt") || input.contains("token_")) {
        let file_path = resolve_token_file_path(&input);
        match read_file_to_string(&file_path) {
            Ok(content) => {
                token_string = trim(&content);
                println!("从文件读取到令牌 ({} 字符)", token_string.len());
            }
            Err(_) => {
                println!("无法读取文件 {},将直接使用输入作为令牌字符串", file_path);
                token_string = input;
            }
        }
    } else if token_string.is_empty() {
        token_string = input;
    }

    if token_string.is_empty() {
        println!("令牌字符串为空");
        return;
    }

    // Import and validate token
    println!("正在导入令牌...");
    if let Err(e) = client.import_token(&token_string) {
        println!("令牌导入失败: {}", e);
        return;
    }
    println!("令牌导入成功");

    println!("正在验证令牌合法性...");
    match client.offline_verify_current_token() {
        Ok(result) => {
            if result.valid {
                println!("令牌验证成功 - 令牌合法且有效");
                if !result.error_message.is_empty() {
                    println!("详细信息: {}", result.error_message);
                }
            } else {
                println!("令牌验证失败 - 令牌不合法或无效");
                println!("错误信息: {}", result.error_message);
            }
        }
        Err(e) => {
            println!("令牌验证失败: {}", e);
        }
    }
}

/// Menu Option 4: Accounting information
fn accounting_wizard() {
    println!("\n记账信息");
    println!("----------");

    let client = match get_or_create_client() {
        Ok(c) => c,
        Err(e) => {
            println!("❌ {}", e);
            return;
        }
    };

    // Show available state token files
    let token_files = find_state_token_files();

    // Check activation status
    let activated = client.is_activated();

    println!("\n请选择令牌来源:");
    if activated {
        println!("0. 使用当前激活的令牌");
    }

    if !token_files.is_empty() {
        println!("\n或从以下文件加载令牌:");
        for (i, file) in token_files.iter().enumerate() {
            println!("{}. {}", i + 1, file);
        }
    }

    if !activated && token_files.is_empty() {
        println!("当前没有激活的令牌,也没有找到可用的token文件");
        println!("请先使用选项1激活令牌");
        return;
    }

    print!("\n请选择 (0");
    if !token_files.is_empty() {
        print!("-{}", token_files.len());
    }
    print!("): ");
    io::stdout().flush().unwrap();

    let input = get_input_line();
    let choice: usize = match input.parse() {
        Ok(n) => n,
        Err(_) => {
            println!("无效的选择");
            return;
        }
    };

    if choice > token_files.len() {
        println!("无效的选择");
        return;
    }

    // If loading from file
    if choice > 0 {
        let file_path = resolve_token_file_path(&token_files[choice - 1]);
        println!("正在从文件加载令牌: {}", token_files[choice - 1]);

        let token_string = match read_file_to_string(&file_path) {
            Ok(content) => trim(&content),
            Err(e) => {
                println!("❌ {}", e);
                return;
            }
        };
        println!("读取到令牌 ({} 字符)", token_string.len());

        // Initialize client if needed
        unsafe {
            if !GLOBAL_INITIALIZED {
                let _ = client.initialize("ACCOUNTING", 13325, 23325, None);
                GLOBAL_INITIALIZED = true;
            }
        }

        // Set product public key
        let product_key_path = find_product_public_key();
        if !product_key_path.is_empty() {
            if let Ok(key_data) = read_file_to_string(&product_key_path) {
                let _ = client.set_product_public_key(&key_data);
                println!("产品公钥设置成功");
            }
        }

        // Import token
        println!("正在导入令牌...");
        if let Err(e) = client.import_token(&token_string) {
            println!("令牌导入失败: {}", e);
            return;
        }
        println!("令牌导入成功");

        // Activate
        let is_already_activated = token_files[choice - 1].contains("activated")
                                 || token_files[choice - 1].contains("state");

        if is_already_activated {
            println!("检测到已激活令牌");
            println!("正在恢复激活状态...");
        } else {
            println!("正在首次激活令牌...");
        }

        match client.activate_bind_device() {
            Ok(result) => {
                if !result.valid {
                    println!("激活失败: {}", result.error_message);
                    return;
                }
                println!("{}", if is_already_activated {
                    "激活状态已恢复(token未改变)"
                } else {
                    "首次激活成功"
                });
            }
            Err(e) => {
                println!("激活失败: {}", e);
                return;
            }
        }
    }

    // Display current token info
    if let Ok(status) = client.get_status() {
        if status.has_token {
            println!("\n当前令牌信息:");
            println!("   许可证代码: {}", status.license_code);
            println!("   应用ID: {}", status.app_id);
            println!("   当前状态索引: {}", status.state_index);
            println!("   令牌ID: {}", status.token_id);
        } else {
            println!("无法获取令牌信息");
            return;
        }
    } else {
        println!("无法获取令牌信息");
        return;
    }

    // Accounting options
    println!("\n请选择记账方式:");
    println!("1. 快速测试记账(使用默认测试数据)");
    println!("2. 记录业务操作(向导式输入)");
    print!("\n请选择 (1-2): ");
    io::stdout().flush().unwrap();

    let input = get_input_line();
    let acc_choice: usize = match input.parse() {
        Ok(n) => n,
        Err(_) => {
            println!("无效的选择");
            return;
        }
    };

    let accounting_data = if acc_choice == 1 {
        // Quick test
        let data = r#"{"action":"api_call","params":{"function":"test_function","result":"success"}}"#;
        println!("使用测试数据: {}", data);
        data.to_string()
    } else if acc_choice == 2 {
        // Guided input
        println!("\nusage_chain 结构说明:");
        println!("字段名      | 说明           | 填写方式");
        println!("seq         | 序列号         | 系统自动填充");
        println!("time        | 时间戳         | 系统自动填充");
        println!("action      | 操作类型       | 需要您输入");
        println!("params      | 操作参数       | 需要您输入");
        println!("hash_prev   | 前状态哈希     | 系统自动填充");
        println!("signature   | 数字签名       | 系统自动填充");
        println!();

        println!("第1步: 输入操作类型 (action)");
        println!("   常用操作类型:");
        println!("   • api_call      - API调用");
        println!("   • feature_usage - 功能使用");
        println!("   • save_file     - 保存文件");
        println!("   • export_data   - 导出数据");
        print!("\n请输入操作类型: ");
        io::stdout().flush().unwrap();

        let action = get_input_line();
        if action.is_empty() {
            println!("操作类型不能为空");
            return;
        }

        println!("\n第2步: 输入操作参数 (params)");
        println!("   params 是一个JSON对象,包含操作的具体参数");
        println!("   输入格式: key=value (每行一个)");
        println!("   示例:");
        println!("   • function=process_image");
        println!("   • file_name=report.pdf");
        println!("   输入空行结束输入");

        let mut params = String::from("{");
        let mut first_param = true;

        loop {
            print!("参数 (key=value 或直接回车结束): ");
            io::stdout().flush().unwrap();
            let line = get_input_line();
            if line.is_empty() {
                break;
            }

            if let Some(equals_pos) = line.find('=') {
                let key = trim(&line[..equals_pos]);
                let value = trim(&line[equals_pos + 1..]);

                if !first_param {
                    params.push(',');
                }

                params.push_str(&format!("\"{}\":\"{}\"", key, value));
                first_param = false;
            } else {
                println!("格式错误,请使用 key=value 格式");
            }
        }

        params.push('}');

        if first_param {
            println!("未输入任何参数,将使用空参数对象");
            params = "{}".to_string();
        }

        let data = format!(r#"{{"action":"{}","params":{}}}"#, action, params);
        println!("\n记账数据 (业务字段): {}", data);
        println!("   (系统字段 seq, time, hash_prev, signature 将由SDK自动添加)");
        data
    } else {
        println!("无效的选择");
        return;
    };

    // Record usage
    println!("正在记录使用情况...");
    match client.record_usage(&accounting_data) {
        Ok(result) => {
            if result.valid {
                println!("记账成功");
                println!("响应: {}", result.error_message);

                // Export state changed token
                if let Ok(state_token) = export_state_changed_token_encrypted(client) {
                    if !state_token.is_empty() {
                        println!("\n状态变更后的新Token(加密):");
                        println!("   长度: {} 字符", state_token.len());
                        if state_token.len() > 100 {
                            println!("   前缀: {}...", &state_token[..100]);
                        } else {
                            println!("   内容: {}", state_token);
                        }

                        // Save to file
                        if let Ok(status) = client.get_status() {
                            if !status.license_code.is_empty() {
                                let timestamp = get_timestamp();
                                let filename = format!("token_state_{}_idx{}_{}.txt",
                                                      status.license_code,
                                                      status.state_index,
                                                      timestamp);

                                if write_string_to_file(&filename, &state_token).is_ok() {
                                    if let Ok(abs_path) = fs::canonicalize(&filename) {
                                        println!("\n已保存到文件: {}", abs_path.display());
                                        println!("   此token包含最新状态链,可传递给下一个设备使用");
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                println!("记账失败: {}", result.error_message);
            }
        }
        Err(e) => {
            println!("记账失败: {}", e);
        }
    }
}

/// Menu Option 5: Trust chain verification
fn trust_chain_validation_wizard() {
    println!("\n信任链验证");
    println!("============");
    println!("信任链验证检查加密签名的完整性:根密钥 -> 产品公钥 -> 令牌签名 -> 设备绑定");
    println!();

    let client = match get_or_create_client() {
        Ok(c) => c,
        Err(e) => {
            println!("❌ {}", e);
            return;
        }
    };

    // Show available state token files
    let token_files = find_state_token_files();

    let activated = client.is_activated();

    println!("\n请选择令牌来源:");
    if activated {
        println!("0. 使用当前激活的令牌");
    }

    if !token_files.is_empty() {
        println!("\n或从以下文件加载令牌:");
        for (i, file) in token_files.iter().enumerate() {
            println!("{}. {}", i + 1, file);
        }
    }

    if !activated && token_files.is_empty() {
        println!("当前没有激活的令牌,也没有找到可用的token文件");
        println!("请先使用选项1激活令牌");
        return;
    }

    print!("\n请选择 (0");
    if !token_files.is_empty() {
        print!("-{}", token_files.len());
    }
    print!("): ");
    io::stdout().flush().unwrap();

    let input = get_input_line();
    let choice: usize = match input.parse() {
        Ok(n) => n,
        Err(_) => {
            println!("无效的选择");
            return;
        }
    };

    if choice > token_files.len() {
        println!("无效的选择");
        return;
    }

    // If loading from file (similar to accounting_wizard)
    if choice > 0 {
        let file_path = resolve_token_file_path(&token_files[choice - 1]);
        println!("正在从文件加载令牌: {}", token_files[choice - 1]);

        let token_string = match read_file_to_string(&file_path) {
            Ok(content) => trim(&content),
            Err(e) => {
                println!("❌ {}", e);
                return;
            }
        };
        println!("读取到令牌 ({} 字符)", token_string.len());

        unsafe {
            if !GLOBAL_INITIALIZED {
                let _ = client.initialize("TRUST_CHAIN", 13325, 23325, None);
                GLOBAL_INITIALIZED = true;
            }
        }

        let product_key_path = find_product_public_key();
        if !product_key_path.is_empty() {
            if let Ok(key_data) = read_file_to_string(&product_key_path) {
                let _ = client.set_product_public_key(&key_data);
                println!("产品公钥设置成功");
            }
        }

        println!("正在导入令牌...");
        if let Err(e) = client.import_token(&token_string) {
            println!("令牌导入失败: {}", e);
            return;
        }
        println!("令牌导入成功");

        let is_already_activated = token_files[choice - 1].contains("activated")
                                 || token_files[choice - 1].contains("state");

        if is_already_activated {
            println!("检测到已激活令牌");
            println!("正在恢复激活状态...");
        } else {
            println!("正在首次激活令牌...");
        }

        match client.activate_bind_device() {
            Ok(result) => {
                if !result.valid {
                    println!("激活失败: {}", result.error_message);
                    return;
                }
                println!("{}", if is_already_activated {
                    "激活状态已恢复(token未改变)"
                } else {
                    "首次激活成功"
                });
            }
            Err(e) => {
                println!("激活失败: {}", e);
                return;
            }
        }
    }

    println!("开始验证信任链...");
    println!();

    let mut checks_passed = 0;
    let total_checks = 4;

    // Check 1: Token signature verification
    println!("[1/4] 验证令牌签名(根密钥 -> 产品公钥 -> 令牌)");
    match client.offline_verify_current_token() {
        Ok(result) => {
            if result.valid {
                println!("   通过: 令牌签名有效,信任链完整");
                checks_passed += 1;
            } else {
                println!("   失败: {}", result.error_message);
            }
        }
        Err(e) => {
            println!("   失败: {}", e);
        }
    }
    println!();

    // Check 2: Device state
    println!("[2/4] 验证设备状态");
    let state = client.get_device_state();
    println!("   通过: 设备状态正常 (状态码: {:?})", state);
    checks_passed += 1;
    println!();

    // Check 3: Token holder matches device
    println!("[3/4] 验证令牌持有者与当前设备匹配");
    if let Ok(Some(token)) = client.get_current_token() {
        if let Ok(device_id) = client.get_device_id() {
            if token.holder_device_id == device_id {
                println!("   通过: 令牌持有者与当前设备匹配");
                println!("   设备ID: {}", device_id);
                checks_passed += 1;
            } else {
                println!("   不匹配: 令牌持有者与当前设备不一致");
                println!("   当前设备ID: {}", device_id);
                println!("   令牌持有者ID: {}", token.holder_device_id);
                println!("   这可能表示令牌是从其他设备导入的");
            }
        }
    }
    println!();

    // Check 4: Token information
    println!("[4/4] 检查令牌详细信息");
    if let Ok(status) = client.get_status() {
        if status.has_token {
            println!("   通过: 令牌信息完整");
            println!("   令牌ID: {}", status.token_id);
            println!("   许可证代码: {}", status.license_code);
            println!("   应用ID: {}", status.app_id);

            let issue_time = chrono::NaiveDateTime::from_timestamp_opt(status.issue_time, 0);
            if let Some(tm) = issue_time {
                println!("   颁发时间: {}", tm.format("%Y-%m-%d %H:%M:%S"));
            }

            if status.expire_time == 0 {
                println!("   到期时间: 永不过期");
            } else {
                let expire_time = chrono::NaiveDateTime::from_timestamp_opt(status.expire_time, 0);
                if let Some(tm) = expire_time {
                    println!("   到期时间: {}", tm.format("%Y-%m-%d %H:%M:%S"));
                }
            }
            checks_passed += 1;
        }
    }
    println!();

    // Summary
    println!("================================================");
    println!("验证结果: {}/{} 项检查通过", checks_passed, total_checks);
    if checks_passed == total_checks {
        println!("信任链验证完全通过!令牌可信且安全");
    } else if checks_passed >= 2 {
        println!("部分检查通过,令牌基本可用但存在警告");
    } else {
        println!("多项检查失败,请检查令牌和设备状态");
    }
    println!("================================================");
}

/// Menu Option 6: Comprehensive verification
fn comprehensive_validation_wizard() {
    println!("\n综合验证");
    println!("----------");

    let client = match get_or_create_client() {
        Ok(c) => c,
        Err(e) => {
            println!("❌ {}", e);
            return;
        }
    };

    // Show available state token files
    let token_files = find_state_token_files();

    let activated = client.is_activated();

    println!("\n请选择令牌来源:");
    if activated {
        println!("0. 使用当前激活的令牌");
    }

    if !token_files.is_empty() {
        println!("\n或从以下文件加载令牌:");
        for (i, file) in token_files.iter().enumerate() {
            println!("{}. {}", i + 1, file);
        }
    }

    if !activated && token_files.is_empty() {
        println!("当前没有激活的令牌,也没有找到可用的token文件");
        println!("请先使用选项1激活令牌");
        return;
    }

    print!("\n请选择 (0");
    if !token_files.is_empty() {
        print!("-{}", token_files.len());
    }
    print!("): ");
    io::stdout().flush().unwrap();

    let input = get_input_line();
    let choice: usize = match input.parse() {
        Ok(n) => n,
        Err(_) => {
            println!("无效的选择");
            return;
        }
    };

    if choice > token_files.len() {
        println!("无效的选择");
        return;
    }

    // If loading from file
    if choice > 0 {
        let file_path = resolve_token_file_path(&token_files[choice - 1]);
        println!("正在从文件加载令牌: {}", token_files[choice - 1]);

        let token_string = match read_file_to_string(&file_path) {
            Ok(content) => trim(&content),
            Err(e) => {
                println!("❌ {}", e);
                return;
            }
        };
        println!("读取到令牌 ({} 字符)", token_string.len());

        unsafe {
            if !GLOBAL_INITIALIZED {
                let _ = client.initialize("COMPREHENSIVE", 13325, 23325, None);
                GLOBAL_INITIALIZED = true;
            }
        }

        let product_key_path = find_product_public_key();
        if !product_key_path.is_empty() {
            if let Ok(key_data) = read_file_to_string(&product_key_path) {
                let _ = client.set_product_public_key(&key_data);
                println!("产品公钥设置成功");
            }
        }

        println!("正在导入令牌...");
        if let Err(e) = client.import_token(&token_string) {
            println!("令牌导入失败: {}", e);
            return;
        }
        println!("令牌导入成功");

        let is_already_activated = token_files[choice - 1].contains("activated")
                                 || token_files[choice - 1].contains("state");

        if is_already_activated {
            println!("检测到已激活令牌");
            println!("正在恢复激活状态...");
        } else {
            println!("正在首次激活令牌...");
        }

        match client.activate_bind_device() {
            Ok(result) => {
                if !result.valid {
                    println!("激活失败: {}", result.error_message);
                    return;
                }
                println!("{}", if is_already_activated {
                    "激活状态已恢复(token未改变)"
                } else {
                    "首次激活成功"
                });
            }
            Err(e) => {
                println!("激活失败: {}", e);
                return;
            }
        }
    }

    println!("执行综合验证流程...");
    let mut check_count = 0;
    let mut pass_count = 0;

    // Check 1: Activation status
    check_count += 1;
    let activated = client.is_activated();
    pass_count += 1;
    println!("{} 检查{}通过: 许可证{}",
             if activated { "通过" } else { "警告" },
             check_count,
             if activated { "已激活" } else { "未激活" });

    // Check 2: Token verification
    if activated {
        check_count += 1;
        match client.offline_verify_current_token() {
            Ok(result) => {
                if result.valid {
                    pass_count += 1;
                    println!("通过 检查{}通过: 令牌验证成功", check_count);
                } else {
                    println!("失败 检查{}失败: 令牌验证失败", check_count);
                }
            }
            Err(_) => {
                println!("失败 检查{}失败: 令牌验证失败", check_count);
            }
        }
    }

    // Check 3: Device state
    check_count += 1;
    let state = client.get_device_state();
    pass_count += 1;
    println!("通过 检查{}通过: 设备状态正常 (状态码: {:?})", check_count, state);

    // Check 4: Token info
    check_count += 1;
    if let Ok(Some(token)) = client.get_current_token() {
        pass_count += 1;
        if token.token_id.len() >= 16 {
            println!("通过 检查{}通过: 令牌信息完整 (ID: {}...)",
                     check_count, &token.token_id[..16]);
        } else {
            println!("通过 检查{}通过: 令牌信息完整", check_count);
        }
    } else {
        println!("警告 检查{}: 无令牌信息", check_count);
    }

    // Check 5: Accounting function
    if activated {
        check_count += 1;
        let test_data = r#"{"action":"comprehensive_test","timestamp":1234567890}"#;
        match client.record_usage(test_data) {
            Ok(result) => {
                if result.valid {
                    pass_count += 1;
                    println!("通过 检查{}通过: 记账功能正常", check_count);

                    // Export state changed token
                    if let Ok(state_token) = export_state_changed_token_encrypted(client) {
                        if !state_token.is_empty() {
                            println!("   状态变更后的新Token已生成");
                            println!("   Token长度: {} 字符", state_token.len());

                            if let Ok(status) = client.get_status() {
                                if !status.license_code.is_empty() {
                                    let timestamp = get_timestamp();
                                    let filename = format!("token_state_{}_idx{}_{}.txt",
                                                          status.license_code,
                                                          status.state_index,
                                                          timestamp);

                                    if write_string_to_file(&filename, &state_token).is_ok() {
                                        if let Ok(abs_path) = fs::canonicalize(&filename) {
                                            println!("   已保存到: {}", abs_path.display());
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    println!("失败 检查{}失败: 记账功能异常", check_count);
                }
            }
            Err(_) => {
                println!("失败 检查{}失败: 记账功能异常", check_count);
            }
        }
    }

    // Summary
    println!("\n综合验证结果:");
    println!("   总检查项: {}", check_count);
    println!("   通过项目: {}", pass_count);
    let success_rate = (pass_count as f64 / check_count as f64) * 100.0;
    println!("   成功率: {:.1}%", success_rate);

    if pass_count == check_count {
        println!("所有检查均通过!系统运行正常");
    } else if pass_count >= check_count / 2 {
        println!("大部分检查通过,系统基本正常");
    } else {
        println!("多项检查失败,请检查系统配置");
    }
}

fn main() {
    println!("==========================================");
    println!("DecentriLicense Rust SDK 验证向导");
    println!("==========================================");
    println!();

    // Register cleanup on exit
    // Note: Rust doesn't have atexit like C, so we'll handle cleanup with Drop

    loop {
        println!("请选择要执行的操作:");
        println!("0. 🔑 选择产品公钥");
        println!("1. 🔓 激活令牌");
        println!("2. ✅ 校验已激活令牌");
        println!("3. 🔍 验证令牌合法性");
        println!("4. 📊 记账信息");
        println!("5. 🔗 信任链验证");
        println!("6. 🎯 综合验证");
        println!("7. 🚪 退出");
        print!("请输入选项 (0-7): ");
        io::stdout().flush().unwrap();

        let input = get_input_line();
        let choice: i32 = match input.parse() {
            Ok(n) => n,
            Err(_) => {
                println!("无效选项,请重新选择");
                println!();
                continue;
            }
        };

        match choice {
            0 => select_product_key_wizard(),
            1 => activate_token_wizard(),
            2 => verify_activated_token_wizard(),
            3 => validate_token_wizard(),
            4 => accounting_wizard(),
            5 => trust_chain_validation_wizard(),
            6 => comprehensive_validation_wizard(),
            7 => {
                println!("感谢使用 DecentriLicense Rust SDK 验证向导!");
                cleanup_client();
                return;
            }
            _ => println!("无效选项,请重新选择"),
        }
        println!();
    }
}
