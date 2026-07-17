import SwiftUI
import CoreLocation

/// Second page of the watch app: the route's elevation profile with total
/// ascent/descent and a marker at the user's position along the route.
struct ElevationProfileView: View {
  @EnvironmentObject private var routeStore: RouteStore
  @EnvironmentObject private var locationProvider: WatchLocationProvider

  /// Crown-scrubbed distance along the route; negative = follow the live
  /// position.
  @State private var scrubDistance = -1.0
  /// Raw crown value; only its deltas matter — they get a speed-dependent
  /// gain so slow turns scrub meter-by-meter and sustained spins sweep the
  /// whole route.
  @State private var crownAccumulator = 0.0
  @State private var lastCrownEvent: Date?
  @State private var crownSpeed = 0.0

  /// One slow crown revolution moves the cursor about 3% of the route…
  private static let slowGain = 0.03
  /// …and a fast, sustained spin about 1.5 routes per revolution.
  private static let fastGain = 1.5
  /// Crown speeds (revolutions/s) between which the gain ramps up; the ramp
  /// starts late and eases in cubically so deliberate turns stay precise.
  private static let slowSpeed = 0.4
  private static let fastSpeed = 2.5

  var body: some View {
    if let route = routeStore.route {
      let samples = route.altitudeSamples
      if samples.count > 1 {
        chart(route: route, samples: samples)
      }
    }
  }

  private func chart(route: WatchRoutePayload, samples: [AltitudeSample]) -> some View {
    let total = samples.last!.distance
    let traveled = traveledDistance(route: route)
    let isScrubbing = scrubDistance >= 0
    let markerDistance = isScrubbing ? scrubDistance : traveled
    // The crown scrubs an inspection cursor along the profile; double-tap
    // snaps back to the live position.
    let crown = Binding<Double>(
      get: { crownAccumulator },
      set: { applyCrown($0, traveled: traveled, total: total) })

    return VStack(spacing: 2) {
      // The header shares the clock band, leading-aligned: ascent/descent,
      // or the cursor readout while scrubbing.
      HStack(spacing: 10) {
        if isScrubbing {
          Text("\(Self.formatDistance(scrubDistance)) · \(Self.formatAltitude(Self.altitude(at: scrubDistance, samples: samples)))")
            .foregroundStyle(.yellow)
            .lineLimit(1)
        } else {
          Label(Self.formatAltitude(route.ascent), systemImage: "arrow.up.right")
          Label(Self.formatAltitude(route.descent), systemImage: "arrow.down.right")
        }
        Spacer(minLength: 48)
      }
      .font(.footnote)
      .labelStyle(.titleAndIcon)
      .padding(.top, 6)
      .padding(.leading, 14)

      Canvas { context, size in
        drawProfile(in: &context, size: size, samples: samples,
                    traveled: markerDistance, isScrubbing: isScrubbing)
      }

      HStack {
        Text(Self.formatAltitude(samples.map(\.altitude).min() ?? 0))
        Spacer()
        Text(Self.formatAltitude(samples.map(\.altitude).max() ?? 0))
      }
      .font(.system(size: 11))
      .foregroundStyle(.secondary)
    }
    .padding(.horizontal, 4)
    .ignoresSafeArea(edges: .top)
    .focusable()
    .digitalCrownRotation(crown)
    .onTapGesture(count: 2) { scrubDistance = -1 }
  }

  /// Applies a crown delta with acceleration: the gain eases in with the
  /// smoothed rotation speed, so precision stays high until you are clearly
  /// spinning through the route.
  private func applyCrown(_ newValue: Double, traveled: Double?, total: Double) {
    let delta = newValue - crownAccumulator
    crownAccumulator = newValue
    guard delta != 0, total > 0 else { return }

    let now = Date()
    if let last = lastCrownEvent {
      let dt = now.timeIntervalSince(last)
      if dt > 0.5 {
        crownSpeed = 0  // a pause resets to fine-grained scrubbing
      } else {
        crownSpeed = crownSpeed * 0.7 + (abs(delta) / max(dt, 1.0 / 120)) * 0.3
      }
    }
    lastCrownEvent = now

    let ramp = min(max((crownSpeed - Self.slowSpeed) / (Self.fastSpeed - Self.slowSpeed), 0), 1)
    let gain = Self.slowGain + (Self.fastGain - Self.slowGain) * ramp * ramp * ramp
    let base = scrubDistance >= 0 ? scrubDistance : (traveled ?? 0)
    scrubDistance = min(max(base + delta * gain * total, 0), total)
  }

