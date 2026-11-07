import Chart
import SwiftUI

struct PlacePageElevationProfile: View {
  let content: PlacePageContent

  @Environment(\.dynamicTypeSize) private var dynamicTypeSize

  var body: some View {
    let columns = Array(repeating: GridItem(.flexible(), spacing: 8, alignment: .leading),
                        count: dynamicTypeSize.isAccessibilitySize ? 2 : 4)
    VStack(spacing: 4) {
      if let trackInfo = content.track?.trackInfo {
        LazyVGrid(columns: columns, alignment: .leading, spacing: 8) {
          ElevationStat(title: L("elevation_profile_ascent"), value: trackInfo.ascent, image: "ic_em_ascent_24")
          ElevationStat(title: L("elevation_profile_descent"), value: trackInfo.descent, image: "ic_em_descent_24")
          ElevationStat(title: L("elevation_profile_max_elevation"), value: trackInfo.maxElevation, image: "ic_em_max_attitude_24")
          ElevationStat(title: L("elevation_profile_min_elevation"), value: trackInfo.minElevation, image: "ic_em_min_attitude_24")
        }
        .padding(.horizontal)
        .padding(.top, 16)
      }
      ElevationChart(content: content,
                     revision: content.revision,
                     activePointDistance: content.activePointDistance,
                     myPositionDistance: content.myPositionDistance)
        .frame(height: 176)
        .padding(.horizontal)
    }
  }
}

private struct ElevationStat: View {
  let title: String
  let value: String
  let image: String

  @ScaledMetric(relativeTo: .subheadline) private var iconSize: CGFloat = 20

  var body: some View {
    VStack(alignment: .leading, spacing: 0) {
      HStack(spacing: 6) {
        Image(image)
          .renderingMode(.template)
          .resizable()
          .scaledToFit()
          .frame(width: iconSize, height: iconSize)
          .foregroundStyle(.primary)
        Text(value)
          .font(.subheadline.weight(.medium))
          .lineLimit(1)
          .minimumScaleFactor(0.7)
      }
      Text(title)
        .font(.caption2)
        .lineLimit(1)
        .minimumScaleFactor(0.7)
    }
    .foregroundStyle(.secondary)
    .accessibilityElement(children: .ignore)
    .accessibilityLabel(title)
    .accessibilityValue(value)
  }
}

private struct ElevationChart: UIViewRepresentable {
  let content: PlacePageContent
  let revision: Int
  let activePointDistance: Double?
  let myPositionDistance: Double?

  final class Coordinator {
    var revision: Int?
    var chartData: ElevationProfileChartData?
    var activePointDistance: Double?
    var myPositionDistance: Double?
    let formatter = ElevationProfileFormatter()
  }

  func makeCoordinator() -> Coordinator {
    Coordinator()
  }

  func makeUIView(context: Context) -> ChartView {
    let chartView = ChartView()
    let coordinator = context.coordinator
    let content = content
    chartView.onSelectedPointChanged = { [weak content] point in
      guard let content, let chartData = coordinator.chartData, !chartData.points.isEmpty else { return }
      let distance: Double = floor(point) / CGFloat(chartData.points.count) * chartData.maxDistance
      let selected = chartData.points.first { $0.distance >= distance } ?? chartData.points[0]
      content.selectElevationPoint(selected.coordinates, distance: selected.distance)
    }
    return chartView
  }

  func updateUIView(_ chartView: ChartView, context: Context) {
    let coordinator = context.coordinator
    if coordinator.revision != revision {
      coordinator.revision = revision
      configure(chartView, coordinator: coordinator)
    }
    guard chartView.chartData != nil, content.track?.elevationProfileData?.isTrackRecording == false else { return }
    if let distance = activePointDistance, distance != coordinator.activePointDistance {
      coordinator.activePointDistance = distance
      chartView.setSelectedPoint(distance)
    }
    if let distance = myPositionDistance, distance != coordinator.myPositionDistance {
      coordinator.myPositionDistance = distance
      chartView.myPosition = distance
    }
  }

  private func configure(_ chartView: ChartView, coordinator: Coordinator) {
    coordinator.chartData = content.track?.elevationProfileData.map(ElevationProfileChartData.init)
    guard let track = content.track,
          let profileData = track.elevationProfileData,
          let chartData = coordinator.chartData,
          chartData.points.count >= 2 else {
      chartView.isUserInteractionEnabled = false
      return
    }
    chartView.chartData = ChartPresentationData(chartData, formatter: coordinator.formatter)
    guard !profileData.isTrackRecording else {
      chartView.isChartViewInfoHidden = true
      return
    }
    chartView.isUserInteractionEnabled = true
    coordinator.activePointDistance = track.activePointDistance
    coordinator.myPositionDistance = track.myPositionDistance
    chartView.setSelectedPoint(track.activePointDistance)
    chartView.myPosition = track.myPositionDistance
  }
}

struct ElevationProfileChartData {
  struct Line: ChartLine {
    var values: [ChartValue]
    var color: UIColor
    var type: ChartLineType
  }

  let chartLines: [Line]
  let distances: [Double]
  let maxDistance: Double
  let points: [ElevationHeightPoint]

  init(_ elevationData: ElevationProfileData) {
    points = elevationData.points
    let chartValues = points.map { ChartValue(xValues: $0.distance, y: $0.altitude) }
    distances = points.map { $0.distance }
    maxDistance = distances.last ?? 0
    let lineColor = StyleManager.shared.theme?.colors.chartLine ?? .blue
    let lineShadowColor = StyleManager.shared.theme?.colors.chartShadow ?? .lightGray
    chartLines = [Line(values: chartValues, color: lineColor, type: .line),
                  Line(values: chartValues, color: lineShadowColor, type: .lineArea)]
  }
}

extension ElevationProfileChartData: ChartData {
  var xAxisValues: [Double] { distances }
  var lines: [ChartLine] { chartLines }
  var type: ChartType { .regular }
}
