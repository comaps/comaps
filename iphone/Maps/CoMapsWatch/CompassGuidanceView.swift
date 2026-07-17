import SwiftUI
import CoreLocation

/// Compass-style guidance: a big arrow pointing along the route relative to
/// the direction you face, with distance, ETA, and the elevation strip below.
/// The background turns red when you are more than 100 m off the route and
/// the arrow then points back to it.
struct CompassGuidanceView: View {
  @EnvironmentObject private var routeStore: RouteStore
  @EnvironmentObject private var locationProvider: WatchLocationProvider
  @EnvironmentObject private var workout: WorkoutSessionManager

  /// How far ahead along the route the arrow aims; short enough to follow
  /// curves, long enough not to flip on GPS jitter.
  private static let lookaheadMeters = 100.0
  private static let offRouteThresholdMeters = 100.0

  var body: some View {
    if let route = routeStore.route {
      guidance(route)
    }
  }

  private func guidance(_ route: WatchRoutePayload) -> some View {
    let projection = RouteProjection(origin: route.coordinates[route.coordinates.count / 2])
    let points = route.coordinates.map(projection.point)
    let cumulative = RouteMath.cumulativeLengths(points)
    let userPoint = locationProvider.location.map { projection.point($0.coordinate) }
    let progress = userPoint.map { RouteMath.progress(points: points, cumulativeLengths: cumulative, user: $0) }
    let offRoute = (progress?.distanceFromRoute ?? 0) > Self.offRouteThresholdMeters

    return ZStack {
      (offRoute ? Color(red: 0.55, green: 0.12, blue: 0.12) : Color.black)
        .ignoresSafeArea()

      if let progress, let userPoint {
        let total = cumulative[points.count - 1]
        let traveled = total - progress.distanceRemaining
        let target = offRoute
          ? progress.snappedPoint
          : RouteMath.point(atDistance: traveled + Self.lookaheadMeters, points: points, cumulativeLengths: cumulative)
        let bearing = Self.bearingDegrees(from: userPoint, to: target)
        let arrowRotation = bearing - (locationProvider.orientationDegrees ?? 0)

        // Arrival time shares the clock band, leading-aligned like the
        // system time is trailing-aligned; the flag marks it as the ETA.
        if let eta = eta(route: route, remaining: progress.distanceRemaining) {
          VStack {
            HStack(spacing: 3) {
              Image(systemName: "flag.checkered")
                .font(.system(size: 11))
              Text(eta.arrival, style: .time)
                .font(.footnote)
              Spacer(minLength: 56)
            }
            .foregroundStyle(offRoute ? .white : .green)
            .padding(.leading, 18)
            .padding(.top, 6)
            Spacer()
          }
          .ignoresSafeArea(edges: .top)
        }

        // Live next maneuver from the phone, folded into this page; the
        // compass shrinks to make room for its distance and street name.
        let turn = routeStore.turnInfo.flatMap { $0.isStale ? nil : $0 }
        let hasStreetName = turn?.streetName != nil
        let circleSize: CGFloat = turn == nil ? 96 : (hasStreetName ? 70 : 78)
        let arrowSize: CGFloat = turn == nil ? 52 : (hasStreetName ? 38 : 42)

        VStack(spacing: 4) {
          ZStack {
            Circle()
              .fill(.white.opacity(offRoute ? 0.2 : 0.1))
              .frame(width: circleSize, height: circleSize)
            Image(systemName: "location.north.fill")
              .font(.system(size: arrowSize))
              .foregroundStyle(offRoute ? .white : .green)
              .rotationEffect(.degrees(arrowRotation))
              .animation(.easeInOut(duration: 0.25), value: arrowRotation)
          }

          if let turn {
            VStack(spacing: 1) {
              HStack(spacing: 4) {
                Image(systemName: turn.symbolName)
                Text(turn.distanceText)
              }
              .font(.headline)

              if let streetName = turn.streetName {
                Text(streetName)
                  .font(.caption2)
                  .lineLimit(1)
                  .minimumScaleFactor(0.75)
              }
            }
            .foregroundStyle(offRoute ? .white : .green)
          }

          elevationStrip(route: route, fraction: total > 0 ? traveled / total : 0, offRoute: offRoute)

          HStack(spacing: 8) {
            chip(Self.formatDistance(offRoute ? progress.distanceFromRoute : progress.distanceRemaining))
            if let eta = eta(route: route, remaining: progress.distanceRemaining) {
              chip(String(format: String(localized: "%lld min"), eta.minutes))
            }
          }

          HStack(spacing: 12) {
            // Ends the route on the phone (which also stops the workout).
            Button {
              routeStore.stopRoute()
            } label: {
              Label("End route", systemImage: "xmark.circle.fill")
                .font(.system(size: 12))
            }
            .buttonStyle(.plain)
            .foregroundStyle(offRoute ? .white : .red)

            // A running workout keeps the app alive on wrist-down:
            // continuous guidance, Always-On, and off-route wrist taps.
            Button {
              workout.toggle(routerType: route.routerType)
            } label: {
              Label(workout.isRunning ? "Stop workout" : "Record workout",
                    systemImage: workout.isRunning ? "stop.circle.fill" : "record.circle")
                .font(.system(size: 12))
            }
            .buttonStyle(.plain)
            .foregroundStyle(workout.isRunning ? .red : .secondary)
          }
        }
        .padding(.horizontal, 6)
      } else {
        Text(locationProvider.isDenied ? "Location access denied" : "Locating…")
          .font(.headline)
          .foregroundStyle(.secondary)
      }
    }
  }

