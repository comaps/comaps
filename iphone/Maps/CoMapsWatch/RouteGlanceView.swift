import SwiftUI
import CoreLocation

/// Glanceable "where am I on my route" screen: the route polyline over a
/// minimalist vector map of the surroundings (roads/water/rail extracted on
/// the phone), digital-crown zoom, and the distance still to go. When zoomed
/// in, the map rotates so the direction you are facing points up; the
/// fit-to-route overview stays north-up.
struct RouteGlanceView: View {
  @EnvironmentObject private var routeStore: RouteStore
  @EnvironmentObject private var locationProvider: WatchLocationProvider

  /// log2 of the visible span in meters; crown-controlled.
  @State private var zoomExponent = 10.0  // 2^10 = ~1 km across the screen
  @State private var fitToRoute = true
  @State private var contextCache = MapContextPathCache()

  private static let minZoomExponent = 7.0   // 128 m
  private static let maxZoomExponent = 17.0  // 131 km

  var body: some View {
    if let route = routeStore.route {
      routeCanvas(route)
    } else {
      emptyState
    }
  }

  // MARK: - Map rendering

  private func routeCanvas(_ route: WatchRoutePayload) -> some View {
    let projection = RouteProjection(origin: route.coordinates[route.coordinates.count / 2])
    let points = route.coordinates.map(projection.point)
    let cumulative = RouteMath.cumulativeLengths(points)
    let userPoint = locationProvider.location.map { projection.point($0.coordinate) }
    let progress = userPoint.map { RouteMath.progress(points: points, cumulativeLengths: cumulative, user: $0) }

    return GeometryReader { geometry in
      let size = geometry.size
      let center = userPoint ?? boundsCenter(points)
      let fittedSpan = fittingSpan(points, size: size)
      let spanMeters = fitToRoute ? fittedSpan : pow(2, zoomExponent)
      let scale = min(size.width, size.height) / spanMeters
      // Heading-up only while zoomed in; the whole-route overview would just
      // spin distractingly on every wrist movement.
      let rotationDegrees = (fitToRoute ? nil : locationProvider.orientationDegrees) ?? 0
      let transform = Self.mapTransform(center: center, scale: scale,
                                        rotationDegrees: rotationDegrees, size: size)
      // While fitted, the crown reads the fitted zoom level, so the first
      // rotation zooms from what is on screen instead of jumping to the
      // last manual zoom.
      let crownZoom = Binding<Double>(
        get: { fitToRoute ? min(max(log2(fittedSpan), Self.minZoomExponent), Self.maxZoomExponent) : zoomExponent },
        set: { newValue in
          zoomExponent = newValue
          fitToRoute = false
        })

      ZStack {
        Canvas { context, _ in
          drawMapContext(in: &context, transform: transform, spanMeters: spanMeters, projection: projection)
          drawDestinations(in: &context, transform: transform, spanMeters: spanMeters, projection: projection)
          drawRoute(in: &context, points: points, transform: transform,
                    traveledUpTo: progress?.nextPointIndex)
          if let userPoint {
            drawUser(in: &context, at: userPoint.applying(transform),
                     course: locationProvider.location?.course ?? -1,
                     mapRotationDegrees: rotationDegrees)
          }
          if rotationDegrees != 0 {
            drawNorthIndicator(in: &context, rotationDegrees: rotationDegrees)
          }
        }

        VStack {
          // Share the clock band: status leading, system time trailing.
          HStack(alignment: .top) {
            statusLine(route: route, progress: progress)
            Spacer(minLength: 56)
          }
          .padding(.top, 4)
          .padding(.leading, 8)
          Spacer()
          if !route.destinationName.isEmpty {
            HStack(spacing: 4) {
              if let icon = Self.routerTypeIcon(route.routerType) {
                Image(systemName: icon)
              }
              Text(route.destinationName)
                .lineLimit(1)
            }
            .font(.footnote)
            .foregroundStyle(.secondary)
            .padding(.vertical, 2)
            .padding(.horizontal, 8)
            .background(.black.opacity(0.55), in: Capsule())
            // Clear of the TabView page indicator and the screen curve.
            .padding(.bottom, 18)
          }
        }
        .padding(.horizontal, 4)
      }
      .focusable()
      .digitalCrownRotation(crownZoom,
                            from: Self.minZoomExponent, through: Self.maxZoomExponent,
                            by: 0.1, sensitivity: .medium,
                            isContinuous: false, isHapticFeedbackEnabled: true)
      .onTapGesture(count: 2) { fitToRoute = true }
    }
    // The map fills the whole screen, including the clock band on top.
    .ignoresSafeArea()
  }

