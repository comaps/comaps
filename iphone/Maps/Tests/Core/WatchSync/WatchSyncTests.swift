import CoreLocation
import XCTest
@testable import CoMaps__Debug_

/// Tests for the phone-watch sync layer: the polyline simplification feeding
/// the application context, the payload and corridor-blob serialization
/// boundaries, and the route math the watch navigates by.
final class WatchSyncTests: XCTestCase {
  // MARK: - Polyline simplification

  func testSimplifyCollapsesCollinearPointsAndKeepsEndpoints() {
    let line = (0...100).map { CLLocationCoordinate2D(latitude: 0, longitude: Double($0) * 0.001) }
    let simplified = WatchRouteSyncManager.simplify(line, toleranceMeters: 5, maxPoints: 2000)
    XCTAssertEqual(simplified.count, 2)
    XCTAssertEqual(simplified.first?.longitude, 0)
    XCTAssertEqual(simplified.last?.longitude, 0.1)
  }

  func testSimplifyRespectsPointCap() {
    // A zigzag with ~111 m amplitude never collapses at 5 m tolerance, so the
    // cap must be enforced by the escalating-tolerance loop instead.
    let zigzag = (0..<1000).map {
      CLLocationCoordinate2D(latitude: $0 % 2 == 0 ? 0 : 0.001, longitude: Double($0) * 0.001)
    }
    let simplified = WatchRouteSyncManager.simplify(zigzag, toleranceMeters: 5, maxPoints: 100)
    XCTAssertLessThanOrEqual(simplified.count, 100)
    XCTAssertGreaterThanOrEqual(simplified.count, 2)
    XCTAssertEqual(simplified.first?.longitude, zigzag.first?.longitude)
    XCTAssertEqual(simplified.last?.longitude, zigzag.last?.longitude)
  }

  // MARK: - Route payload (application context)

  func testRoutePayloadSurvivesRoundTrip() {
    let coordinates = [CLLocationCoordinate2D(latitude: -41.63894, longitude: 145.94311),
                       CLLocationCoordinate2D(latitude: -41.64997, longitude: 145.94596),
                       CLLocationCoordinate2D(latitude: -41.66120, longitude: 145.95330)]
    var altitudes = Data()
    for (distance, altitude) in [(Float32(0), Float32(912)), (Float32(2500), Float32(1250))] {
      append(distance, to: &altitudes)
      append(altitude, to: &altitudes)
    }
    let payload = WatchRoutePayload(coordinates: coordinates, routerType: "pedestrian",
                                    destinationName: "Kitchen Hut",
                                    altitudeData: altitudes, ascent: 417, descent: 95)

    let decoded = WatchRoutePayload(contextDictionary: payload.contextDictionary())

    XCTAssertNotNil(decoded)
    XCTAssertEqual(decoded?.coordinates.count, coordinates.count)
    for (a, b) in zip(decoded?.coordinates ?? [], coordinates) {
      XCTAssertEqual(a.latitude, b.latitude)  // Float64 must survive bit-exact.
      XCTAssertEqual(a.longitude, b.longitude)
    }
    XCTAssertEqual(decoded?.routerType, "pedestrian")
    XCTAssertEqual(decoded?.destinationName, "Kitchen Hut")
    XCTAssertEqual(decoded?.ascent, 417)
    XCTAssertEqual(decoded?.descent, 95)
    let samples = decoded?.altitudeSamples ?? []
    XCTAssertEqual(samples.count, 2)
    XCTAssertEqual(samples.last?.distance ?? 0, 2500, accuracy: 0.001)
    XCTAssertEqual(samples.last?.altitude ?? 0, 1250, accuracy: 0.001)
  }

  func testRoutePayloadRejectsInactiveAndMalformedContexts() {
    XCTAssertNil(WatchRoutePayload(contextDictionary: WatchRoutePayload.inactiveContext()))
    XCTAssertNil(WatchRoutePayload(contextDictionary: [:]))

    let valid = WatchRoutePayload(coordinates: [CLLocationCoordinate2D(latitude: 1, longitude: 2),
                                                CLLocationCoordinate2D(latitude: 3, longitude: 4)],
                                  routerType: "vehicle", destinationName: "").contextDictionary()

    var truncated = valid
    truncated[WatchRoutePayload.coordinatesKey] = (valid[WatchRoutePayload.coordinatesKey] as! Data).dropLast(8)
    XCTAssertNil(WatchRoutePayload(contextDictionary: truncated))

    var singlePoint = valid
    singlePoint[WatchRoutePayload.coordinatesKey] = (valid[WatchRoutePayload.coordinatesKey] as! Data).prefix(16)
    XCTAssertNil(WatchRoutePayload(contextDictionary: singlePoint))

    var inactive = valid
    inactive[WatchRoutePayload.activeKey] = false
    XCTAssertNil(WatchRoutePayload(contextDictionary: inactive))
  }

  // MARK: - Corridor blob (CMWC)

