import CoreLocation

/// Route snapshot exchanged between the phone and the watch through the
/// WatchConnectivity application context. Coordinates travel as packed
/// Float64 lat/lon pairs to stay well under the transfer size limits.
/// Next-turn snapshot streamed by the phone during navigation. The dictionary
/// keys must stay in sync with MWMWatchMapExtractor.mm.
struct WatchTurnInfo {
  static let symbolKey = "tSym"
  static let distanceKey = "tDist"
  static let streetKey = "tStreet"
  static let timestampKey = "ts"

  /// SF Symbol depicting the maneuver, e.g. "arrow.turn.up.left".
  let symbolName: String
  /// Localized distance to the maneuver, formatted by the phone.
  let distanceText: String
  /// Preformatted name of the road after the maneuver. Nil for unnamed roads.
  let streetName: String?
  let timestamp: Date

  var isStale: Bool { Date().timeIntervalSince(timestamp) > 30 }

  init?(dictionary: [String: Any]) {
    guard let symbolName = dictionary[Self.symbolKey] as? String,
          let distanceText = dictionary[Self.distanceKey] as? String
    else { return nil }
    self.symbolName = symbolName
    self.distanceText = distanceText
    self.streetName = (dictionary[Self.streetKey] as? String).flatMap { $0.isEmpty ? nil : $0 }
    self.timestamp = dictionary[Self.timestampKey] as? Date ?? Date()
  }
}

struct AltitudeSample {
  /// Distance from the route start, meters.
  let distance: Double
  /// Altitude above sea level, meters.
  let altitude: Double
}

struct WatchRoutePayload {
  static let versionKey = "version"
  static let activeKey = "active"
  static let coordinatesKey = "coords"
  static let routerTypeKey = "router"
  static let destinationKey = "destination"
  static let timestampKey = "timestamp"
  /// Packed Float32 (distance, altitude) pairs in meters; absent when the
  /// route has no altitude information (e.g. car routes).
  static let altitudesKey = "alt"
  static let ascentKey = "ascent"
  static let descentKey = "descent"
  /// Message sent by the watch to ask the phone for the current context.
  static let refreshRequestKey = "requestRoute"
  /// Message sent by the watch to ask the phone for its bookmark list; the
  /// reply carries an array of WatchDestination dictionaries under
  /// destinationsKey.
  static let destinationsRequestKey = "requestDestinations"
  static let destinationsKey = "destinations"
  /// Message sent by the watch to ask the phone to build a route to a
  /// WatchDestination dictionary.
  static let routeRequestKey = "requestRouteTo"
  /// Message sent by the watch to save its current location as a bookmark
  /// (a WatchDestination dictionary); the reply carries the refreshed
  /// destination list under destinationsKey.
  static let addBookmarkKey = "addBookmark"
  /// Fire-and-forget message from the phone with a WatchTurnInfo dictionary
  /// while a navigation session is running.
  static let turnInfoKey = "turnInfo"
  /// Message sent by the watch to stop the active route on the phone; the
  /// reply carries the (now inactive) context.
  static let stopRouteKey = "stopRoute"

  static let version = 2

  var coordinates: [CLLocationCoordinate2D]
  var routerType: String
  var destinationName: String
  var timestamp: Date
  var altitudeData: Data?
  var ascent: Double
  var descent: Double

  init(coordinates: [CLLocationCoordinate2D], routerType: String, destinationName: String,
       timestamp: Date = Date(), altitudeData: Data? = nil, ascent: Double = 0, descent: Double = 0) {
    self.coordinates = coordinates
    self.routerType = routerType
    self.destinationName = destinationName
    self.timestamp = timestamp
    self.altitudeData = altitudeData
    self.ascent = ascent
    self.descent = descent
  }

  var altitudeSamples: [AltitudeSample] {
    guard let data = altitudeData else { return [] }
    let pairSize = 2 * MemoryLayout<Float32>.size
    guard data.count % pairSize == 0 else { return [] }
    var samples = [AltitudeSample]()
    samples.reserveCapacity(data.count / pairSize)
    data.withUnsafeBytes { (raw: UnsafeRawBufferPointer) in
      for offset in stride(from: 0, to: raw.count, by: pairSize) {
        let distance = raw.loadUnaligned(fromByteOffset: offset, as: Float32.self)
        let altitude = raw.loadUnaligned(fromByteOffset: offset + MemoryLayout<Float32>.size, as: Float32.self)
        samples.append(AltitudeSample(distance: Double(distance), altitude: Double(altitude)))
      }
    }
    return samples
  }

  func contextDictionary() -> [String: Any] {
    var data = Data(capacity: coordinates.count * 2 * MemoryLayout<Double>.size)
    for coordinate in coordinates {
      withUnsafeBytes(of: coordinate.latitude) { data.append(contentsOf: $0) }
      withUnsafeBytes(of: coordinate.longitude) { data.append(contentsOf: $0) }
    }
    var context: [String: Any] = [
      Self.versionKey: Self.version,
      Self.activeKey: true,
      Self.coordinatesKey: data,
      Self.routerTypeKey: routerType,
      Self.destinationKey: destinationName,
      Self.timestampKey: timestamp,
    ]
    if let altitudeData, !altitudeData.isEmpty {
      context[Self.altitudesKey] = altitudeData
      context[Self.ascentKey] = ascent
      context[Self.descentKey] = descent
    }
    return context
  }

  static func inactiveContext() -> [String: Any] {
    [versionKey: version, activeKey: false, timestampKey: Date()]
  }

  /// Returns nil for inactive, malformed or degenerate (< 2 points) payloads.
  init?(contextDictionary dictionary: [String: Any]) {
    let pairSize = 2 * MemoryLayout<Double>.size
    guard dictionary[Self.activeKey] as? Bool == true,
          let data = dictionary[Self.coordinatesKey] as? Data,
          data.count % pairSize == 0, data.count >= 2 * pairSize
    else { return nil }

    var coordinates = [CLLocationCoordinate2D]()
    coordinates.reserveCapacity(data.count / pairSize)
    data.withUnsafeBytes { (raw: UnsafeRawBufferPointer) in
      for offset in stride(from: 0, to: raw.count, by: pairSize) {
        let lat = raw.loadUnaligned(fromByteOffset: offset, as: Double.self)
        let lon = raw.loadUnaligned(fromByteOffset: offset + MemoryLayout<Double>.size, as: Double.self)
        coordinates.append(CLLocationCoordinate2D(latitude: lat, longitude: lon))
      }
    }
    self.coordinates = coordinates
    self.routerType = dictionary[Self.routerTypeKey] as? String ?? ""
    self.destinationName = dictionary[Self.destinationKey] as? String ?? ""
    self.timestamp = dictionary[Self.timestampKey] as? Date ?? Date()
    self.altitudeData = dictionary[Self.altitudesKey] as? Data
    self.ascent = dictionary[Self.ascentKey] as? Double ?? 0
    self.descent = dictionary[Self.descentKey] as? Double ?? 0
  }
}
