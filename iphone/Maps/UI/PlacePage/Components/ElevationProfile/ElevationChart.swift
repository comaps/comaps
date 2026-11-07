import Charts
import SwiftUI

struct ElevationChart: View {
  let profile: ElevationProfile
  let activePointDistance: Double?
  let myPositionDistance: Double?
  let onSelect: @MainActor (CLLocationCoordinate2D, Double) -> Void

  @State private var rawSelection: Double?

  private let lineColor = Color.BaseColors.blue
  private let distanceLabel = L("placepage_distance")

  var body: some View {
    VStack(alignment: .leading, spacing: 4) {
      if let selected = selectedPoint {
        ElevationSelectionInfo(point: selected)
      }
      chart
        .frame(height: 176)
    }
  }

  private var chart: some View {
    Chart {
      AreaPlot(profile.samples,
               x: .value(distanceLabel, \.distance),
               yStart: .value("", \.base),
               yEnd: .value("", \.altitude))
        .foregroundStyle(.linearGradient(colors: [lineColor.opacity(0.3), lineColor.opacity(0.05)],
                                         startPoint: .top, endPoint: .bottom))
      LinePlot(profile.samples,
               x: .value(distanceLabel, \.distance),
               y: .value("", \.altitude))
        .foregroundStyle(lineColor)
        .lineStyle(StrokeStyle(lineWidth: 2.5, lineCap: .round, lineJoin: .round))
      if let myPosition = myPositionPoint {
        PointMark(x: .value(distanceLabel, myPosition.distance),
                  y: .value("", myPosition.altitude))
          .symbol {
            Circle()
              .fill(lineColor)
              .stroke(.white, lineWidth: 2)
              .frame(width: 12, height: 12)
          }
      }
      if let selected = selectedPoint {
        RuleMark(x: .value(distanceLabel, selected.distance))
          .foregroundStyle(lineColor.opacity(0.5))
          .lineStyle(StrokeStyle(lineWidth: 1))
        PointMark(x: .value(distanceLabel, selected.distance),
                  y: .value("", selected.altitude))
          .symbolSize(80)
          .foregroundStyle(lineColor)
      }
    }
    .chartXScale(domain: 0...max(profile.maxDistance, 1))
    .chartYScale(domain: profile.altitudeDomain)
    .chartXAxis {
      AxisMarks(values: .automatic(desiredCount: 5)) { value in
        AxisGridLine()
        AxisValueLabel {
          if let distance = value.as(Double.self) {
            Text(DistanceFormatter.distanceString(fromMeters: distance))
          }
        }
      }
    }
    .chartYAxis {
      AxisMarks(position: .trailing, values: profile.altitudeSteps) { value in
        AxisGridLine()
        AxisValueLabel {
          if let altitude = value.as(Double.self) {
            Text(AltitudeFormatter.altitudeString(fromMeters: altitude))
          }
        }
      }
    }
    .chartXSelection(value: profile.isTrackRecording ? .constant(nil) : $rawSelection)
    .onChange(of: rawSelection) {
      guard let rawSelection, let point = profile.point(at: rawSelection) else { return }
      onSelect(point.coordinates, point.distance)
    }
  }

  private var selectedPoint: ElevationHeightPoint? {
    guard !profile.isTrackRecording else { return nil }
    return (rawSelection ?? activePointDistance).flatMap(profile.point(at:))
  }

  private var myPositionPoint: ElevationHeightPoint? {
    guard !profile.isTrackRecording, let myPositionDistance, myPositionDistance >= 0 else { return nil }
    return profile.point(at: myPositionDistance)
  }
}
