import SwiftUI

struct PlacePageActionBar: View {
  let content: PlacePageContent

  @State private var isConfirmingTrackDeletion = false
  @AppStorage("kUDDidHighlightPoint2PointButton") private var didHighlightRouteTo = false

  var body: some View {
    let items = content.actionBarItems
    HStack(alignment: .top, spacing: 0) {
      ForEach(items.visible, id: \.self) { action in
        button(for: action)
      }
      if !items.overflow.isEmpty {
        Menu {
          Section(content.preview.title ?? "") {
            ForEach(items.overflow, id: \.self) { action in
              menuItem(for: action)
            }
          }
        } label: {
          PlacePageActionLabel(title: L("placepage_more_button")) {
            Image(systemName: "ellipsis")
          }
        }
        .tint(.primary)
      }
    }
    .buttonStyle(.borderless)
    .padding(.horizontal, 4)
    .padding(.top, 6)
    .padding(.bottom, 4)
    .overlay(alignment: .top) {
      Divider()
    }
    .confirmationDialog(L("placepage_delete_track_confirmation_alert_message"),
                        isPresented: $isConfirmingTrackDeletion,
                        titleVisibility: .visible) {
      Button(L("delete"), role: .destructive) {
        content.perform(.deleteTrack)
      }
    }
  }

  @ViewBuilder
  private func button(for action: PlacePageAction) -> some View {
    let phones = content.data.infoData?.phones ?? []
    Group {
      if action == .call && phones.count > 1 {
        Menu {
          phoneButtons(phones)
        } label: {
          label(for: action)
        }
      } else {
        Button {
          perform(action)
        } label: {
          label(for: action)
        }
      }
    }
    .tint(tint(for: action))
  }

  @ViewBuilder
  private func menuItem(for action: PlacePageAction) -> some View {
    let phones = content.data.infoData?.phones ?? []
    if action == .call && phones.count > 1 {
      Menu(title(for: action)) {
        phoneButtons(phones)
      }
    } else {
      Button(title(for: action), role: action == .deleteTrack ? .destructive : nil) {
        perform(action)
      }
    }
  }

  private func phoneButtons(_ phones: [PlacePagePhone]) -> some View {
    ForEach(phones, id: \.phone) { phone in
      Button(phone.phone) {
        content.call(phone)
      }
    }
  }

  private func perform(_ action: PlacePageAction) {
    switch action {
    case .deleteTrack:
      isConfirmingTrackDeletion = true
    case .routeTo:
      didHighlightRouteTo = true
      content.perform(action)
    default:
      content.perform(action)
    }
  }

  private func label(for action: PlacePageAction) -> some View {
    PlacePageActionLabel(title: title(for: action)) {
      icon(for: action)
    }
    .accessibilityValue(action == .download ? downloadValue : "")
  }

  private var downloadValue: String {
    guard content.mapNodeStatus == .downloading, let progress = content.downloadProgress else { return "" }
    return progress.formatted(.percent.precision(.fractionLength(0)))
  }

  private func title(for action: PlacePageAction) -> String {
    switch action {
    case .download:
      return L("download")
    case .call:
      return L("placepage_call_button")
    case .routeFrom:
      return L("p2p_from_here")
    case .routeTo:
      return L("p2p_to_here")
    case .routeAddStop:
      return L("placepage_add_stop")
    case .routeRemoveStop:
      return L("placepage_remove_stop")
    case .bookmark:
      if content.bookmark != nil {
        return L("delete")
      }
      return content.didRemoveBookmark && BookmarksManager.shared().hasRecentlyDeletedBookmark() ? L("restore") : L("save")
    case .deleteTrack:
      return L("delete")
    case .saveTrackRecording:
      return L("save")
    case .discardTrackRecording:
      return L("discard")
    case .avoidToll:
      return L("avoid_tolls")
    case .avoidDirty:
      return L("avoid_unpaved")
    case .avoidFerry:
      return L("avoid_ferry")
    }
  }

  private func tint(for action: PlacePageAction) -> Color {
    switch action {
    case .deleteTrack, .discardTrackRecording:
      return .red
    case .routeTo where !didHighlightRouteTo:
      return .accentColor
    case .download:
      return .accentColor
    default:
      return .primary
    }
  }

  @ViewBuilder
  private func icon(for action: PlacePageAction) -> some View {
    switch action {
    case .download:
      PlacePageDownloadIcon(status: content.mapNodeStatus, progress: content.downloadProgress)
    case .call:
      Image(systemName: "phone.fill")
    case .routeFrom:
      PlacePageActionImage(name: "ic_route_from")
    case .routeTo:
      PlacePageActionImage(name: "ic_route_to")
    case .routeAddStop:
      PlacePageActionImage(name: "ic_add_route_point")
    case .routeRemoveStop:
      PlacePageActionImage(name: "ic_remove_route_point")
    case .bookmark:
      PlacePageActionImage(name: content.bookmark != nil ? "ic_bookmarks_on" : "ic_bookmarks_off")
    case .deleteTrack:
      PlacePageActionImage(name: "ic_route_manager_trash")
    case .saveTrackRecording:
      Image(systemName: "square.and.arrow.down")
    case .discardTrackRecording:
      Image(systemName: "trash.fill")
    case .avoidToll:
      PlacePageActionImage(name: "ic_avoid_tolls")
    case .avoidDirty:
      PlacePageActionImage(name: "ic_avoid_dirty")
    case .avoidFerry:
      PlacePageActionImage(name: "ic_avoid_ferry")
    }
  }
}
