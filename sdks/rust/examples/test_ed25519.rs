use std::fs;
use decenlicense::*;

fn read_file(path: &str) -> String {
    fs::read_to_string(path).unwrap_or_default()
}

fn new_client() -> *mut DL_Client {
    unsafe {
        let client = dl_client_create();
        if client.is_null() {
            panic!("Failed to create client");
        }
        let mut config: DL_ClientConfig = std::mem::zeroed();
        config.udp_port = 13325;
        config.tcp_port = 23325;
        let rc = dl_client_initialize(client, &config);
        if rc != DL_ErrorCode_DL_ERROR_SUCCESS {
            panic!("Init failed: {:?}", rc);
        }
        client
    }
}

fn test_windsurf_free() {
    let dir = "/Volumes/workspace/project/dl-issuer/dl-issuer";
    let token_json = read_file(&format!("{}/token_windsurf-free_AUTO-GENERATED-LICENSE-2026-018-0B3GNW_20260427090408.json", dir));
    let product_key = read_file(&format!("{}/public_windsurf-free_20260427090402.pem", dir));
    assert!(!token_json.is_empty() && !product_key.is_empty(), "windsurf-free files not found");
    println!("windsurf-free token loaded");

    unsafe {
        let c1 = new_client();
        dl_client_set_product_public_key(c1, product_key.as_ptr() as *const i8);
        dl_client_import_token(c1, token_json.as_ptr() as *const i8);
        let mut vr1 = std::mem::zeroed::<DL_VerificationResult>();
        dl_client_offline_verify_current_token(c1, &mut vr1);
        assert!(vr1.valid != 0, "windsurf-free verify failed: {}", std::ffi::CStr::from_ptr(vr1.error_message.as_ptr()).to_str().unwrap_or(""));
        println!("✅ Rust SDK: windsurf-free offline_verify passed");
        dl_client_shutdown(c1);
        dl_client_destroy(c1);

        let c2 = new_client();
        dl_client_set_product_public_key(c2, product_key.as_ptr() as *const i8);
        dl_client_import_token(c2, token_json.as_ptr() as *const i8);
        let mut vr2 = std::mem::zeroed::<DL_VerificationResult>();
        dl_client_activate_bind_device(c2, &mut vr2);
        assert!(vr2.valid != 0, "windsurf-free activate failed: {}", std::ffi::CStr::from_ptr(vr2.error_message.as_ptr()).to_str().unwrap_or(""));
        println!("✅ Rust SDK: windsurf-free activate_bind_device passed");
        dl_client_shutdown(c2);
        dl_client_destroy(c2);
    }
}

fn test_cursor_free() {
    let dir = "/Volumes/workspace/project/dl-issuer/dl-issuer";
    let token_json = read_file(&format!("{}/token_cursor-free_AUTO-GENERATED-LICENSE-2026-019-I0SKNQ_20260427090411.json", dir));
    let product_key = read_file(&format!("{}/public_cursor-free_20260427090405.pem", dir));
    assert!(!token_json.is_empty() && !product_key.is_empty(), "cursor-free files not found");
    println!("cursor-free token loaded");

    unsafe {
        let c1 = new_client();
        dl_client_set_product_public_key(c1, product_key.as_ptr() as *const i8);
        dl_client_import_token(c1, token_json.as_ptr() as *const i8);
        let mut vr1 = std::mem::zeroed::<DL_VerificationResult>();
        dl_client_offline_verify_current_token(c1, &mut vr1);
        assert!(vr1.valid != 0, "cursor-free verify failed: {}", std::ffi::CStr::from_ptr(vr1.error_message.as_ptr()).to_str().unwrap_or(""));
        println!("✅ Rust SDK: cursor-free offline_verify passed");
        dl_client_shutdown(c1);
        dl_client_destroy(c1);

        let c2 = new_client();
        dl_client_set_product_public_key(c2, product_key.as_ptr() as *const i8);
        dl_client_import_token(c2, token_json.as_ptr() as *const i8);
        let mut vr2 = std::mem::zeroed::<DL_VerificationResult>();
        dl_client_activate_bind_device(c2, &mut vr2);
        assert!(vr2.valid != 0, "cursor-free activate failed: {}", std::ffi::CStr::from_ptr(vr2.error_message.as_ptr()).to_str().unwrap_or(""));
        println!("✅ Rust SDK: cursor-free activate_bind_device passed");
        dl_client_shutdown(c2);
        dl_client_destroy(c2);
    }
}

fn main() {
    test_windsurf_free();
    test_cursor_free();
    println!("\n🎉 All Rust SDK tests passed!");
}
