import Foundation
import CoreLocation
import UIKit

/// Service managing the location sharing lifecycle
@objc class LocationSharingService: NSObject {

  @objc static let shared = LocationSharingService()

  private var locationManager: CLLocationManager?
  private var apiClient: LocationSharingApiClient?
  private let session = LocationSharingSession.shared
  private var updateTimer: Timer?

  // Battery monitoring
  private var batteryLevelObserver: NSObjectProtocol?

  private override init() {
    super.init()
    setupSession()
  }

  deinit {
    stopSharing()
  }

  // MARK: - Public API

  /// Start sharing location
  @objc func startSharing() -> String? {
    let config = LocationSharingConfig()

    guard let shareUrl = session.start(with: config) else {
      NSLog("Failed to start location sharing session")
      return nil
    }

    // Initialize API client
    guard let credentials = session.credentials else {
      return nil
    }

    apiClient = LocationSharingApiClient(
      serverBaseUrl: config.serverBaseUrl,
      sessionId: credentials.sessionId)

    // Request location authorization if needed
    setupLocationManager()
    requestLocationAuthorization()

    // Start location updates
    startLocationUpdates()

    // Setup battery monitoring
    setupBatteryMonitoring()

    // Send session creation to server
    apiClient?.createSession()

    NSLog("Location sharing service started")

    return shareUrl
  }

  /// Stop sharing location
  @objc func stopSharing() {
    // Stop location updates
    stopLocationUpdates()

    // Stop battery monitoring
    stopBatteryMonitoring()

    // End session on server
    apiClient?.endSession()
    apiClient = nil

    // Stop session
    session.stop()

    NSLog("Location sharing service stopped")
  }

  /// Check if currently sharing
  @objc var isSharing: Bool {
    return session.state == .active
  }

  /// Get current share URL
  @objc var shareUrl: String? {
    return session.shareUrl
  }

  // MARK: - Location Management

  private func setupLocationManager() {
    if locationManager == nil {
      locationManager = CLLocationManager()
      locationManager?.delegate = self
      locationManager?.desiredAccuracy = kCLLocationAccuracyBest
      locationManager?.allowsBackgroundLocationUpdates = true
      locationManager?.pausesLocationUpdatesAutomatically = false
      locationManager?.showsBackgroundLocationIndicator = true
    }
  }

  private func requestLocationAuthorization() {
    guard let manager = locationManager else { return }

    let status = CLLocationManager.authorizationStatus()

    switch status {
    case .notDetermined:
      manager.requestWhenInUseAuthorization()
    case .authorizedWhenInUse:
      manager.requestAlwaysAuthorization()
    case .authorizedAlways:
      break
    case .denied, .restricted:
      NSLog("Location authorization denied or restricted")
      session.onError?("Location permission required")
    @unknown default:
      break
    }
  }

  private func startLocationUpdates() {
    locationManager?.startUpdatingLocation()

    // Also monitor significant location changes for battery efficiency
    locationManager?.startMonitoringSignificantLocationChanges()
  }

  private func stopLocationUpdates() {
    locationManager?.stopUpdatingLocation()
    locationManager?.stopMonitoringSignificantLocationChanges()
  }

  // MARK: - Battery Monitoring

  private func setupBatteryMonitoring() {
    UIDevice.current.isBatteryMonitoringEnabled = true

    batteryLevelObserver = NotificationCenter.default.addObserver(
      forName: UIDevice.batteryLevelDidChangeNotification,
      object: nil,
      queue: .main) { [weak self] _ in
      self?.updateBatteryLevel()
    }

    // Initial battery level
    updateBatteryLevel()
  }

  private func stopBatteryMonitoring() {
    if let observer = batteryLevelObserver {
      NotificationCenter.default.removeObserver(observer)
      batteryLevelObserver = nil
    }

    UIDevice.current.isBatteryMonitoringEnabled = false
  }

  private func updateBatteryLevel() {
    let level = Int(UIDevice.current.batteryLevel * 100)
    if level >= 0 {
      session.updateBatteryLevel(level)
    }
  }

  // MARK: - Session Setup

  private func setupSession() {
    session.onStateChange = { [weak self] state in
      self?.handleStateChange(state)
    }

    session.onError = { error in
      NSLog("Location sharing error: \(error)")
    }

    session.onPayloadReady = { [weak self] data in
      self?.sendPayloadToServer(data)
    }
  }

  private func handleStateChange(_ state: LocationSharingState) {
    switch state {
    case .active:
      LocationSharingNotifier.shared.scheduleActiveNotification()
    case .inactive:
      LocationSharingNotifier.shared.cancelNotifications()
    case .error:
      LocationSharingNotifier.shared.cancelNotifications()
    default:
      break
    }
  }

  // MARK: - Server Communication

  private func sendPayloadToServer(_ data: Data) {
    guard let jsonString = String(data: data, encoding: .utf8) else {
      NSLog("Failed to convert payload to string")
      return
    }

    apiClient?.updateLocation(encryptedPayload: jsonString) { success, error in
      if success {
        NSLog("Location update sent successfully")
      } else {
        NSLog("Failed to send location update: \(error ?? "unknown error")")
      }
    }
  }
}

// MARK: - CLLocationManagerDelegate

extension LocationSharingService: CLLocationManagerDelegate {

  func locationManager(_ manager: CLLocationManager, didUpdateLocations locations: [CLLocation]) {
    guard let location = locations.last else { return }

    // Update session with new location
    session.updateLocation(location)

    // Update notification with current location
    LocationSharingNotifier.shared.updateNotification(location: location)

    // TODO: Check if navigation is active and update navigation info
    // This would integrate with the routing framework
  }

  func locationManager(_ manager: CLLocationManager, didFailWithError error: Error) {
    NSLog("Location manager error: \(error.localizedDescription)")
    session.onError?(error.localizedDescription)
  }

  func locationManager(_ manager: CLLocationManager, didChangeAuthorization status: CLAuthorizationStatus) {
    switch status {
    case .authorizedAlways, .authorizedWhenInUse:
      if isSharing {
        startLocationUpdates()
      }
    case .denied, .restricted:
      session.onError?("Location permission denied")
      stopSharing()
    default:
      break
    }
  }
}
