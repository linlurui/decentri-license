import Foundation
import CryptoKit

/// Compute the SHA256 hash of a license public key for use as AES key
/// Extracts the actual public key PEM from the file content (strips ROOT_SIGNATURE suffix if present)
public func computeLicenseKeyHash(_ licensePublicKeyPEM: String) -> Data {
    var actualPublicKeyPEM = licensePublicKeyPEM

    // Find and strip ROOT_SIGNATURE: suffix
    let marker = "\nROOT_SIGNATURE:"
    if let range = licensePublicKeyPEM.range(of: marker, options: .literal) {
        actualPublicKeyPEM = String(licensePublicKeyPEM[..<range.lowerBound])
    }

    let hash = SHA256.hash(data: Data(actualPublicKeyPEM.utf8))
    return Data(hash)
}
