#include <napi.h>
#include <iostream>
#include <string>
#include <vector>

// Include DecentriLicense headers
#include "decenlicense_c.h"

class DecentriLicenseClient : public Napi::ObjectWrap<DecentriLicenseClient> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  DecentriLicenseClient(const Napi::CallbackInfo& info);
  ~DecentriLicenseClient();

 private:
  static Napi::FunctionReference constructor;
  
  // Methods
  Napi::Value Initialize(const Napi::CallbackInfo& info);
  Napi::Value SetProductPublicKey(const Napi::CallbackInfo& info);
  Napi::Value ImportToken(const Napi::CallbackInfo& info);
  Napi::Value OfflineVerifyCurrentToken(const Napi::CallbackInfo& info);
  Napi::Value ActivateBindDevice(const Napi::CallbackInfo& info);
  Napi::Value GetStatus(const Napi::CallbackInfo& info);
  Napi::Value RecordUsage(const Napi::CallbackInfo& info);
  Napi::Value ExportCurrentTokenEncrypted(const Napi::CallbackInfo& info);
  Napi::Value ExportActivatedTokenEncrypted(const Napi::CallbackInfo& info);
  Napi::Value ExportStateChangedTokenEncrypted(const Napi::CallbackInfo& info);
  Napi::Value Activate(const Napi::CallbackInfo& info);
  Napi::Value IsActivated(const Napi::CallbackInfo& info);
  Napi::Value GetDeviceId(const Napi::CallbackInfo& info);
  Napi::Value GetDeviceState(const Napi::CallbackInfo& info);
  Napi::Value GetCurrentToken(const Napi::CallbackInfo& info);
  Napi::Value Shutdown(const Napi::CallbackInfo& info);

  // Internal client instance
  DL_Client* client_ = nullptr;
  bool initialized_ = false;
};

// Static member initialization
Napi::FunctionReference DecentriLicenseClient::constructor;

// Constructor
DecentriLicenseClient::DecentriLicenseClient(const Napi::CallbackInfo& info) 
  : Napi::ObjectWrap<DecentriLicenseClient>(info) {
  client_ = dl_client_create();
}

DecentriLicenseClient::~DecentriLicenseClient() {
  if (client_) {
    dl_client_destroy(client_);
    client_ = nullptr;
  }
}

static void throwOnError(Napi::Env env, DL_ErrorCode code, const char* context) {
  if (code == DL_ERROR_SUCCESS) {
    return;
  }
  std::string msg = std::string(context) + ": error code=" + std::to_string(static_cast<int>(code));
  Napi::Error::New(env, msg).ThrowAsJavaScriptException();
}

