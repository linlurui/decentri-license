#import "DecentriLicenseClient.h"
#import "decenlicense_c.h"
#include <string.h>

// Error domain
static NSString * const DLErrorDomain = @"com.decentrilicense";

// Helper: create NSError from error code
static NSError * _Nullable dlError(NSInteger code, NSString *context) {
    return [NSError errorWithDomain:DLErrorDomain
                               code:code
                           userInfo:@{NSLocalizedDescriptionKey: [NSString stringWithFormat:@"%@: error code=%ld", context, (long)code]}];
}

#pragma mark - DLVerificationResult

@implementation DLVerificationResult
@end

#pragma mark - DLStatusResult

@implementation DLStatusResult
@end

#pragma mark - DecentriLicenseClient

@interface DecentriLicenseClient ()
@property (nonatomic, assign) DL_Client *client;
@property (nonatomic, assign) BOOL initialized;
@end

@implementation DecentriLicenseClient

- (instancetype)init {
    self = [super init];
    if (self) {
        _client = dl_client_create();
        _initialized = NO;
    }
    return self;
}

- (void)dealloc {
    if (_client) {
        if (_initialized) {
            dl_client_shutdown(_client);
        }
        dl_client_destroy(_client);
        _client = NULL;
    }
}

- (BOOL)initializeWithUdpPort:(uint16_t)udpPort
                      tcpPort:(uint16_t)tcpPort
                        error:(NSError *_Nullable *_Nullable)error {
    if (!_client) {
        if (error) *error = dlError(-1, @"Client not created");
        return NO;
    }

    DL_ClientConfig config = {0};
    config.license_code = "";
    config.preferred_mode = DL_CONNECTION_MODE_WAN_REGISTRY;
    config.udp_port = udpPort;
    config.tcp_port = tcpPort;
    config.registry_server_url = "";

    DL_ErrorCode rc = dl_client_initialize(_client, &config);
    if (rc != DL_ERROR_SUCCESS) {
        if (error) *error = dlError(rc, @"dl_client_initialize");
        return NO;
    }
    _initialized = YES;
    return YES;
}

- (BOOL)setProductPublicKey:(NSString *)content
                       error:(NSError *_Nullable *_Nullable)error {
    if (!_initialized) {
        if (error) *error = dlError(DL_ERROR_NOT_INITIALIZED, @"Client not initialized");
        return NO;
    }
    DL_ErrorCode rc = dl_client_set_product_public_key(_client, [content UTF8String]);
    if (rc != DL_ERROR_SUCCESS) {
        if (error) *error = dlError(rc, @"dl_client_set_product_public_key");
        return NO;
    }
    return YES;
}

- (BOOL)importToken:(NSString *)tokenInput
              error:(NSError *_Nullable *_Nullable)error {
    if (!_initialized) {
        if (error) *error = dlError(DL_ERROR_NOT_INITIALIZED, @"Client not initialized");
        return NO;
    }
    DL_ErrorCode rc = dl_client_import_token(_client, [tokenInput UTF8String]);
    if (rc != DL_ERROR_SUCCESS) {
        if (error) *error = dlError(rc, @"dl_client_import_token");
        return NO;
    }
    return YES;
}

- (nullable DLVerificationResult *)offlineVerifyCurrentToken:(NSError *_Nullable *_Nullable)error {
    if (!_initialized) {
        if (error) *error = dlError(DL_ERROR_NOT_INITIALIZED, @"Client not initialized");
        return nil;
    }
    DL_VerificationResult vr = {0};
    DL_ErrorCode rc = dl_client_offline_verify_current_token(_client, &vr);
    if (rc != DL_ERROR_SUCCESS) {
        if (error) *error = dlError(rc, @"dl_client_offline_verify_current_token");
        return nil;
    }
    DLVerificationResult *result = [[DLVerificationResult alloc] init];
    result.valid = (vr.valid == 1);
    result.errorMessage = @(vr.error_message);
    return result;
}

