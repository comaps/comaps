import Foundation
import CryptoKit

/// The class responsible for executing Swift functions
@objc class Bridge: NSObject {
  @objc static func verifyRegionsFile(rawPublicKey: Data, rawData: Data, rawSignature: Data) -> Bool {
        if let publicKey = try? Curve25519.Signing.PublicKey(rawRepresentation: rawPublicKey), publicKey.isValidSignature(rawSignature, for: rawData) {
            return true
        }
        return false
    }
}
