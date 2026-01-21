package crypto

/*
#include <stdlib.h>
*/
import (
	"crypto/sha256"
)

// ComputeLicenseKeyHash computes the SHA256 hash of a license public key for use as AES key
func ComputeLicenseKeyHash(licensePublicKeyPEM string) []byte {
	// Extract the actual public key PEM from the file content
	// The file may contain additional data after the PEM block
	actualPublicKeyPEM := licensePublicKeyPEM
	rootSigPos := len(licensePublicKeyPEM)
	for i := 0; i < len(licensePublicKeyPEM)-15; i++ {
		if licensePublicKeyPEM[i] == '\n' &&
			licensePublicKeyPEM[i+1] == 'R' &&
			licensePublicKeyPEM[i+2] == 'O' &&
			licensePublicKeyPEM[i+3] == 'O' &&
			licensePublicKeyPEM[i+4] == 'T' &&
			licensePublicKeyPEM[i+5] == '_' &&
			licensePublicKeyPEM[i+6] == 'S' &&
			licensePublicKeyPEM[i+7] == 'I' &&
			licensePublicKeyPEM[i+8] == 'G' &&
			licensePublicKeyPEM[i+9] == 'N' &&
			licensePublicKeyPEM[i+10] == 'A' &&
			licensePublicKeyPEM[i+11] == 'T' &&
			licensePublicKeyPEM[i+12] == 'U' &&
			licensePublicKeyPEM[i+13] == 'R' &&
			licensePublicKeyPEM[i+14] == 'E' &&
			licensePublicKeyPEM[i+15] == ':' {
			rootSigPos = i
			break
		}
	}

	if rootSigPos < len(licensePublicKeyPEM) {
		actualPublicKeyPEM = licensePublicKeyPEM[:rootSigPos]
	}

	// Generate SHA256 hash of the license public key
	hash := sha256.Sum256([]byte(actualPublicKeyPEM))
	return hash[:]
}