  /// Meters (east/north around the projection origin) to screen points:
  /// translate to the view center, rotate for heading-up, flip y (north up).
  private static func mapTransform(center: CGPoint, scale: Double,
                                   rotationDegrees: Double, size: CGSize) -> CGAffineTransform {
    CGAffineTransform(translationX: -center.x, y: -center.y)
      .concatenating(CGAffineTransform(rotationAngle: rotationDegrees * .pi / 180))
      .concatenating(CGAffineTransform(scaleX: scale, y: -scale))
      .concatenating(CGAffineTransform(translationX: size.width / 2, y: size.height / 2))
  }

  private func drawMapContext(in context: inout GraphicsContext, transform: CGAffineTransform,
                              spanMeters: Double, projection: RouteProjection) {
    contextCache.update(mapContext: routeStore.mapContext, projection: projection)
    let layers = contextCache.layers
    guard !layers.isEmpty else { return }

    if let water = layers[.waterArea] {
      context.fill(water.applying(transform), with: .color(.blue.opacity(0.25)))
    }
    if let waterLine = layers[.waterLine] {
      context.stroke(waterLine.applying(transform), with: .color(.blue.opacity(0.55)),
                     style: StrokeStyle(lineWidth: 1.5, lineCap: .round))
    }
    if spanMeters < 25_000, let rail = layers[.rail] {
      context.stroke(rail.applying(transform), with: .color(.white.opacity(0.25)),
                     style: StrokeStyle(lineWidth: 1, dash: [4, 3]))
    }
    if let major = layers[.majorRoad] {
      context.stroke(major.applying(transform), with: .color(.gray.opacity(0.8)),
                     style: StrokeStyle(lineWidth: 2, lineCap: .round, lineJoin: .round))
    }
    if spanMeters < 8_000, let minor = layers[.minorRoad] {
      context.stroke(minor.applying(transform), with: .color(.gray.opacity(0.5)),
                     style: StrokeStyle(lineWidth: 1.2, lineCap: .round, lineJoin: .round))
    }
    if spanMeters < 4_000, let path = layers[.path] {
      context.stroke(path.applying(transform), with: .color(.gray.opacity(0.45)),
                     style: StrokeStyle(lineWidth: 1, dash: [2, 2]))
    }
  }

  /// Saved places as small dots, so bookmarks give context like on the phone.
  private func drawDestinations(in context: inout GraphicsContext, transform: CGAffineTransform,
                                spanMeters: Double, projection: RouteProjection) {
    guard spanMeters < 20_000 else { return }
    for destination in routeStore.destinations {
      let point = projection.point(destination.coordinate).applying(transform)
      let rect = CGRect(x: point.x - 3.5, y: point.y - 3.5, width: 7, height: 7)
      context.fill(Path(ellipseIn: rect), with: .color(.yellow))
      context.stroke(Path(ellipseIn: rect), with: .color(.black.opacity(0.6)), lineWidth: 1)
    }
  }

