import CoreLocation

/// Simplified vector geometry around the route ("corridor"), extracted on the
/// phone by MWMWatchMapExtractor and transferred as a WCSession file.
///
/// Binary layout (little-endian):
///   "CMWC" (4 bytes), UInt16 version, UInt16 pad, UInt32 featureCount,
///   Float64 timestamp, then per feature:
///   UInt8 kind, UInt8 pad, UInt16 pointCount, pointCount × (Float32 lat, Float32 lon).
struct WatchMapFeature {
  enum Kind: UInt8 {
    case majorRoad = 0
    case minorRoad = 1
    case path = 2
    case rail = 3
    case waterLine = 4
    /// Consecutive coordinate triples form filled triangles.
    case waterArea = 5
  }

  let kind: Kind
  let coordinates: [CLLocationCoordinate2D]
}

struct WatchMapContext {
  static let formatVersion: UInt16 = 1

  let timestamp: Date
  let features: [WatchMapFeature]

  init?(data: Data) {
    var offset = 0
    func read<T>(_ type: T.Type) -> T? {
      guard offset + MemoryLayout<T>.size <= data.count else { return nil }
      defer { offset += MemoryLayout<T>.size }
      return data.withUnsafeBytes { $0.loadUnaligned(fromByteOffset: offset, as: T.self) }
    }

    guard data.count >= 20,
          data.prefix(4).elementsEqual("CMWC".utf8)
    else { return nil }
    offset = 4
    guard let version = read(UInt16.self), version == Self.formatVersion,
          read(UInt16.self) != nil,
          let featureCount = read(UInt32.self),
          let timestampSeconds = read(Float64.self)
    else { return nil }
    self.timestamp = Date(timeIntervalSince1970: timestampSeconds)

    var features = [WatchMapFeature]()
    features.reserveCapacity(Int(featureCount))
    for _ in 0..<featureCount {
      guard let rawKind = read(UInt8.self),
            read(UInt8.self) != nil,
            let pointCount = read(UInt16.self),
            offset + Int(pointCount) * 8 <= data.count
      else { return nil }
      var coordinates = [CLLocationCoordinate2D]()
      coordinates.reserveCapacity(Int(pointCount))
      data.withUnsafeBytes { (raw: UnsafeRawBufferPointer) in
        for i in 0..<Int(pointCount) {
          let base = offset + i * 8
          let lat = raw.loadUnaligned(fromByteOffset: base, as: Float32.self)
          let lon = raw.loadUnaligned(fromByteOffset: base + 4, as: Float32.self)
          coordinates.append(CLLocationCoordinate2D(latitude: Double(lat), longitude: Double(lon)))
        }
      }
      offset += Int(pointCount) * 8
      // Unknown kinds are skipped so the format can grow.
      if let kind = WatchMapFeature.Kind(rawValue: rawKind), coordinates.count >= 2 {
        features.append(WatchMapFeature(kind: kind, coordinates: coordinates))
      }
    }
    self.features = features
  }
}