  // MARK: - Pieces

  private func elevationStrip(route: WatchRoutePayload, fraction: Double, offRoute: Bool) -> some View {
    let samples = route.altitudeSamples
    return VStack(spacing: 3) {
      if samples.count > 1 {
        Canvas { context, size in
          let minAltitude = samples.map(\.altitude).min()!
          let span = max(samples.map(\.altitude).max()! - minAltitude, 10)
          let total = samples.last!.distance
          guard total > 0 else { return }
          var line = Path()
          line.move(to: CGPoint(x: 0, y: size.height * (1 - (samples[0].altitude - minAltitude) / span)))
          for sample in samples.dropFirst() {
            line.addLine(to: CGPoint(x: size.width * sample.distance / total,
                                     y: size.height * (1 - (sample.altitude - minAltitude) / span)))
          }
          context.stroke(line, with: .color(.white.opacity(0.8)), lineWidth: 1.5)
        }
        .frame(height: 22)
      }
      GeometryReader { geometry in
        let clamped = min(max(fraction, 0), 1)
        ZStack(alignment: .leading) {
          Capsule().fill(.white.opacity(0.25))
          Capsule().fill(offRoute ? Color.white : Color.green)
            .frame(width: max(geometry.size.width * clamped, 4))
          Circle()
            .fill(.white)
            .frame(width: 8, height: 8)
            .offset(x: max(geometry.size.width * clamped - 4, 0))
        }
      }
      .frame(height: 5)
    }
  }

  private func chip(_ text: String) -> some View {
    Text(text)
      .font(.footnote)
      .padding(.vertical, 2)
      .padding(.horizontal, 8)
      .background(.white.opacity(0.15), in: Capsule())
  }

  // MARK: - Math

  /// Degrees clockwise from north; projected x is east, y is north.
  private static func bearingDegrees(from: CGPoint, to: CGPoint) -> Double {
    atan2(to.x - from.x, to.y - from.y) * 180 / .pi
  }

  private func eta(route: WatchRoutePayload, remaining: Double) -> (minutes: Int, arrival: Date)? {
    let measured = locationProvider.location?.speed ?? -1
    let speed = measured > 0.3 ? measured : Self.defaultSpeed(routerType: route.routerType)
    guard speed > 0 else { return nil }
    let seconds = remaining / speed
    return (minutes: max(1, Int((seconds / 60).rounded())), arrival: Date().addingTimeInterval(seconds))
  }

  /// Fallback speeds (m/s) when GPS speed is unavailable (standing still).
  private static func defaultSpeed(routerType: String) -> Double {
    switch routerType {
    case "vehicle": return 13.9  // 50 km/h
    case "bicycle": return 4.5   // 16 km/h
    case "transit": return 8.3   // 30 km/h
    default: return 1.4          // walking
    }
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
}
