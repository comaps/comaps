import Foundation
import CryptoKit

/// The class responsible for executing Swift functions
@objc class Bridge: NSObject {
  @objc static func verifyRegionsFile(rawPublicKey: Data, rawData: Data, dataSize: Int, rawSignature: Data) -> Bool {
        if rawData.bytes.byteCount == dataSize, let publicKey = try? Curve25519.Signing.PublicKey(rawRepresentation: rawPublicKey), publicKey.isValidSignature(rawSignature, for: rawData) {
            return true
        }
        return false
    }
}
