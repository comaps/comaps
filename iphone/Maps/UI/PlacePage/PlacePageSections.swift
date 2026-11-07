import SwiftUI

struct PlacePageSections: View {
  let content: PlacePageContent

  var body: some View {
    PlacePageDividedStack(leadingDivider: content.objectType != .trackRecording) {
      switch content.objectType {
      case .track:
        if let mark = content.userMark {
          PlacePageBookmarkSection(content: content, mark: mark)
        }
        if let track = content.track, track.trackInfo.hasElevationInfo, track.elevationProfileData != nil {
          PlacePageElevationProfile(content: content)
        }
      case .trackRecording:
        if content.track != nil {
          PlacePageElevationProfile(content: content)
        }
      default:
        if let mark = content.userMark {
          PlacePageBookmarkSection(content: content, mark: mark)
        }
        if let html = content.data.wikiDescriptionHtml {
          PlacePageWikiSection(html: html)
        }
        if let info = content.data.infoData {
          PlacePageInfoSection(content: content, info: info)
        }
        if let osmContribution = content.osmContribution {
          PlacePageOSMSection(content: content, data: osmContribution)
        }
      }
    }
  }
}
