import SwiftUI

struct ElevationSelectionInfo: View {
  let point: ElevationHeightPoint

  var body: some View {
    Text(verbatim: "\(DistanceFormatter.distanceString(fromMeters: point.distance)) · ▲ \(AltitudeFormatter.altitudeString(fromMeters: point.altitude))")
      .font(.footnote)
      .monospacedDigit()
      .foregroundStyle(.secondary)
  }
}
