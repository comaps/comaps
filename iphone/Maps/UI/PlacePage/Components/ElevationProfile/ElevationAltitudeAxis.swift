import SwiftUI

struct ElevationAltitudeAxis {
  private let step: Double
  private let metersPerUnit: Double

  init(units: Units = SettingsBridge.measurementUnits()) {
    switch units {
    case .imperial:
      step = 100
      metersPerUnit = 0.3048
    default:
      step = 50
      metersPerUnit = 1
    }
  }

  var stepInMeters: Double {
    step * metersPerUnit
  }

  func lowerBound(_ altitude: Double) -> Double {
    floor(altitude / metersPerUnit / step) * stepInMeters
  }

  func upperBound(_ altitude: Double) -> Double {
    ceil(altitude / metersPerUnit / step) * stepInMeters
  }

  func steps(lower: Double, upper: Double) -> [Double] {
    var stepSize = stepInMeters
    while ((upper - lower) / stepSize).rounded(.up) > 6 {
      stepSize *= 2
    }
    return Array(stride(from: lower, through: upper, by: stepSize))
  }
}
