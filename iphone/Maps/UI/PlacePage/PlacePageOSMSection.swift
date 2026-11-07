import SwiftUI

struct PlacePageOSMSection: View {
  let content: PlacePageContent
  let data: PlacePageOSMContributionData

  var body: some View {
    VStack(alignment: .leading, spacing: 12) {
      HStack {
        Text(L("contribute_to_osm"))
        Spacer()
        Button {
          content.openURL("https://welcome.openstreetmap.org")
        } label: {
          Image(systemName: "questionmark.circle")
        }
        .buttonStyle(.borderless)
        .accessibilityLabel(L("help"))
      }
      ViewThatFits(in: .horizontal) {
        HStack(spacing: 12) {
          buttons
        }
        VStack(spacing: 8) {
          buttons
        }
      }
      .buttonStyle(.bordered)
    }
    .padding()
  }

  @ViewBuilder
  private var buttons: some View {
    switch data.state {
    case .canAddOrEditPlace:
      if data.showAddPlace {
        button(L("contribute_to_osm_add_place")) {
          MWMPlacePageManagerHelper.addPlace(content.data.locationCoordinate)
        }
      }
      if data.showEditPlace {
        button(L("edit_place")) {
          MWMPlacePageManagerHelper.editPlace()
        }
      }
    case .shouldUpdateMap:
      button(L("contribute_to_osm_update_map"), action: content.toggleMapDownload)
    case .mapIsDownloading:
      button(L("downloading")) {}
        .disabled(true)
    @unknown default:
      EmptyView()
    }
  }

  private func button(_ title: String, action: @escaping @MainActor () -> Void) -> some View {
    Button(action: action) {
      Text(title)
        .frame(maxWidth: .infinity)
    }
  }
}