- (nullable DLStatusResult *)getStatus:(NSError *_Nullable *_Nullable)error {
    if (!_initialized) {
        if (error) *error = dlError(DL_ERROR_NOT_INITIALIZED, @"Client not initialized");
        return nil;
    }
    DL_StatusResult sr = {0};
    DL_ErrorCode rc = dl_client_get_status(_client, &sr);
    if (rc != DL_ERROR_SUCCESS) {
        if (error) *error = dlError(rc, @"dl_client_get_status");
        return nil;
    }
    DLStatusResult *result = [[DLStatusResult alloc] init];
    result.hasToken = (sr.has_token == 1);
    result.isActivated = (sr.is_activated == 1);
    result.issueTime = sr.issue_time;
    result.expireTime = sr.expire_time;
    result.stateIndex = sr.state_index;
    result.tokenId = @(sr.token_id);
    result.holderDeviceId = @(sr.holder_device_id);
    result.appId = @(sr.app_id);
    result.licenseCode = @(sr.license_code);
    return result;
}

- (nullable DLVerificationResult *)activateBindDevice:(NSError *_Nullable *_Nullable)error {
    if (!_initialized) {
        if (error) *error = dlError(DL_ERROR_NOT_INITIALIZED, @"Client not initialized");
        return nil;
    }
    DL_VerificationResult vr = {0};
    DL_ErrorCode rc = dl_client_activate_bind_device(_client, &vr);
    if (rc != DL_ERROR_SUCCESS) {
        if (error) *error = dlError(rc, @"dl_client_activate_bind_device");
        return nil;
    }
    DLVerificationResult *result = [[DLVerificationResult alloc] init];
    result.valid = (vr.valid == 1);
    result.errorMessage = @(vr.error_message);
    return result;
}

- (nullable DLVerificationResult *)recordUsage:(NSString *)payloadJson
                                         error:(NSError *_Nullable *_Nullable)error {
    if (!_initialized) {
        if (error) *error = dlError(DL_ERROR_NOT_INITIALIZED, @"Client not initialized");
        return nil;
    }
    DL_VerificationResult vr = {0};
    DL_ErrorCode rc = dl_client_record_usage(_client, [payloadJson UTF8String], &vr);
    if (rc != DL_ERROR_SUCCESS) {
        if (error) *error = dlError(rc, @"dl_client_record_usage");
        return nil;
    }
    DLVerificationResult *result = [[DLVerificationResult alloc] init];
    result.valid = (vr.valid == 1);
    result.errorMessage = @(vr.error_message);
    return result;
}

- (nullable NSString *)exportCurrentTokenEncrypted:(NSError *_Nullable *_Nullable)error {
    if (!_initialized) {
        if (error) *error = dlError(DL_ERROR_NOT_INITIALIZED, @"Client not initialized");
        return nil;
    }
    char buf[8192] = {0};
    DL_ErrorCode rc = dl_client_export_current_token_encrypted(_client, buf, sizeof(buf));
    if (rc != DL_ERROR_SUCCESS) {
        if (error) *error = dlError(rc, @"dl_client_export_current_token_encrypted");
        return nil;
    }
    return @(buf);
}

- (nullable NSString *)exportActivatedTokenEncrypted:(NSError *_Nullable *_Nullable)error {
    if (!_initialized) {
        if (error) *error = dlError(DL_ERROR_NOT_INITIALIZED, @"Client not initialized");
        return nil;
    }
    char buf[8192] = {0};
    DL_ErrorCode rc = dl_client_export_activated_token_encrypted(_client, buf, sizeof(buf));
    if (rc != DL_ERROR_SUCCESS) {
        if (error) *error = dlError(rc, @"dl_client_export_activated_token_encrypted");
        return nil;
    }
    return @(buf);
}

- (nullable NSString *)exportStateChangedTokenEncrypted:(NSError *_Nullable *_Nullable)error {
    if (!_initialized) {
        if (error) *error = dlError(DL_ERROR_NOT_INITIALIZED, @"Client not initialized");
        return nil;
    }
    char buf[8192] = {0};
    DL_ErrorCode rc = dl_client_export_state_changed_token_encrypted(_client, buf, sizeof(buf));
    if (rc != DL_ERROR_SUCCESS) {
        if (error) *error = dlError(rc, @"dl_client_export_state_changed_token_encrypted");
        return nil;
    }
    return @(buf);
}