  func testMapContextParsesFeaturesAndSkipsUnknownKinds() {
    let blob = corridorBlob(features: [
      (kind: 0, points: [(-41.64, 145.94), (-41.65, 145.95), (-41.66, 145.96)]),
      (kind: 9, points: [(0, 0), (1, 1)]),  // unknown kind: skip, keep parsing
      (kind: 5, points: [(-41.64, 145.94), (-41.64, 145.95), (-41.65, 145.94)]),
    ])

    let context = WatchMapContext(data: blob)

    XCTAssertNotNil(context)
    XCTAssertEqual(context?.features.count, 2)
    XCTAssertEqual(context?.features.first?.kind, .majorRoad)
    XCTAssertEqual(context?.features.last?.kind, .waterArea)
    XCTAssertEqual(context?.features.first?.coordinates.count, 3)
    XCTAssertEqual(context?.features.first?.coordinates.first?.latitude ?? 0, -41.64, accuracy: 0.0001)
    XCTAssertEqual(context?.timestamp.timeIntervalSince1970 ?? 0, 1_700_000_000, accuracy: 1)
  }

  func testMapContextRejectsCorruptBlobs() {
    XCTAssertNil(WatchMapContext(data: Data()))
    XCTAssertNil(WatchMapContext(data: corridorBlob(magic: "XXXX", features: [])))
    XCTAssertNil(WatchMapContext(data: corridorBlob(version: 99, features: [])))
    // Point count promising more data than the blob carries must not crash.
    let truncated = corridorBlob(features: [(kind: 0, points: [(1, 2), (3, 4)])]).dropLast(4)
    XCTAssertNil(WatchMapContext(data: Data(truncated)))
  }

  // MARK: - Route math

  func testProgressSnapsToRouteAndMeasuresRemaining() {
    // An L-shaped route in meter space: east 100 m, then north 100 m.
    let points = [CGPoint(x: 0, y: 0), CGPoint(x: 100, y: 0), CGPoint(x: 100, y: 100)]
    let cumulative = RouteMath.cumulativeLengths(points)
    XCTAssertEqual(cumulative, [0, 100, 200])

    let progress = RouteMath.progress(points: points, cumulativeLengths: cumulative,
                                      user: CGPoint(x: 50, y: 10))
    XCTAssertEqual(progress.snappedPoint.x, 50, accuracy: 1e-9)
    XCTAssertEqual(progress.snappedPoint.y, 0, accuracy: 1e-9)
    XCTAssertEqual(progress.distanceFromRoute, 10, accuracy: 1e-9)
    XCTAssertEqual(progress.distanceRemaining, 150, accuracy: 1e-9)
    XCTAssertEqual(progress.nextPointIndex, 1)

    let arrived = RouteMath.progress(points: points, cumulativeLengths: cumulative,
                                     user: CGPoint(x: 100, y: 250))
    XCTAssertEqual(arrived.distanceRemaining, 0)
  }

  func testPointAtDistanceInterpolatesAndClamps() {
    let points = [CGPoint(x: 0, y: 0), CGPoint(x: 100, y: 0), CGPoint(x: 100, y: 100)]
    let cumulative = RouteMath.cumulativeLengths(points)

    let midway = RouteMath.point(atDistance: 150, points: points, cumulativeLengths: cumulative)
    XCTAssertEqual(midway.x, 100, accuracy: 1e-9)
    XCTAssertEqual(midway.y, 50, accuracy: 1e-9)

    XCTAssertEqual(RouteMath.point(atDistance: -5, points: points, cumulativeLengths: cumulative),
                   CGPoint(x: 0, y: 0))
    XCTAssertEqual(RouteMath.point(atDistance: 999, points: points, cumulativeLengths: cumulative),
                   CGPoint(x: 100, y: 100))
  }

  // MARK: - Destinations

  func testDestinationRoundTripAndHomeDetection() {
    let home = WatchDestination(dictionary: ["n": "home", "la": -41.6, "lo": 145.9])
    XCTAssertNotNil(home)
    XCTAssertTrue(home?.isHome ?? false)

    let pin = WatchDestination(dictionary: home?.dictionary ?? [:])
    XCTAssertEqual(pin?.name, "home")
    XCTAssertEqual(pin?.coordinate.latitude, -41.6)
    XCTAssertNil(WatchDestination(dictionary: ["n": "missing coordinates"]))
  }

  // MARK: - Helpers

  private func append<T: FixedWidthInteger>(_ value: T, to data: inout Data) {
    withUnsafeBytes(of: value.littleEndian) { data.append(contentsOf: $0) }
  }

  private func append(_ value: Float32, to data: inout Data) {
    append(value.bitPattern, to: &data)
  }

  private func append(_ value: Float64, to data: inout Data) {
    append(value.bitPattern, to: &data)
  }

  /// Builds a corridor blob the same way MWMWatchMapExtractor packs it.
  private func corridorBlob(version: UInt16 = 1, magic: String = "CMWC",
                            features: [(kind: UInt8, points: [(Float32, Float32)])]) -> Data {
    var data = Data(magic.utf8)
    append(version, to: &data)
    append(UInt16(0), to: &data)
    append(UInt32(features.count), to: &data)
    append(Float64(1_700_000_000), to: &data)
    for feature in features {
      append(feature.kind, to: &data)
      append(UInt8(0), to: &data)
      append(UInt16(feature.points.count), to: &data)
      for (latitude, longitude) in feature.points {
        append(latitude, to: &data)
        append(longitude, to: &data)
      }
    }
    return data
  }
}
