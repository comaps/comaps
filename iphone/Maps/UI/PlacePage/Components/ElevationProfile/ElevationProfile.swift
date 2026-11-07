import SwiftUI

struct ElevationProfile {
  struct Sample {
    let distance: Double
    let altitude: Double
    let base: Double
  }

  let points: [ElevationHeightPoint]
  let samples: [Sample]
  let isTrackRecording: Bool
  let altitudeDomain: ClosedRange<Double>
  let altitudeSteps: [Double]
  let maxDistance: Double

  init(_ data: ElevationProfileData) {
    points = data.points
    isTrackRecording = data.isTrackRecording
    maxDistance = points.last?.distance ?? 0
    let axis = ElevationAltitudeAxis()
    let altitudes = points.map(\.altitude)
    let lower = axis.lowerBound(altitudes.min() ?? 0)
    let upper = max(axis.upperBound(altitudes.max() ?? 0), lower + axis.stepInMeters)
    altitudeDomain = lower...upper
    altitudeSteps = axis.steps(lower: lower, upper: upper)
    samples = points.map { Sample(distance: $0.distance, altitude: $0.altitude, base: lower) }
  }

  func point(at distance: Double) -> ElevationHeightPoint? {
    points.first { $0.distance >= distance } ?? points.last
  }
}
