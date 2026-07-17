import Combine
import CoreLocation
import Foundation

/// Streams location fixes only while the app is on screen. watchOS suspends
/// the app on wrist-down, so this behaves like an on-demand fix per glance.
/// When the paired phone is in range, watchOS transparently uses the phone's
/// GPS, which keeps the cost of a glance negligible.
final class WatchLocationProvider: NSObject, ObservableObject {
  @Published private(set) var location: CLLocation?
  /// Compass heading in degrees from north, nil without a magnetometer fix.
  @Published private(set) var headingDegrees: Double?
  @Published private(set) var isDenied = false

  /// Direction the user is facing (compass), falling back to the direction of
  /// movement (GPS course) on watches without a working compass.
  var orientationDegrees: Double? {
    if let headingDegrees { return headingDegrees }
    if let course = location?.course, course >= 0 { return course }
    return nil
  }

  private let manager = CLLocationManager()

  override init() {
    super.init()
    manager.delegate = self
    manager.desiredAccuracy = kCLLocationAccuracyBest
    manager.activityType = .otherNavigation
    manager.headingFilter = 5
  }

  func start() {
    switch manager.authorizationStatus {
    case .notDetermined:
      manager.requestWhenInUseAuthorization()
    case .denied, .restricted:
      isDenied = true
    default:
      startUpdates()
    }
  }

  func stop() {
    manager.stopUpdatingLocation()
    if CLLocationManager.headingAvailable() {
      manager.stopUpdatingHeading()
    }
  }

  private func startUpdates() {
    manager.startUpdatingLocation()
    if CLLocationManager.headingAvailable() {
      manager.startUpdatingHeading()
    }
  }
}

extension WatchLocationProvider: CLLocationManagerDelegate {
  func locationManagerDidChangeAuthorization(_ manager: CLLocationManager) {
    switch manager.authorizationStatus {
    case .authorizedWhenInUse, .authorizedAlways:
      isDenied = false
      startUpdates()
    case .denied, .restricted:
      isDenied = true
    default:
      break
    }
  }

  func locationManager(_ manager: CLLocationManager, didUpdateLocations locations: [CLLocation]) {
    guard let last = locations.last, last.horizontalAccuracy >= 0 else { return }
    location = last
  }

  func locationManager(_ manager: CLLocationManager, didUpdateHeading newHeading: CLHeading) {
    guard newHeading.headingAccuracy >= 0 else { return }
    headingDegrees = newHeading.trueHeading >= 0 ? newHeading.trueHeading : newHeading.magneticHeading
  }

  func locationManager(_ manager: CLLocationManager, didFailWithError error: Error) {}
}
