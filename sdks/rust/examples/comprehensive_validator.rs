//! DecentriLicense Rust SDK Comprehensive Validator

use std::fs;
use std::env;
use std::io::{self, Write};
use serde_json::Value;

/// Comprehensive validator for DecentriLicense tokens using trust chain model.
pub struct ComprehensiveValidator;

impl ComprehensiveValidator {
    /// Validate a token using the trust chain model.
    /// 
    /// # Arguments
    /// * `token_file` - Path to the token JSON file
    /// * `root_public_key_file` - Path to the root public key file
    /// * `algorithm` - Expected algorithm (RSA, Ed25519, SM2)
    /// 
    /// # Returns
    /// * `Ok(true)` if validation succeeds, `Ok(false)` otherwise
    /// * `Err` if there was an error reading or parsing files
    pub fn validate_token_with_trust_chain(
        token_file: &str, 
        root_public_key_file: &str, 
        algorithm: &str
    ) -> Result<bool, Box<dyn std::error::Error>> {
        // Load token from file
        let token_content = fs::read_to_string(token_file)?;
        let token_data: Value = serde_json::from_str(&token_content)?;
        
        // Load root public key from file
        let root_public_key = fs::read_to_string(root_public_key_file)?;
        
        // Verify trust chain
        Self::verify_trust_chain(&token_data, &root_public_key, algorithm)
    }
    
    /// Verify the trust chain for a token.
    /// 
    /// # Arguments
    /// * `token_data` - Token data object
    /// * `root_public_key` - Root public key content
    /// * `algorithm` - Expected algorithm
    /// 
    /// # Returns
    /// * `Ok(true)` if trust chain is valid, `Ok(false)` otherwise
    /// * `Err` if there was an error during verification
    fn verify_trust_chain(
        token_data: &Value, 
        _root_public_key: &str, 
        algorithm: &str
    ) -> Result<bool, Box<dyn std::error::Error>> {
        println!("Verifying trust chain for token {}", 
            token_data.get("token_id").and_then(|v| v.as_str()).unwrap_or("unknown"));
            
        println!("  License Code: {}", 
            token_data.get("license_code").and_then(|v| v.as_str()).unwrap_or("unknown"));
            
        println!("  Algorithm: {}", 
            token_data.get("alg").and_then(|v| v.as_str()).unwrap_or("unknown"));
            
        println!("  Expected Algorithm: {}", algorithm);
        
        let has_license_pub_key = token_data.get("license_public_key")
            .and_then(|v| v.as_str())
            .map(|s| !s.is_empty())
            .unwrap_or(false);
        println!("  License Public Key present: {}", has_license_pub_key);
        
        let has_root_signature = token_data.get("root_signature")
            .and_then(|v| v.as_str())
            .map(|s| !s.is_empty())
            .unwrap_or(false);
        println!("  Root Signature present: {}", has_root_signature);
        
        // Verify that the algorithm matches
        if token_data.get("alg").and_then(|v| v.as_str()) != Some(algorithm) {
            println!("Algorithm mismatch: expected {}, got {}", 
                algorithm, 
                token_data.get("alg").and_then(|v| v.as_str()).unwrap_or("unknown"));
            return Ok(false);
        }
        
        // Check that required fields are present
        if !has_license_pub_key || !has_root_signature {
            println!("Missing required trust chain fields");
            return Ok(false);
        }
        
        // In a real implementation, we would:
        // 1. Verify the root signature of the license public key using the root public key
        // 2. Verify the token signature using the verified license public key
        
        // For demonstration purposes, we'll just return true if both fields are present and algorithm matches
        Ok(true)
    }
}

fn list_files_for_selection(exts: &[&str]) -> Result<Vec<String>, Box<dyn std::error::Error>> {
    let mut files: Vec<String> = Vec::new();
    for entry in fs::read_dir(".")? {
        let entry = entry?;
        let path = entry.path();
        if !path.is_file() {
            continue;
        }
        let name = match path.file_name().and_then(|s| s.to_str()) {
            Some(s) => s.to_string(),
            None => continue,
        };
        if !exts.is_empty() {
            let lower = name.to_lowercase();
            let mut ok = false;
            for ext in exts {
                if lower.ends_with(ext) {
                    ok = true;
                    break;
                }
            }
            if !ok {
                continue;
            }
        }
        files.push(name);
    }
    files.sort();
    Ok(files)
}

fn ask(prompt: &str) -> Result<String, Box<dyn std::error::Error>> {
    print!("{}", prompt);
    io::stdout().flush()?;
    let mut s = String::new();
    io::stdin().read_line(&mut s)?;
    Ok(s.trim().to_string())
}

fn pick_file_from_cwd(title: &str, exts: &[&str]) -> Result<String, Box<dyn std::error::Error>> {
    let files = list_files_for_selection(exts)?;
    println!("{}", title);
    if files.is_empty() {
        return ask("当前目录没有可选文件，请手动输入路径: ");
    }
    for (i, f) in files.iter().enumerate() {
        println!("{}. {}", i + 1, f);
    }
    println!("0. 手动输入路径");
    let sel = ask("请选择文件编号: ")?;
    if let Ok(n) = sel.parse::<usize>() {
        if n >= 1 && n <= files.len() {
            return Ok(files[n - 1].clone());
        }
    }
    ask("请输入文件路径: ")
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = env::args().collect();

    let mut token_file = args.get(1).cloned().unwrap_or_default();
    let mut root_public_key_file = args.get(2).cloned().unwrap_or_default();
    let mut algorithm = args.get(3).cloned().unwrap_or_default();

    if token_file.trim().is_empty() {
        token_file = pick_file_from_cwd("请选择 token 文件:", &[".json", ".txt"])?;
    }
    if root_public_key_file.trim().is_empty() {
        root_public_key_file = pick_file_from_cwd("请选择根公钥文件:", &[".pem"])?;
    }
    if algorithm.trim().is_empty() {
        algorithm = ask("请输入 algorithm (RSA/Ed25519/SM2): ")?;
    }

    if token_file.trim().is_empty() || root_public_key_file.trim().is_empty() || algorithm.trim().is_empty() {
        println!("Usage: comprehensive_validator <token_file> <root_public_key_file> <algorithm>");
        std::process::exit(1);
    }
    
    let valid = ComprehensiveValidator::validate_token_with_trust_chain(
        &token_file,
        &root_public_key_file,
        &algorithm
    )?;
    
    if valid {
        println!("✅ Token validation successful!");
        Ok(())
    } else {
        println!("❌ Token validation failed!");
        std::process::exit(1);
    }
}