#include <jni.h>
#include "decenlicense_c.h"
#include <cstring>
#include <cstdlib>

#ifdef __cplusplus
extern "C" {
#endif

// Helper function to convert C string to Java string
jstring toJavaString(JNIEnv *env, const char* str) {
    if (str == nullptr) {
        return env->NewStringUTF("");
    }
    return env->NewStringUTF(str);
}

// Helper function to convert Java string to C string
char* toCString(JNIEnv *env, jstring jstr) {
    if (jstr == nullptr) {
        return nullptr;
    }
    
    const char *cstr = env->GetStringUTFChars(jstr, nullptr);
    char *result = strdup(cstr);
    env->ReleaseStringUTFChars(jstr, cstr);
    return result;
}

static jobject createVerificationResult(JNIEnv* env, const DL_VerificationResult& vr) {
    jclass cls = env->FindClass("com/decentrilicense/VerificationResult");
    if (cls == nullptr) {
        return nullptr;
    }
    jmethodID ctor = env->GetMethodID(cls, "<init>", "()V");
    if (ctor == nullptr) {
        return nullptr;
    }
    jobject obj = env->NewObject(cls, ctor);
    if (obj == nullptr) {
        return nullptr;
    }

    jfieldID validField = env->GetFieldID(cls, "valid", "Z");
    jfieldID errField = env->GetFieldID(cls, "errorMessage", "Ljava/lang/String;");
    if (validField == nullptr || errField == nullptr) {
        return nullptr;
    }
    env->SetBooleanField(obj, validField, vr.valid ? JNI_TRUE : JNI_FALSE);
    env->SetObjectField(obj, errField, toJavaString(env, vr.error_message));
    return obj;
}

static jobject createStatusResult(JNIEnv* env, const DL_StatusResult& st) {
    jclass cls = env->FindClass("com/decentrilicense/StatusResult");
    if (cls == nullptr) {
        return nullptr;
    }
    jmethodID ctor = env->GetMethodID(cls, "<init>", "()V");
    if (ctor == nullptr) {
        return nullptr;
    }
    jobject obj = env->NewObject(cls, ctor);
    if (obj == nullptr) {
        return nullptr;
    }

    jfieldID hasTokenField = env->GetFieldID(cls, "hasToken", "Z");
    jfieldID activatedField = env->GetFieldID(cls, "activated", "Z");
    jfieldID issueTimeField = env->GetFieldID(cls, "issueTime", "J");
    jfieldID expireTimeField = env->GetFieldID(cls, "expireTime", "J");
    jfieldID stateIndexField = env->GetFieldID(cls, "stateIndex", "J");
    jfieldID tokenIdField = env->GetFieldID(cls, "tokenId", "Ljava/lang/String;");
    jfieldID holderDeviceIdField = env->GetFieldID(cls, "holderDeviceId", "Ljava/lang/String;");
    jfieldID appIdField = env->GetFieldID(cls, "appId", "Ljava/lang/String;");
    jfieldID licenseCodeField = env->GetFieldID(cls, "licenseCode", "Ljava/lang/String;");
    if (hasTokenField == nullptr || activatedField == nullptr || issueTimeField == nullptr || expireTimeField == nullptr ||
        stateIndexField == nullptr || tokenIdField == nullptr || holderDeviceIdField == nullptr || appIdField == nullptr ||
        licenseCodeField == nullptr) {
        return nullptr;
    }

    env->SetBooleanField(obj, hasTokenField, st.has_token ? JNI_TRUE : JNI_FALSE);
    env->SetBooleanField(obj, activatedField, st.is_activated ? JNI_TRUE : JNI_FALSE);
    env->SetLongField(obj, issueTimeField, static_cast<jlong>(st.issue_time));
    env->SetLongField(obj, expireTimeField, static_cast<jlong>(st.expire_time));
    env->SetLongField(obj, stateIndexField, static_cast<jlong>(st.state_index));
    env->SetObjectField(obj, tokenIdField, toJavaString(env, st.token_id));
    env->SetObjectField(obj, holderDeviceIdField, toJavaString(env, st.holder_device_id));
    env->SetObjectField(obj, appIdField, toJavaString(env, st.app_id));
    env->SetObjectField(obj, licenseCodeField, toJavaString(env, st.license_code));
    return obj;
}

// Create client
JNIEXPORT jlong JNICALL Java_com_decentrilicense_DecentriLicenseClient_createClient
  (JNIEnv *env, jobject obj) {
    DL_Client* client = dl_client_create();
    return reinterpret_cast<jlong>(client);
}

// Initialize client
JNIEXPORT jint JNICALL Java_com_decentrilicense_DecentriLicenseClient_initializeClient
  (JNIEnv *env, jobject obj, jlong handle, jstring licenseCode, jint udpPort, jint tcpPort, jstring registryServerUrl) {
    DL_Client* client = reinterpret_cast<DL_Client*>(handle);
    if (client == nullptr) {
        return -1;
    }
    
    DL_ClientConfig config = {};
    config.license_code = toCString(env, licenseCode);
    config.udp_port = static_cast<uint16_t>(udpPort);
    config.tcp_port = static_cast<uint16_t>(tcpPort);
    config.registry_server_url = toCString(env, registryServerUrl);
    
    DL_ErrorCode result = dl_client_initialize(client, &config);
    
    // Free allocated strings
    free(const_cast<char*>(config.license_code));
    free(const_cast<char*>(config.registry_server_url));
    
    return static_cast<jint>(result);
}

// Activate client
JNIEXPORT jobject JNICALL Java_com_decentrilicense_DecentriLicenseClient_activateClient
  (JNIEnv *env, jobject obj, jlong handle) {
    DL_Client* client = reinterpret_cast<DL_Client*>(handle);
    if (client == nullptr) {
        return nullptr;
    }
    
    DL_ActivationResult result;
    DL_ErrorCode error = dl_client_activate(client, &result);
    
    if (error != DL_ERROR_SUCCESS) {
        return nullptr;
    }
    
    // Create ActivationResult object
    jclass activationResultClass = env->FindClass("com/decentrilicense/ActivationResult");
    jmethodID constructor = env->GetMethodID(activationResultClass, "<init>", "()V");
    jobject activationResult = env->NewObject(activationResultClass, constructor);
    
    // Set fields
    jfieldID successField = env->GetFieldID(activationResultClass, "success", "Z");
    env->SetBooleanField(activationResult, successField, result.success);
    
    jfieldID messageField = env->GetFieldID(activationResultClass, "message", "Ljava/lang/String;");
    jstring message = toJavaString(env, result.message);
    env->SetObjectField(activationResult, messageField, message);
    
    // Handle token if present
    if (result.token != nullptr && result.success) {
        jclass tokenClass = env->FindClass("com/decentrilicense/Token");
        jmethodID tokenConstructor = env->GetMethodID(tokenClass, "<init>", "()V");
        jobject token = env->NewObject(tokenClass, tokenConstructor);
        
        // Set token fields
        jfieldID tokenIdField = env->GetFieldID(tokenClass, "tokenId", "Ljava/lang/String;");
        jstring tokenId = toJavaString(env, result.token->token_id);
        env->SetObjectField(token, tokenIdField, tokenId);
        
        jfieldID holderDeviceIdField = env->GetFieldID(tokenClass, "holderDeviceId", "Ljava/lang/String;");
        jstring holderDeviceId = toJavaString(env, result.token->holder_device_id);
        env->SetObjectField(token, holderDeviceIdField, holderDeviceId);
        
        jfieldID issueTimeField = env->GetFieldID(tokenClass, "issueTime", "J");
        env->SetLongField(token, issueTimeField, result.token->issue_time);
        
        jfieldID expireTimeField = env->GetFieldID(tokenClass, "expireTime", "J");
        env->SetLongField(token, expireTimeField, result.token->expire_time);
        
        jfieldID signatureField = env->GetFieldID(tokenClass, "signature", "Ljava/lang/String;");
        jstring signature = toJavaString(env, result.token->signature);
        env->SetObjectField(token, signatureField, signature);
        
        jfieldID tokenField = env->GetFieldID(activationResultClass, "token", "Lcom/decentrilicense/Token;");
        env->SetObjectField(activationResult, tokenField, token);
    }
    
    return activationResult;
}

// Get current token
JNIEXPORT jobject JNICALL Java_com_decentrilicense_DecentriLicenseClient_getCurrentTokenClient
  (JNIEnv *env, jobject obj, jlong handle) {
    DL_Client* client = reinterpret_cast<DL_Client*>(handle);
    if (client == nullptr) {
        return nullptr;
    }
    
    DL_Token token;
    DL_ErrorCode error = dl_client_get_current_token(client, &token);
    
    if (error != DL_ERROR_SUCCESS) {
        return nullptr;
    }
    
    // Create Token object
    jclass tokenClass = env->FindClass("com/decentrilicense/Token");
    jmethodID constructor = env->GetMethodID(tokenClass, "<init>", "()V");
    jobject tokenObj = env->NewObject(tokenClass, constructor);
    
    // Set fields
    jfieldID tokenIdField = env->GetFieldID(tokenClass, "tokenId", "Ljava/lang/String;");
    jstring tokenId = toJavaString(env, token.token_id);
    env->SetObjectField(tokenObj, tokenIdField, tokenId);
    
    jfieldID holderDeviceIdField = env->GetFieldID(tokenClass, "holderDeviceId", "Ljava/lang/String;");
    jstring holderDeviceId = toJavaString(env, token.holder_device_id);
    env->SetObjectField(tokenObj, holderDeviceIdField, holderDeviceId);
    
    jfieldID issueTimeField = env->GetFieldID(tokenClass, "issueTime", "J");
    env->SetLongField(tokenObj, issueTimeField, token.issue_time);
    
    jfieldID expireTimeField = env->GetFieldID(tokenClass, "expireTime", "J");
    env->SetLongField(tokenObj, expireTimeField, token.expire_time);
    
    jfieldID signatureField = env->GetFieldID(tokenClass, "signature", "Ljava/lang/String;");
    jstring signature = toJavaString(env, token.signature);
    env->SetObjectField(tokenObj, signatureField, signature);
    
    return tokenObj;
}

JNIEXPORT jint JNICALL Java_com_decentrilicense_DecentriLicenseClient_setProductPublicKeyClient
  (JNIEnv *env, jobject obj, jlong handle, jstring productPublicKeyFileContent) {
    DL_Client* client = reinterpret_cast<DL_Client*>(handle);
    if (client == nullptr) {
        return -1;
    }
    char* pk = toCString(env, productPublicKeyFileContent);
    DL_ErrorCode rc = dl_client_set_product_public_key(client, pk);
    free(pk);
    return static_cast<jint>(rc);
}

JNIEXPORT jint JNICALL Java_com_decentrilicense_DecentriLicenseClient_importTokenClient
  (JNIEnv *env, jobject obj, jlong handle, jstring tokenInput) {
    DL_Client* client = reinterpret_cast<DL_Client*>(handle);
    if (client == nullptr) {
        return -1;
    }
    char* ti = toCString(env, tokenInput);
    DL_ErrorCode rc = dl_client_import_token(client, ti);
    free(ti);
    return static_cast<jint>(rc);
}

JNIEXPORT jstring JNICALL Java_com_decentrilicense_DecentriLicenseClient_getCurrentTokenJsonClient
  (JNIEnv *env, jobject obj, jlong handle) {
    DL_Client* client = reinterpret_cast<DL_Client*>(handle);
    if (client == nullptr) {
        return nullptr;
    }
    const size_t kBufSize = 65536;
    char* buf = static_cast<char*>(calloc(1, kBufSize));
    if (buf == nullptr) {
        return nullptr;
    }
    DL_ErrorCode rc = dl_client_get_current_token_json(client, buf, kBufSize);
    if (rc != DL_ERROR_SUCCESS) {
        free(buf);
        return nullptr;
    }
    jstring out = toJavaString(env, buf);
    free(buf);
    return out;
}

JNIEXPORT jstring JNICALL Java_com_decentrilicense_DecentriLicenseClient_exportCurrentTokenEncryptedClient
  (JNIEnv *env, jobject obj, jlong handle) {
    DL_Client* client = reinterpret_cast<DL_Client*>(handle);
    if (client == nullptr) {
        return nullptr;
    }
    const size_t kBufSize = 65536;
    char* buf = static_cast<char*>(calloc(1, kBufSize));
    if (buf == nullptr) {
        return nullptr;
    }
    DL_ErrorCode rc = dl_client_export_current_token_encrypted(client, buf, kBufSize);
    if (rc != DL_ERROR_SUCCESS) {
        free(buf);
        return nullptr;
    }
    jstring out = toJavaString(env, buf);
    free(buf);
    return out;
}

JNIEXPORT jobject JNICALL Java_com_decentrilicense_DecentriLicenseClient_offlineVerifyCurrentTokenClient
  (JNIEnv *env, jobject obj, jlong handle) {
    DL_Client* client = reinterpret_cast<DL_Client*>(handle);
    if (client == nullptr) {
        return nullptr;
    }
    DL_VerificationResult vr;
    memset(&vr, 0, sizeof(vr));
    DL_ErrorCode rc = dl_client_offline_verify_current_token(client, &vr);
    if (rc != DL_ERROR_SUCCESS) {
        return nullptr;
    }
    return createVerificationResult(env, vr);
}

JNIEXPORT jobject JNICALL Java_com_decentrilicense_DecentriLicenseClient_getStatusClient
  (JNIEnv *env, jobject obj, jlong handle) {
    DL_Client* client = reinterpret_cast<DL_Client*>(handle);
    if (client == nullptr) {
        return nullptr;
    }
    DL_StatusResult st;
    memset(&st, 0, sizeof(st));
    DL_ErrorCode rc = dl_client_get_status(client, &st);
    if (rc != DL_ERROR_SUCCESS) {
        return nullptr;
    }
    return createStatusResult(env, st);
}

JNIEXPORT jobject JNICALL Java_com_decentrilicense_DecentriLicenseClient_activateBindDeviceClient
  (JNIEnv *env, jobject obj, jlong handle) {
    DL_Client* client = reinterpret_cast<DL_Client*>(handle);
    if (client == nullptr) {
        return nullptr;
    }
    DL_VerificationResult vr;
    memset(&vr, 0, sizeof(vr));
    DL_ErrorCode rc = dl_client_activate_bind_device(client, &vr);
    if (rc != DL_ERROR_SUCCESS) {
        return nullptr;
    }
    return createVerificationResult(env, vr);
}

JNIEXPORT jobject JNICALL Java_com_decentrilicense_DecentriLicenseClient_recordUsageClient
  (JNIEnv *env, jobject obj, jlong handle, jstring newStatePayloadJson) {
    DL_Client* client = reinterpret_cast<DL_Client*>(handle);
    if (client == nullptr) {
        return nullptr;
    }
    char* payload = toCString(env, newStatePayloadJson);
    DL_VerificationResult vr;
    memset(&vr, 0, sizeof(vr));
    DL_ErrorCode rc = dl_client_record_usage(client, payload, &vr);
    free(payload);
    if (rc != DL_ERROR_SUCCESS) {
        return nullptr;
    }
    return createVerificationResult(env, vr);
}

JNIEXPORT jstring JNICALL Java_com_decentrilicense_DecentriLicenseClient_exportActivatedTokenEncryptedClient
  (JNIEnv *env, jobject obj, jlong handle) {
    DL_Client* client = reinterpret_cast<DL_Client*>(handle);
    if (client == nullptr) {
        return nullptr;
    }
    const size_t kBufSize = 65536;
    char* buf = static_cast<char*>(calloc(1, kBufSize));
    if (buf == nullptr) {
        return nullptr;
    }
    DL_ErrorCode rc = dl_client_export_activated_token_encrypted(client, buf, kBufSize);
    if (rc != DL_ERROR_SUCCESS) {
        free(buf);
        return nullptr;
    }
    jstring out = toJavaString(env, buf);
    free(buf);
    return out;
}

JNIEXPORT jstring JNICALL Java_com_decentrilicense_DecentriLicenseClient_exportStateChangedTokenEncryptedClient
  (JNIEnv *env, jobject obj, jlong handle) {
    DL_Client* client = reinterpret_cast<DL_Client*>(handle);
    if (client == nullptr) {
        return nullptr;
    }
    const size_t kBufSize = 65536;
    char* buf = static_cast<char*>(calloc(1, kBufSize));
    if (buf == nullptr) {
        return nullptr;
    }
    DL_ErrorCode rc = dl_client_export_state_changed_token_encrypted(client, buf, kBufSize);
    if (rc != DL_ERROR_SUCCESS) {
        free(buf);
        return nullptr;
    }
    jstring out = toJavaString(env, buf);
    free(buf);
    return out;
}

// Check if activated
JNIEXPORT jboolean JNICALL Java_com_decentrilicense_DecentriLicenseClient_isActivatedClient
  (JNIEnv *env, jobject obj, jlong handle) {
    DL_Client* client = reinterpret_cast<DL_Client*>(handle);
    if (client == nullptr) {
        return JNI_FALSE;
    }
    
    int result = dl_client_is_activated(client);
    return result ? JNI_TRUE : JNI_FALSE;
}

// Get device ID
JNIEXPORT jstring JNICALL Java_com_decentrilicense_DecentriLicenseClient_getDeviceIdClient
  (JNIEnv *env, jobject obj, jlong handle) {
    DL_Client* client = reinterpret_cast<DL_Client*>(handle);
    if (client == nullptr) {
        return env->NewStringUTF("");
    }
    
    char device_id[256];
    DL_ErrorCode error = dl_client_get_device_id(client, device_id, sizeof(device_id));
    
    if (error != DL_ERROR_SUCCESS) {
        return env->NewStringUTF("");
    }
    
    return env->NewStringUTF(device_id);
}

// Get device state
JNIEXPORT jint JNICALL Java_com_decentrilicense_DecentriLicenseClient_getDeviceStateClient
  (JNIEnv *env, jobject obj, jlong handle) {
    DL_Client* client = reinterpret_cast<DL_Client*>(handle);
    if (client == nullptr) {
        return 0;
    }
    
    DL_DeviceState state = dl_client_get_device_state(client);
    return static_cast<jint>(state);
}

// Shutdown client
JNIEXPORT jint JNICALL Java_com_decentrilicense_DecentriLicenseClient_shutdownClient
  (JNIEnv *env, jobject obj, jlong handle) {
    DL_Client* client = reinterpret_cast<DL_Client*>(handle);
    if (client == nullptr) {
        return -1;
    }
    
    DL_ErrorCode result = dl_client_shutdown(client);
    return static_cast<jint>(result);
}

// Destroy client
JNIEXPORT void JNICALL Java_com_decentrilicense_DecentriLicenseClient_destroyClient
  (JNIEnv *env, jobject obj, jlong handle) {
    DL_Client* client = reinterpret_cast<DL_Client*>(handle);
    if (client != nullptr) {
        dl_client_destroy(client);
    }
}

#ifdef __cplusplus
}
#endif