- (BOOL)isActivated {
    if (!_client) return NO;
    return dl_client_is_activated(_client) == 1;
}

- (nullable NSString *)getDeviceId:(NSError *_Nullable *_Nullable)error {
    if (!_initialized) {
        if (error) *error = dlError(DL_ERROR_NOT_INITIALIZED, @"Client not initialized");
        return nil;
    }
    char buf[128] = {0};
    DL_ErrorCode rc = dl_client_get_device_id(_client, buf, sizeof(buf));
    if (rc != DL_ERROR_SUCCESS) {
        if (error) *error = dlError(rc, @"dl_client_get_device_id");
        return nil;
    }
    return @(buf);
}

- (NSString *)getDeviceState {
    if (!_client) return @"idle";
    DL_DeviceState st = dl_client_get_device_state(_client);
    switch (st) {
        case DL_DEVICE_STATE_DISCOVERING: return @"discovering";
        case DL_DEVICE_STATE_ELECTING:    return @"electing";
        case DL_DEVICE_STATE_COORDINATOR: return @"coordinator";
        case DL_DEVICE_STATE_FOLLOWER:    return @"follower";
        default:                          return @"idle";
    }
}

- (nullable NSString *)getStatePayload:(NSError *_Nullable *_Nullable)error {
    if (!_initialized) {
        if (error) *error = dlError(DL_ERROR_NOT_INITIALIZED, @"Client not initialized");
        return nil;
    }
    char buf[65536] = {0};
    DL_ErrorCode rc = dl_client_get_state_payload(_client, buf, sizeof(buf));
    if (rc != DL_ERROR_SUCCESS) {
        if (error) *error = dlError(rc, @"dl_client_get_state_payload");
        return nil;
    }
    return @(buf);
}

- (nullable DLVerificationResult *)addRecoveryChannel:(NSString *)password
                                                error:(NSError *_Nullable *_Nullable)error {
    if (!_initialized) {
        if (error) *error = dlError(DL_ERROR_NOT_INITIALIZED, @"Client not initialized");
        return nil;
    }
    DL_VerificationResult vr = {0};
    DL_ErrorCode rc = dl_client_add_recovery_channel(_client, [password UTF8String], &vr);
    if (rc != DL_ERROR_SUCCESS) {
        if (error) *error = dlError(rc, @"dl_client_add_recovery_channel");
        return nil;
    }
    DLVerificationResult *result = [[DLVerificationResult alloc] init];
    result.valid = (vr.valid == 1);
    result.errorMessage = @(vr.error_message);
    return result;
}

- (nullable DLVerificationResult *)removeRecoveryChannel:(NSError *_Nullable *_Nullable)error {
    if (!_initialized) {
        if (error) *error = dlError(DL_ERROR_NOT_INITIALIZED, @"Client not initialized");
        return nil;
    }
    DL_VerificationResult vr = {0};
    DL_ErrorCode rc = dl_client_remove_recovery_channel(_client, &vr);
    if (rc != DL_ERROR_SUCCESS) {
        if (error) *error = dlError(rc, @"dl_client_remove_recovery_channel");
        return nil;
    }
    DLVerificationResult *result = [[DLVerificationResult alloc] init];
    result.valid = (vr.valid == 1);
    result.errorMessage = @(vr.error_message);
    return result;
}

- (BOOL)shutdown:(NSError *_Nullable *_Nullable)error {
    if (!_client) return YES;
    DL_ErrorCode rc = dl_client_shutdown(_client);
    dl_client_destroy(_client);
    _client = NULL;
    _initialized = NO;
    if (rc != DL_ERROR_SUCCESS) {
        if (error) *error = dlError(rc, @"dl_client_shutdown");
        return NO;
    }
    return YES;
}

@end
