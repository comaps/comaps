import CoreGraphics
import CoreLocation

/// Local equirectangular projection around a fixed origin. At route scale the
/// distortion is negligible and it is cheap enough to run every frame.
struct RouteProjection {
  private static let metersPerDegreeLat = 111_320.0

  let origin: CLLocationCoordinate2D
  private let metersPerDegreeLon: Double

  init(origin: CLLocationCoordinate2D) {
    self.origin = origin
    metersPerDegreeLon = Self.metersPerDegreeLat * cos(origin.latitude * .pi / 180)
  }

  /// Meters east (x) / north (y) of the origin.
  func point(_ coordinate: CLLocationCoordinate2D) -> CGPoint {
    CGPoint(x: (coordinate.longitude - origin.longitude) * metersPerDegreeLon,
            y: (coordinate.latitude - origin.latitude) * Self.metersPerDegreeLat)
  }
}

struct RouteProgress {
  /// Distance left along the route from the snapped position, in meters.
  let distanceRemaining: Double
  /// Crow-flight distance from the user to the nearest point of the route.
  let distanceFromRoute: Double
  /// The user position snapped onto the route, in projected meters.
  let snappedPoint: CGPoint
  /// Index of the polyline point ending the segment the user is snapped to;
  /// points before it count as traveled.
  let nextPointIndex: Int
}

enum RouteMath {
  static func cumulativeLengths(_ points: [CGPoint]) -> [Double] {
    var lengths = [Double](repeating: 0, count: points.count)
    for i in 1..<points.count {
      let dx = points[i].x - points[i - 1].x
      let dy = points[i].y - points[i - 1].y
      lengths[i] = lengths[i - 1] + (dx * dx + dy * dy).squareRoot()
    }
    return lengths
  }

  /// Point on the polyline at the given distance from the start (clamped).
  static func point(atDistance distance: Double, points: [CGPoint], cumulativeLengths: [Double]) -> CGPoint {
    guard let total = cumulativeLengths.last, total > 0 else { return points[0] }
    let target = min(max(0, distance), total)
    for i in 1..<points.count where cumulativeLengths[i] >= target {
      let segment = cumulativeLengths[i] - cumulativeLengths[i - 1]
      guard segment > 0 else { return points[i] }
      let t = (target - cumulativeLengths[i - 1]) / segment
      return CGPoint(x: points[i - 1].x + t * (points[i].x - points[i - 1].x),
                     y: points[i - 1].y + t * (points[i].y - points[i - 1].y))
    }
    return points[points.count - 1]
  }

  static func progress(points: [CGPoint], cumulativeLengths: [Double], user: CGPoint) -> RouteProgress {
    var bestDistanceSquared = Double.greatestFiniteMagnitude
    var bestPoint = points[0]
    var bestIndex = 1
    var bestTraveled = 0.0

    for i in 1..<points.count {
      let a = points[i - 1]
      let b = points[i]
      let dx = b.x - a.x
      let dy = b.y - a.y
      let lengthSquared = dx * dx + dy * dy
      var t = 0.0
      if lengthSquared > 0 {
        t = min(1, max(0, ((user.x - a.x) * dx + (user.y - a.y) * dy) / lengthSquared))
      }
      let candidate = CGPoint(x: a.x + t * dx, y: a.y + t * dy)
      let cdx = user.x - candidate.x
      let cdy = user.y - candidate.y
      let distanceSquared = cdx * cdx + cdy * cdy
      if distanceSquared < bestDistanceSquared {
        bestDistanceSquared = distanceSquared
        bestPoint = candidate
        bestIndex = i
        bestTraveled = cumulativeLengths[i - 1] + t * lengthSquared.squareRoot()
      }
    }

    return RouteProgress(distanceRemaining: max(0, cumulativeLengths[points.count - 1] - bestTraveled),
                         distanceFromRoute: bestDistanceSquared.squareRoot(),
                         snappedPoint: bestPoint,
                         nextPointIndex: bestIndex)
  }
}
