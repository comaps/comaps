import CoreLocation

/// A place the watch can ask the phone to route to (a bookmark on the phone).
/// The dictionary keys must stay in sync with MWMWatchMapExtractor.mm.
struct WatchDestination: Identifiable {
  static let nameKey = "n"
  static let latitudeKey = "la"
  static let longitudeKey = "lo"

  let name: String
  let coordinate: CLLocationCoordinate2D

  var id: String { "\(name)|\(coordinate.latitude)|\(coordinate.longitude)" }

  /// "Home" gets pinned to the top of the list with a house icon.
  var isHome: Bool { name.compare("home", options: [.caseInsensitive, .diacriticInsensitive]) == .orderedSame }

  var dictionary: [String: Any] {
    [Self.nameKey: name,
     Self.latitudeKey: coordinate.latitude,
     Self.longitudeKey: coordinate.longitude]
  }

  init?(dictionary: [String: Any]) {
    guard let name = dictionary[Self.nameKey] as? String,
          let latitude = dictionary[Self.latitudeKey] as? Double,
          let longitude = dictionary[Self.longitudeKey] as? Double
    else { return nil }
    self.name = name
    self.coordinate = CLLocationCoordinate2D(latitude: latitude, longitude: longitude)
  }
}
