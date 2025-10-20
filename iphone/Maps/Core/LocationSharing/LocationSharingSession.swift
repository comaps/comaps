import Foundation
import CoreLocation

/// Session state for location sharing
@objc enum LocationSharingState: Int {
  case inactive
  case starting
  case active
  case paused
  case stopping
  case error
}

/// Mode for location sharing
enum LocationSharingMode {
  case standalone  // GPS only
  case navigation  // GPS + ETA + distance
}

/// Configuration for location sharing session
struct LocationSharingConfig {
  var updateIntervalSeconds: Int = 20
  var includeDestinationName: Bool = true
  var includeBatteryLevel: Bool = true
  var lowBatteryThreshold: Int = 10
  var serverBaseUrl: String = "https://live.organicmaps.app"
}

/// Session credentials
struct LocationSharingCredentials {
  let sessionId: String
  let encryptionKey: String

  /// Generate share URL from credentials
  func generateShareUrl(serverBaseUrl: String) -> String {
    let combined = "\(sessionId):\(encryptionKey)"
    guard let data = combined.data(using: .utf8) else { return "" }
    let base64 = data.base64EncodedString()
      .replacingOccurrences(of: "+", with: "-")
      .replacingOccurrences(of: "/", with: "_")
      .replacingOccurrences(of: "=", with: "")

    var url = serverBaseUrl
    if !url.hasSuffix("/") {
      url += "/"
    }
    url += "live/\(base64)"

    return url
  }

  /// Generate new random credentials
  static func generate() -> LocationSharingCredentials {
    let sessionId = UUID().uuidString
    let encryptionKey = Self.generateRandomKey()
    return LocationSharingCredentials(sessionId: sessionId, encryptionKey: encryptionKey)
  }

  private static func generateRandomKey() -> String {
    var bytes = [UInt8](repeating: 0, count: 32)
    _ = SecRandomCopyBytes(kSecRandomDefault, bytes.count, &bytes)
    return Data(bytes).base64EncodedString()
  }
}

/// Location payload structure
struct LocationPayload {
  var timestamp: TimeInterval
  var latitude: Double
  var longitude: Double
  var accuracy: Double
  var speed: Double?
  var bearing: Double?
  var mode: LocationSharingMode = .standalone
  var eta: TimeInterval?
  var distanceRemaining: Int?
  var destinationName: String?
  var batteryLevel: Int?

  init(location: CLLocation) {
    self.timestamp = Date().timeIntervalSince1970
    self.latitude = location.coordinate.latitude
    self.longitude = location.coordinate.longitude
    self.accuracy = location.horizontalAccuracy

    if location.speed >= 0 {
      self.speed = location.speed
    }

    if location.course >= 0 {
      self.bearing = location.course
    }
  }

  func toJSON() -> [String: Any] {
    var json: [String: Any] = [
      "timestamp": Int(timestamp),
      "lat": latitude,
      "lon": longitude,
      "accuracy": accuracy
    ]

    if let speed = speed {
      json["speed"] = speed
    }

    if let bearing = bearing {
      json["bearing"] = bearing
    }

    json["mode"] = mode == .navigation ? "navigation" : "standalone"

    if mode == .navigation {
      if let eta = eta {
        json["eta"] = Int(eta)
      }
      if let distanceRemaining = distanceRemaining {
        json["distanceRemaining"] = distanceRemaining
      }
      if let destinationName = destinationName {
        json["destinationName"] = destinationName
      }
    }

    if let batteryLevel = batteryLevel {
      json["batteryLevel"] = batteryLevel
    }

    return json
  }
}

/// Main location sharing session manager
@objc class LocationSharingSession: NSObject {

  // Singleton instance
  @objc static let shared = LocationSharingSession()

  // State
  private(set) var state: LocationSharingState = .inactive
  private(set) var credentials: LocationSharingCredentials?
  private(set) var config: LocationSharingConfig = LocationSharingConfig()
  private(set) var shareUrl: String?

  // Current payload
  private var currentPayload: LocationPayload?
  private var lastUpdateTimestamp: TimeInterval = 0

  // Callbacks
  var onStateChange: ((LocationSharingState) -> Void)?
  var onError: ((String) -> Void)?
  var onPayloadReady: ((Data) -> Void)?

