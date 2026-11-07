import SwiftUI

struct PlacePageElevationProfile: View {
  let content: PlacePageContent

  @Environment(\.dynamicTypeSize) private var dynamicTypeSize

  var body: some View {
    let columns = Array(repeating: GridItem(.flexible(), spacing: 8, alignment: .leading),
                        count: dynamicTypeSize.isAccessibilitySize ? 2 : 4)
    VStack(spacing: 12) {
      if let trackInfo = content.track?.trackInfo {
        LazyVGrid(columns: columns, alignment: .leading, spacing: 8) {
          ElevationStat(title: L("elevation_profile_ascent"), value: trackInfo.ascent, image: "ic_em_ascent_24")
          ElevationStat(title: L("elevation_profile_descent"), value: trackInfo.descent, image: "ic_em_descent_24")
          ElevationStat(title: L("elevation_profile_max_elevation"), value: trackInfo.maxElevation, image: "ic_em_max_attitude_24")
          ElevationStat(title: L("elevation_profile_min_elevation"), value: trackInfo.minElevation, image: "ic_em_min_attitude_24")
        }
      }
      if let data = content.track?.elevationProfileData, data.points.count >= 2 {
        ElevationChart(profile: ElevationProfile(data),
                       activePointDistance: content.activePointDistance,
                       myPositionDistance: content.myPositionDistance,
                       onSelect: content.selectElevationPoint)
      }
    }
    .padding()
  }
}