  private func drawRoute(in context: inout GraphicsContext, points: [CGPoint],
                         transform: CGAffineTransform, traveledUpTo: Int?) {
    guard points.count >= 2 else { return }
    let splitIndex = traveledUpTo ?? 1

    if splitIndex > 1 {
      var traveled = Path()
      traveled.move(to: points[0])
      for point in points[1..<splitIndex] { traveled.addLine(to: point) }
      context.stroke(traveled.applying(transform), with: .color(.gray.opacity(0.6)),
                     style: StrokeStyle(lineWidth: 4, lineCap: .round, lineJoin: .round))
    }

    var remaining = Path()
    remaining.move(to: points[max(0, splitIndex - 1)])
    for point in points[splitIndex...] { remaining.addLine(to: point) }
    context.stroke(remaining.applying(transform), with: .color(.blue),
                   style: StrokeStyle(lineWidth: 4, lineCap: .round, lineJoin: .round))

    let start = points[0].applying(transform)
    context.fill(Path(ellipseIn: CGRect(x: start.x - 4, y: start.y - 4, width: 8, height: 8)),
                 with: .color(.green))
    if let finish = points.last?.applying(transform) {
      context.fill(Path(ellipseIn: CGRect(x: finish.x - 4, y: finish.y - 4, width: 8, height: 8)),
                   with: .color(.red))
    }
  }

  private func drawUser(in context: inout GraphicsContext, at point: CGPoint, course: Double,
                        mapRotationDegrees: Double) {
    if course >= 0 {
      // Wedge pointing up (north) in local space, tip 14 pt from the center,
      // rotated to the course relative to the map orientation.
      var wedge = Path()
      wedge.move(to: CGPoint(x: 0, y: -14))
      wedge.addLine(to: CGPoint(x: -6, y: -4))
      wedge.addLine(to: CGPoint(x: 6, y: -4))
      wedge.closeSubpath()
      let transform = CGAffineTransform(rotationAngle: (course - mapRotationDegrees) * .pi / 180)
        .concatenating(CGAffineTransform(translationX: point.x, y: point.y))
      context.fill(wedge.applying(transform), with: .color(.white.opacity(0.9)))
    }
    context.fill(Path(ellipseIn: CGRect(x: point.x - 7, y: point.y - 7, width: 14, height: 14)),
                 with: .color(.white))
    context.fill(Path(ellipseIn: CGRect(x: point.x - 5, y: point.y - 5, width: 10, height: 10)),
                 with: .color(.blue))
  }

  private func drawNorthIndicator(in context: inout GraphicsContext, rotationDegrees: Double) {
    // Below the status capsule, clear of the clock band and screen corner.
    let center = CGPoint(x: 16, y: 48)
    let angle = rotationDegrees * .pi / 180
    // Where north points on screen for the current map rotation.
    let direction = CGPoint(x: -sin(angle), y: -cos(angle))
    context.fill(Path(ellipseIn: CGRect(x: center.x - 9, y: center.y - 9, width: 18, height: 18)),
                 with: .color(.black.opacity(0.5)))
    var needle = Path()
    needle.move(to: center)
    needle.addLine(to: CGPoint(x: center.x + direction.x * 7, y: center.y + direction.y * 7))
    context.stroke(needle, with: .color(.red), style: StrokeStyle(lineWidth: 2, lineCap: .round))
    var tail = Path()
    tail.move(to: center)
    tail.addLine(to: CGPoint(x: center.x - direction.x * 7, y: center.y - direction.y * 7))
    context.stroke(tail, with: .color(.white.opacity(0.6)), style: StrokeStyle(lineWidth: 2, lineCap: .round))
  }

  private func statusLine(route: WatchRoutePayload, progress: RouteProgress?) -> some View {
    Group {
      if let progress {
        if progress.distanceFromRoute > 100 {
          Text("\(Self.formatDistance(progress.distanceFromRoute)) to route")
            .foregroundStyle(.orange)
        } else {
          Text("\(Self.formatDistance(progress.distanceRemaining)) left")
            .foregroundStyle(.primary)
        }
      } else {
        Text(locationProvider.isDenied ? "Location access denied" : "Locating…")
          .foregroundStyle(.secondary)
      }
    }
    .font(.headline)
    .padding(.vertical, 2)
    .padding(.horizontal, 8)
    .background(.black.opacity(0.55), in: Capsule())
  }

