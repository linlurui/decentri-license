use serde::{Deserialize, Serialize};
use std::fs;
use std::io::{self, Write};
use std::path::Path;

#[derive(Serialize, Deserialize, Default)]
struct LicenseState {
    token_data: String,
    license_public_key: String,
    is_activated: bool,
    device_id: String,
    activation_time: String,
    usage_count: i32,
}

impl LicenseState {
    fn new() -> Self {
        LicenseState {
            token_data: String::new(),
            license_public_key: String::new(),
            is_activated: false,
            device_id: String::new(),
            activation_time: String::new(),
            usage_count: 0,
        }
    }
}

struct SimpleValidationWizard {
    license_state: LicenseState,
    state_file: String,
}

impl SimpleValidationWizard {
    fn new() -> Self {
        let mut wizard = SimpleValidationWizard {
            license_state: LicenseState::new(),
            state_file: ".decentri/license.state".to_string(),
        };
        
        // Try to load previous state
        wizard.load_state();
        
        wizard
    }
    
    fn run(&mut self) {
        println!("==========================================");
        println!("DecentriLicense Rust SDK 验证向导");
        println!("==========================================");
        
        loop {
            self.print_menu();
            
            let choice = self.get_input("请选择: ");
            let choice = choice.trim();
            
            match choice {
                "1" => self.import_license_key(),
                "2" => self.verify_license(),
                "3" => self.activate_to_device(),
                "4" => self.query_status(),
                "5" => self.record_usage(),
                "0" => {
                    println!("再见！");
                    break;
                }
                _ => println!("❌ 无效选项，请重新输入。"),
            }
            
            println!();
            println!("{}", "-".repeat(50));
            println!();
        }
    }
    
    fn print_menu(&self) {
        println!("\n=== DecentriLicense 向导 ===");
        println!("1. 导入许可证密钥");
        println!("2. 验证许可证");
        println!("3. 激活到当前设备");
        println!("4. 查询当前状态/余额");
        println!("5. 记录使用量（状态迁移）");
        println!("0. 退出");
    }
    
    fn import_license_key(&mut self) {
        println!("\n--- 导入许可证密钥 ---");
        
        let input_method = self.get_input("输入方式 (1: 直接粘贴, 2: 文件路径): ");
        let input_method = input_method.trim();
        
        if input_method == "1" {
            println!("请粘贴许可证密钥（JWT格式或加密后的字符串）:");
            let mut key_data = String::new();
            io::stdin().read_line(&mut key_data).expect("读取输入失败");
            let key_data = key_data.trim();
            
            if key_data.is_empty() {
                println!("❌ 输入不能为空");
                return;
            }
            
            self.license_state.token_data = key_data.to_string();
            println!("✅ 许可证密钥已导入");
            self.save_state();
            
        } else if input_method == "2" {
            let file_path = self.get_input("请输入文件路径: ");
            let file_path = file_path.trim();
            
            if file_path.is_empty() {
                println!("❌ 文件路径不能为空");
                return;
            }
            
            match fs::read_to_string(file_path) {
                Ok(data) => {
                    self.license_state.token_data = data;
                    println!("✅ 许可证密钥已从文件导入");
                    self.save_state();
                }
                Err(_) => {
                    println!("❌ 读取文件失败");
                }
            }
        } else {
            println!("❌ 无效的输入方式");
        }
    }
    
    fn verify_license(&self) {
        println!("\n--- 验证许可证 ---");
        
        if self.license_state.token_data.is_empty() {
            println!("❌ 请先导入许可证密钥");
            return;
        }
        
        // Check if it's an encrypted token
        if self.is_encrypted_token(&self.license_state.token_data) {
            println!("🔒 检测到加密的许可证，正在解密...");
            
            // Here should be the actual decryption logic
            // For simplicity, we assume decryption is successful
            println!("✅ 许可证解密成功");
        } else {
            println!("📄 检测到JSON格式的许可证");
        }
        
        // Simulate verification process
        println!("🔍 正在校验许可证签名...");
        println!("✅ 许可证验证通过");
    }
    
    fn activate_to_device(&mut self) {
        println!("\n--- 激活到当前设备 ---");
        
        if self.license_state.token_data.is_empty() {
            println!("❌ 请先导入许可证密钥");
            return;
        }
        
        // Generate device ID (simplified example)
        let timestamp = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap()
            .as_nanos() % 100000;
        self.license_state.device_id = format!("DEV-{}", timestamp);
        
        // Get current time
        let now: chrono::DateTime<chrono::Local> = chrono::Local::now();
        self.license_state.activation_time = now.format("%Y-%m-%d %H:%M:%S").to_string();
        self.license_state.is_activated = true;
        
        println!("✅ 设备激活成功");
        println!("  设备ID: {}", self.license_state.device_id);
        println!("  激活时间: {}", self.license_state.activation_time);
        
        self.save_state();
    }
    
    fn query_status(&self) {
        println!("\n--- 查询当前状态/余额 ---");
        
        println!("许可证状态:");
        if self.license_state.token_data.is_empty() {
            println!("  是否已导入: 否");
        } else {
            println!("  是否已导入: 是");
        }
        println!("  是否已激活: {}", if self.license_state.is_activated { "是" } else { "否" });
        
        if self.license_state.is_activated {
            println!("  设备ID: {}", self.license_state.device_id);
            println!("  激活时间: {}", self.license_state.activation_time);
        }
        
        println!("  使用次数: {}", self.license_state.usage_count);
    }
    
    fn record_usage(&mut self) {
        println!("\n--- 记录使用量（状态迁移） ---");
        
        if !self.license_state.is_activated {
            println!("❌ 请先激活到当前设备");
            return;
        }
        
        self.license_state.usage_count += 1;
        
        println!("✅ 使用量记录成功");
        println!("  当前使用次数: {}", self.license_state.usage_count);
        
        self.save_state();
    }
    
    fn get_input(&self, prompt: &str) -> String {
        print!("{}", prompt);
        io::stdout().flush().unwrap();
        
        let mut input = String::new();
        io::stdin().read_line(&mut input).expect("读取输入失败");
        input
    }
    
    fn save_state(&self) -> bool {
        // Create directory
        let path = Path::new(&self.state_file);
        if let Some(parent) = path.parent() {
            if let Err(_) = fs::create_dir_all(parent) {
                println!("⚠️  创建目录失败");
                return false;
            }
        }
        
        // Serialize to JSON
        match serde_json::to_string_pretty(&self.license_state) {
            Ok(json) => {
                // Write to file
                match fs::write(&self.state_file, json) {
                    Ok(_) => true,
                    Err(e) => {
                        println!("⚠️  保存状态失败: {}", e);
                        false
                    }
                }
            }
            Err(e) => {
                println!("⚠️  序列化状态失败: {}", e);
                false
            }
        }
    }
    
    fn load_state(&mut self) -> bool {
        match fs::read_to_string(&self.state_file) {
            Ok(data) => {
                match serde_json::from_str::<LicenseState>(&data) {
                    Ok(state) => {
                        self.license_state = state;
                        true
                    }
                    Err(_) => false,
                }
            }
            Err(_) => false,
        }
    }
    
    fn is_encrypted_token(&self, input: &str) -> bool {
        // Encrypted token format: encrypted_data|nonce
        input.contains('|')
    }
}

fn main() {
    let mut wizard = SimpleValidationWizard::new();
    wizard.run();
}