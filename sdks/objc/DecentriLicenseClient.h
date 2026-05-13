#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// Verification result
@interface DLVerificationResult : NSObject
@property (nonatomic, assign) BOOL valid;
@property (nonatomic, copy) NSString *errorMessage;
@end

/// Status result
@interface DLStatusResult : NSObject
@property (nonatomic, assign) BOOL hasToken;
@property (nonatomic, assign) BOOL isActivated;
@property (nonatomic, assign) int64_t issueTime;
@property (nonatomic, assign) int64_t expireTime;
@property (nonatomic, assign) uint64_t stateIndex;
@property (nonatomic, copy) NSString *tokenId;
@property (nonatomic, copy) NSString *holderDeviceId;
@property (nonatomic, copy) NSString *appId;
@property (nonatomic, copy) NSString *licenseCode;
@end

/// DecentriLicense Objective-C Client
@interface DecentriLicenseClient : NSObject

/// Initialize client with ports
- (BOOL)initializeWithUdpPort:(uint16_t)udpPort
                      tcpPort:(uint16_t)tcpPort
                        error:(NSError *_Nullable *_Nullable)error;

/// Set product public key (PEM content)
- (BOOL)setProductPublicKey:(NSString *)content
                       error:(NSError *_Nullable *_Nullable)error;

/// Import token (encrypted string or JSON)
- (BOOL)importToken:(NSString *)tokenInput
              error:(NSError *_Nullable *_Nullable)error;

/// Offline verify current token
- (nullable DLVerificationResult *)offlineVerifyCurrentToken:(NSError *_Nullable *_Nullable)error;

/// Get status
- (nullable DLStatusResult *)getStatus:(NSError *_Nullable *_Nullable)error;

/// Activate and bind device
- (nullable DLVerificationResult *)activateBindDevice:(NSError *_Nullable *_Nullable)error;

/// Record usage / state change
- (nullable DLVerificationResult *)recordUsage:(NSString *)payloadJson
                                         error:(NSError *_Nullable *_Nullable)error;

/// Export current token encrypted
- (nullable NSString *)exportCurrentTokenEncrypted:(NSError *_Nullable *_Nullable)error;

/// Export activated token encrypted
- (nullable NSString *)exportActivatedTokenEncrypted:(NSError *_Nullable *_Nullable)error;

/// Export state-changed token encrypted
- (nullable NSString *)exportStateChangedTokenEncrypted:(NSError *_Nullable *_Nullable)error;

/// Check if activated
- (BOOL)isActivated;

/// Get device ID
- (nullable NSString *)getDeviceId:(NSError *_Nullable *_Nullable)error;

/// Get device state string
- (NSString *)getDeviceState;

/// Get plaintext state_payload (decrypted from SEK if applicable)
- (nullable NSString *)getStatePayload:(NSError *_Nullable *_Nullable)error;

/// Add recovery channel (password/mnemonic) to wrap SEK
- (nullable DLVerificationResult *)addRecoveryChannel:(NSString *)password
                                                error:(NSError *_Nullable *_Nullable)error;

/// Remove recovery channel (clears password-encrypted SEK)
- (nullable DLVerificationResult *)removeRecoveryChannel:(NSError *_Nullable *_Nullable)error;

/// Shutdown client
- (BOOL)shutdown:(NSError *_Nullable *_Nullable)error;

@end

NS_ASSUME_NONNULL_END