  // MARK: - Empty state

  /// RootView shows DestinationsView when there is no route; this only
  /// covers the brief moment while a route is being cleared.
  private var emptyState: some View {
    Image(systemName: "map")
      .font(.title2)
      .foregroundStyle(.secondary)
  }

  // MARK: - Helpers

  private func boundsCenter(_ points: [CGPoint]) -> CGPoint {
    var minX = points[0].x, maxX = points[0].x
    var minY = points[0].y, maxY = points[0].y
    for p in points {
      minX = min(minX, p.x); maxX = max(maxX, p.x)
      minY = min(minY, p.y); maxY = max(maxY, p.y)
    }
    return CGPoint(x: (minX + maxX) / 2, y: (minY + maxY) / 2)
  }

  private func fittingSpan(_ points: [CGPoint], size: CGSize) -> Double {
    var minX = points[0].x, maxX = points[0].x
    var minY = points[0].y, maxY = points[0].y
    for p in points {
      minX = min(minX, p.x); maxX = max(maxX, p.x)
      minY = min(minY, p.y); maxY = max(maxY, p.y)
    }
    // scale = min(w,h) / span, so span must cover both extents after scaling.
    guard size.width > 0, size.height > 0 else { return 1000 }
    let shortSide = min(size.width, size.height)
    let span = max((maxX - minX) * shortSide / size.width,
                   (maxY - minY) * shortSide / size.height)
    return max(200, span * 1.2)  // 20% margin, at least 200 m across
  }

  private static let distanceFormatter: MeasurementFormatter = {
    let formatter = MeasurementFormatter()
    formatter.unitOptions = .naturalScale
    formatter.numberFormatter.maximumFractionDigits = 1
    return formatter
  }()

  private static func formatDistance(_ meters: Double) -> String {
    distanceFormatter.string(from: Measurement(value: meters, unit: UnitLength.meters))
  }

  private static func routerTypeIcon(_ routerType: String) -> String? {
    switch routerType {
    case "vehicle": return "car.fill"
    case "pedestrian": return "figure.walk"
    case "bicycle": return "bicycle"
    case "transit": return "tram.fill"
    default: return nil
    }
  }
}

/// Builds the corridor geometry into meter-space paths once per (context,
/// projection origin) pair; per-frame zoom/rotation happens in the affine
/// transform, so redraws never re-project thousands of points.
private final class MapContextPathCache {
  private var cachedTimestamp: Date?
  private var cachedOrigin: CLLocationCoordinate2D?
  private(set) var layers: [WatchMapFeature.Kind: Path] = [:]

  func update(mapContext: WatchMapContext?, projection: RouteProjection) {
    guard let mapContext else {
      if !layers.isEmpty {
        layers = [:]
        cachedTimestamp = nil
      }
      return
    }
    if cachedTimestamp == mapContext.timestamp,
       cachedOrigin?.latitude == projection.origin.latitude,
       cachedOrigin?.longitude == projection.origin.longitude {
      return
    }
    cachedTimestamp = mapContext.timestamp
    cachedOrigin = projection.origin

    var built: [WatchMapFeature.Kind: Path] = [:]
    for feature in mapContext.features {
      let points = feature.coordinates.map(projection.point)
      var path = built[feature.kind] ?? Path()
      if feature.kind == .waterArea {
        var i = 0
        while i + 2 < points.count {
          path.move(to: points[i])
          path.addLine(to: points[i + 1])
          path.addLine(to: points[i + 2])
          path.closeSubpath()
          i += 3
        }
      } else {
        path.move(to: points[0])
        for point in points.dropFirst() { path.addLine(to: point) }
      }
      built[feature.kind] = path
    }
    layers = built
  }
}