  private override init() {
    super.init()
  }

  /// Start location sharing session
  @objc func start(with config: LocationSharingConfig) -> String? {
    if state != .inactive {
      NSLog("Location sharing already active, stopping previous session")
      stop()
    }

    setState(.starting)

    self.config = config
    self.credentials = LocationSharingCredentials.generate()
    self.lastUpdateTimestamp = 0

    guard let credentials = self.credentials else {
      onError?("Failed to generate credentials")
      setState(.error)
      return nil
    }

    self.shareUrl = credentials.generateShareUrl(serverBaseUrl: config.serverBaseUrl)

    NSLog("Location sharing session started: \(credentials.sessionId)")
    setState(.active)

    return shareUrl
  }

  /// Stop location sharing session
  @objc func stop() {
    if state == .inactive {
      return
    }

    setState(.stopping)

    NSLog("Location sharing session stopped")

    currentPayload = nil
    credentials = nil
    shareUrl = nil
    lastUpdateTimestamp = 0

    setState(.inactive)
  }

  /// Update location
  func updateLocation(_ location: CLLocation) {
    guard state == .active else {
      NSLog("Cannot update location - session not active")
      return
    }

    currentPayload = LocationPayload(location: location)
    processLocationUpdate()
  }

  /// Update navigation info
  func updateNavigationInfo(eta: TimeInterval, distanceRemaining: Int, destinationName: String?) {
    guard state == .active, currentPayload != nil else { return }

    currentPayload?.mode = .navigation
    currentPayload?.eta = eta
    currentPayload?.distanceRemaining = distanceRemaining

    if config.includeDestinationName, let name = destinationName {
      currentPayload?.destinationName = name
    }
  }

  /// Clear navigation info
  func clearNavigationInfo() {
    currentPayload?.mode = .standalone
    currentPayload?.eta = nil
    currentPayload?.distanceRemaining = nil
    currentPayload?.destinationName = nil
  }

  /// Update battery level
  func updateBatteryLevel(_ level: Int) {
    guard state == .active else { return }

    if config.includeBatteryLevel {
      currentPayload?.batteryLevel = level
    }

    // Stop if battery too low
    if level < config.lowBatteryThreshold {
      NSLog("Battery level too low (\(level)%), stopping location sharing")
      onError?("Battery level too low")
      stop()
    }
  }

  // MARK: - Private methods

  private func setState(_ newState: LocationSharingState) {
    if state == newState {
      return
    }

    NSLog("Location sharing state: \(state.rawValue) -> \(newState.rawValue)")
    state = newState
    onStateChange?(newState)
  }

  private func processLocationUpdate() {
    guard shouldSendUpdate() else { return }

    guard let encryptedData = createEncryptedPayload() else {
      onError?("Failed to create encrypted payload")
      return
    }

    lastUpdateTimestamp = Date().timeIntervalSince1970
    onPayloadReady?(encryptedData)
  }

  private func shouldSendUpdate() -> Bool {
    guard currentPayload != nil else { return false }

    let now = Date().timeIntervalSince1970
    let timeSinceLastUpdate = now - lastUpdateTimestamp

    return timeSinceLastUpdate >= Double(config.updateIntervalSeconds)
  }

  private func createEncryptedPayload() -> Data? {
    guard let payload = currentPayload,
          let credentials = credentials else {
      return nil
    }

    let json = payload.toJSON()

    guard let jsonData = try? JSONSerialization.data(withJSONObject: json),
          let jsonString = String(data: jsonData, encoding: .utf8) else {
      NSLog("Failed to serialize payload to JSON")
      return nil
    }

    // Call native encryption (via bridge)
    guard let encryptedJson = LocationSharingBridge.encryptPayload(
      key: credentials.encryptionKey,
      plaintext: jsonString) else {
      NSLog("Encryption failed")
      return nil
    }

    return encryptedJson.data(using: .utf8)
  }
}

/// Swift wrapper for LocationSharingBridgeObjC
extension LocationSharingBridge {
  static func encryptPayload(key: String, plaintext: String) -> String? {
    return LocationSharingBridgeObjC.encryptPayload(withKey: key, plaintext: plaintext)
  }
}