  private func drawProfile(in context: inout GraphicsContext, size: CGSize,
                           samples: [AltitudeSample], traveled: Double?, isScrubbing: Bool) {
    let totalDistance = samples.last!.distance
    guard totalDistance > 0 else { return }
    let minAltitude = samples.map(\.altitude).min()!
    let maxAltitude = samples.map(\.altitude).max()!
    // Flat routes still get a visible line in the middle of the chart.
    let altitudeSpan = max(maxAltitude - minAltitude, 10)
    let inset: CGFloat = 2
    let plotHeight = size.height - 2 * inset

    func point(_ sample: AltitudeSample) -> CGPoint {
      CGPoint(x: inset + (size.width - 2 * inset) * sample.distance / totalDistance,
              y: inset + plotHeight * (1 - (sample.altitude - minAltitude) / altitudeSpan))
    }

    var line = Path()
    line.move(to: point(samples[0]))
    for sample in samples.dropFirst() { line.addLine(to: point(sample)) }

    var fill = line
    fill.addLine(to: CGPoint(x: point(samples[samples.count - 1]).x, y: size.height))
    fill.addLine(to: CGPoint(x: point(samples[0]).x, y: size.height))
    fill.closeSubpath()

    context.fill(fill, with: .linearGradient(
      Gradient(colors: [.green.opacity(0.45), .green.opacity(0.05)]),
      startPoint: CGPoint(x: 0, y: 0), endPoint: CGPoint(x: 0, y: size.height)))
    context.stroke(line, with: .color(.green),
                   style: StrokeStyle(lineWidth: 2, lineCap: .round, lineJoin: .round))

    if let traveled {
      let sample = AltitudeSample(distance: min(traveled, totalDistance),
                                  altitude: Self.altitude(at: traveled, samples: samples))
      let position = point(sample)
      let markerColor: Color = isScrubbing ? .yellow : .white
      var marker = Path()
      marker.move(to: CGPoint(x: position.x, y: inset))
      marker.addLine(to: CGPoint(x: position.x, y: size.height - inset))
      context.stroke(marker, with: .color(markerColor.opacity(0.4)), style: StrokeStyle(lineWidth: 1, dash: [3, 3]))
      context.fill(Path(ellipseIn: CGRect(x: position.x - 4, y: position.y - 4, width: 8, height: 8)),
                   with: .color(markerColor))
      context.fill(Path(ellipseIn: CGRect(x: position.x - 2.5, y: position.y - 2.5, width: 5, height: 5)),
                   with: .color(isScrubbing ? .black : .blue))
    }
  }

  /// Distance already covered along the route, or nil without a location fix.
  private func traveledDistance(route: WatchRoutePayload) -> Double? {
    guard let location = locationProvider.location else { return nil }
    let projection = RouteProjection(origin: route.coordinates[route.coordinates.count / 2])
    let points = route.coordinates.map(projection.point)
    let cumulative = RouteMath.cumulativeLengths(points)
    let progress = RouteMath.progress(points: points, cumulativeLengths: cumulative,
                                      user: projection.point(location.coordinate))
    return cumulative[points.count - 1] - progress.distanceRemaining
  }

  private static func altitude(at distance: Double, samples: [AltitudeSample]) -> Double {
    guard let last = samples.last else { return 0 }
    if distance <= samples[0].distance { return samples[0].altitude }
    if distance >= last.distance { return last.altitude }
    for i in 1..<samples.count where samples[i].distance >= distance {
      let a = samples[i - 1]
      let b = samples[i]
      let span = b.distance - a.distance
      guard span > 0 else { return a.altitude }
      let t = (distance - a.distance) / span
      return a.altitude + t * (b.altitude - a.altitude)
    }
    return last.altitude
  }

  private static func formatAltitude(_ meters: Double) -> String {
    let formatter = MeasurementFormatter()
    formatter.unitOptions = .providedUnit
    formatter.numberFormatter.maximumFractionDigits = 0
    return formatter.string(from: Measurement(value: meters, unit: UnitLength.meters))
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
