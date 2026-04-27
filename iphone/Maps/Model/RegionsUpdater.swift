import CryptoKit

/// The class responsible for managing regions
@objc class RegionalManager: NSObject {
    static func verifyRegionsFile(rawPublicKey: Data, rawData: Data, dataSize: Int? = nil, rawSignature: Data) -> Bool {
        if let publicKey = try? Curve25519.Signing.PublicKey(rawRepresentation: rawPublicKey), publicKey.isValidSignature(rawSignature, for: rawData), rawData.bytes.byteCount == dataSize {
            return true
        }
        
        return false
    }
}
