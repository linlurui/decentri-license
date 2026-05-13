# DecentriLicense Objective-C SDK

Objective-C wrapper for the DecentriLicense `dl-core` C library.

## Files

- `DecentriLicenseClient.h` — Public header with ObjC interface
- `DecentriLicenseClient.m` — Implementation (calls dl-core C API directly)
- `validation_wizard.m` — Interactive validation wizard CLI

## Build

```bash
# Build dl-core first
cd dl-core && mkdir build && cd build && cmake .. && cmake --build .

# Compile wizard
cd sdks/objc
clang -framework Foundation \
      -I../../dl-core/include \
      -L../../dl-core/build \
      -ldecentrilicense \
      -o validation_wizard \
      validation_wizard.m DecentriLicenseClient.m

# Run
DYLD_LIBRARY_PATH=../../dl-core/build ./validation_wizard
```

## Quick Validate

```bash
./validation_wizard validate <token_file> <product_public_key_file>
```

## Usage

```objc
#import "DecentriLicenseClient.h"

DecentriLicenseClient *client = [[DecentriLicenseClient alloc] init];
[client initializeWithUdpPort:13325 tcpPort:23325 error:nil];
[client setProductPublicKey:productKeyPEM error:nil];
[client importToken:tokenString error:nil];

DLVerificationResult *result = [client offlineVerifyCurrentToken:nil];
if (result.valid) {
    NSLog(@"✅ Token is valid");
}

[client shutdown:nil];
```