// Initialize method
Napi::Value DecentriLicenseClient::Initialize(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (!client_) {
    Napi::Error::New(env, "client not created").ThrowAsJavaScriptException();
    return env.Null();
  }
  if (info.Length() < 1 || !info[0].IsObject()) {
    Napi::Error::New(env, "initialize requires config object").ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Object cfg = info[0].As<Napi::Object>();
  std::string licenseCode = cfg.Has("licenseCode") ? cfg.Get("licenseCode").ToString().Utf8Value() : "";
  uint32_t udpPort = cfg.Has("udpPort") ? cfg.Get("udpPort").ToNumber().Uint32Value() : 0;
  uint32_t tcpPort = cfg.Has("tcpPort") ? cfg.Get("tcpPort").ToNumber().Uint32Value() : 0;
  std::string registry = cfg.Has("registryServerUrl") ? cfg.Get("registryServerUrl").ToString().Utf8Value() : "";

  DL_ClientConfig c_cfg{};
  c_cfg.license_code = licenseCode.c_str();
  c_cfg.udp_port = static_cast<uint16_t>(udpPort);
  c_cfg.tcp_port = static_cast<uint16_t>(tcpPort);
  c_cfg.registry_server_url = registry.c_str();

  DL_ErrorCode rc = dl_client_initialize(client_, &c_cfg);
  if (rc != DL_ERROR_SUCCESS) {
    throwOnError(env, rc, "dl_client_initialize");
    return env.Null();
  }
  initialized_ = true;
  
  // Return a success object
  Napi::Object result = Napi::Object::New(env);
  result.Set("success", true);
  result.Set("message", "Client initialized successfully");
  
  return result;
}

Napi::Value DecentriLicenseClient::SetProductPublicKey(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!initialized_) {
    Napi::Error::New(env, "Client not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::Error::New(env, "setProductPublicKey requires string content").ThrowAsJavaScriptException();
    return env.Null();
  }
  std::string content = info[0].ToString().Utf8Value();
  DL_ErrorCode rc = dl_client_set_product_public_key(client_, content.c_str());
  throwOnError(env, rc, "dl_client_set_product_public_key");
  return Napi::Boolean::New(env, rc == DL_ERROR_SUCCESS);
}

Napi::Value DecentriLicenseClient::ImportToken(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!initialized_) {
    Napi::Error::New(env, "Client not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::Error::New(env, "importToken requires token string (encrypted or json)").ThrowAsJavaScriptException();
    return env.Null();
  }
  std::string tokenInput = info[0].ToString().Utf8Value();
  DL_ErrorCode rc = dl_client_import_token(client_, tokenInput.c_str());
  throwOnError(env, rc, "dl_client_import_token");
  return Napi::Boolean::New(env, rc == DL_ERROR_SUCCESS);
}

Napi::Value DecentriLicenseClient::OfflineVerifyCurrentToken(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!initialized_) {
    Napi::Error::New(env, "Client not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  DL_VerificationResult vr{};
  DL_ErrorCode rc = dl_client_offline_verify_current_token(client_, &vr);
  throwOnError(env, rc, "dl_client_offline_verify_current_token");
  Napi::Object out = Napi::Object::New(env);
  out.Set("valid", Napi::Boolean::New(env, vr.valid == 1));
  out.Set("error", Napi::String::New(env, vr.error_message));
  return out;
}

Napi::Value DecentriLicenseClient::ActivateBindDevice(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!initialized_) {
    Napi::Error::New(env, "Client not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  DL_VerificationResult vr{};
  DL_ErrorCode rc = dl_client_activate_bind_device(client_, &vr);
  throwOnError(env, rc, "dl_client_activate_bind_device");
  Napi::Object out = Napi::Object::New(env);
  out.Set("valid", Napi::Boolean::New(env, vr.valid == 1));
  out.Set("errorMessage", Napi::String::New(env, vr.error_message));
  return out;
}

Napi::Value DecentriLicenseClient::GetStatus(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!initialized_) {
    Napi::Error::New(env, "Client not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  DL_StatusResult st{};
  DL_ErrorCode rc = dl_client_get_status(client_, &st);
  throwOnError(env, rc, "dl_client_get_status");
  Napi::Object out = Napi::Object::New(env);
  out.Set("hasToken", Napi::Boolean::New(env, st.has_token == 1));
  out.Set("isActivated", Napi::Boolean::New(env, st.is_activated == 1));
  out.Set("issueTime", Napi::Number::New(env, static_cast<double>(st.issue_time)));
  out.Set("expireTime", Napi::Number::New(env, static_cast<double>(st.expire_time)));
  out.Set("stateIndex", Napi::Number::New(env, static_cast<double>(st.state_index)));
  out.Set("tokenId", Napi::String::New(env, st.token_id));
  out.Set("holderDeviceId", Napi::String::New(env, st.holder_device_id));
  out.Set("appId", Napi::String::New(env, st.app_id));
  out.Set("licenseCode", Napi::String::New(env, st.license_code));
  return out;
}

Napi::Value DecentriLicenseClient::RecordUsage(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!initialized_) {
    Napi::Error::New(env, "Client not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::Error::New(env, "recordUsage requires new_state_payload_json string").ThrowAsJavaScriptException();
    return env.Null();
  }
  std::string payload = info[0].ToString().Utf8Value();
  DL_VerificationResult vr{};
  DL_ErrorCode rc = dl_client_record_usage(client_, payload.c_str(), &vr);
  throwOnError(env, rc, "dl_client_record_usage");
  Napi::Object out = Napi::Object::New(env);
  out.Set("valid", Napi::Boolean::New(env, vr.valid == 1));
  out.Set("errorMessage", Napi::String::New(env, vr.error_message));
  return out;
}

Napi::Value DecentriLicenseClient::ExportCurrentTokenEncrypted(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!initialized_) {
    Napi::Error::New(env, "Client not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  std::vector<char> buf(8192);
  DL_ErrorCode rc = dl_client_export_current_token_encrypted(client_, buf.data(), buf.size());
  throwOnError(env, rc, "dl_client_export_current_token_encrypted");
  return Napi::String::New(env, std::string(buf.data()));
}

Napi::Value DecentriLicenseClient::ExportActivatedTokenEncrypted(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!initialized_) {
    Napi::Error::New(env, "Client not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  std::vector<char> buf(8192);
  DL_ErrorCode rc = dl_client_export_activated_token_encrypted(client_, buf.data(), buf.size());
  throwOnError(env, rc, "dl_client_export_activated_token_encrypted");
  return Napi::String::New(env, std::string(buf.data()));
}

Napi::Value DecentriLicenseClient::ExportStateChangedTokenEncrypted(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!initialized_) {
    Napi::Error::New(env, "Client not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  std::vector<char> buf(8192);
  DL_ErrorCode rc = dl_client_export_state_changed_token_encrypted(client_, buf.data(), buf.size());
  throwOnError(env, rc, "dl_client_export_state_changed_token_encrypted");
  return Napi::String::New(env, std::string(buf.data()));
}

// Activate method
Napi::Value DecentriLicenseClient::Activate(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!initialized_) {
    Napi::Error::New(env, "Client not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }

  DL_ActivationResult ar{};
  DL_ErrorCode rc = dl_client_activate(client_, &ar);
  throwOnError(env, rc, "dl_client_activate");
  Napi::Object result = Napi::Object::New(env);
  result.Set("success", Napi::Boolean::New(env, ar.success == 1));
  result.Set("message", Napi::String::New(env, ar.message));
  return result;
}

// IsActivated method
Napi::Value DecentriLicenseClient::IsActivated(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!initialized_) {
    return Napi::Boolean::New(env, false);
  }
  int v = dl_client_is_activated(client_);
  return Napi::Boolean::New(env, v == 1);
}

// GetDeviceId method
Napi::Value DecentriLicenseClient::GetDeviceId(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!initialized_) {
    return Napi::String::New(env, "");
  }
  std::vector<char> buf(128);
  DL_ErrorCode rc = dl_client_get_device_id(client_, buf.data(), buf.size());
  throwOnError(env, rc, "dl_client_get_device_id");
  return Napi::String::New(env, std::string(buf.data()));
}

// GetDeviceState method
Napi::Value DecentriLicenseClient::GetDeviceState(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!initialized_) {
    return Napi::String::New(env, "idle");
  }
  DL_DeviceState st = dl_client_get_device_state(client_);
  switch (st) {
    case DL_DEVICE_STATE_DISCOVERING: return Napi::String::New(env, "discovering");
    case DL_DEVICE_STATE_ELECTING: return Napi::String::New(env, "electing");
    case DL_DEVICE_STATE_COORDINATOR: return Napi::String::New(env, "coordinator");
    case DL_DEVICE_STATE_FOLLOWER: return Napi::String::New(env, "follower");
    case DL_DEVICE_STATE_IDLE:
    default: return Napi::String::New(env, "idle");
  }
}

// GetCurrentToken method
Napi::Value DecentriLicenseClient::GetCurrentToken(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (!initialized_) {
    return env.Null();
  }
  DL_Token t{};
  DL_ErrorCode rc = dl_client_get_current_token(client_, &t);
  throwOnError(env, rc, "dl_client_get_current_token");
  if (std::string(t.token_id).empty()) {
    return env.Null();
  }
  Napi::Object token = Napi::Object::New(env);
  token.Set("tokenId", Napi::String::New(env, t.token_id));
  token.Set("holderDeviceId", Napi::String::New(env, t.holder_device_id));
  token.Set("licenseCode", Napi::String::New(env, t.license_code));
  token.Set("appId", Napi::String::New(env, t.app_id));
  return token;
}

// Shutdown method
Napi::Value DecentriLicenseClient::Shutdown(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (!client_) {
    return Napi::Boolean::New(env, true);
  }
  (void)dl_client_shutdown(client_);
  initialized_ = false;
  return Napi::Boolean::New(env, true);
}

// Init method
Napi::Object DecentriLicenseClient::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "DecentriLicenseClient", {
    InstanceMethod("initialize", &DecentriLicenseClient::Initialize),
    InstanceMethod("setProductPublicKey", &DecentriLicenseClient::SetProductPublicKey),
    InstanceMethod("importToken", &DecentriLicenseClient::ImportToken),
    InstanceMethod("offlineVerify", &DecentriLicenseClient::OfflineVerifyCurrentToken),
    InstanceMethod("activateBindDevice", &DecentriLicenseClient::ActivateBindDevice),
    InstanceMethod("getStatus", &DecentriLicenseClient::GetStatus),
    InstanceMethod("recordUsage", &DecentriLicenseClient::RecordUsage),
    InstanceMethod("exportEncryptedToken", &DecentriLicenseClient::ExportCurrentTokenEncrypted),
    InstanceMethod("exportActivatedTokenEncrypted", &DecentriLicenseClient::ExportActivatedTokenEncrypted),
    InstanceMethod("exportStateChangedTokenEncrypted", &DecentriLicenseClient::ExportStateChangedTokenEncrypted),
    InstanceMethod("activate", &DecentriLicenseClient::Activate),
    InstanceMethod("isActivated", &DecentriLicenseClient::IsActivated),
    InstanceMethod("getDeviceId", &DecentriLicenseClient::GetDeviceId),
    InstanceMethod("getDeviceState", &DecentriLicenseClient::GetDeviceState),
    InstanceMethod("getCurrentToken", &DecentriLicenseClient::GetCurrentToken),
    InstanceMethod("shutdown", &DecentriLicenseClient::Shutdown)
  });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("DecentriLicenseClient", func);
  return exports;
}

// Module initialization
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  return DecentriLicenseClient::Init(env, exports);
}

NODE_API_MODULE(decenlicense_node, Init)