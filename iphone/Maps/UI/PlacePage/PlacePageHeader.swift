import SwiftUI

struct PlacePageHeader: View {
  let content: PlacePageContent
  let placement: PlacePageModel.Placement

  @State private var shareAnchor: UIView?

  var body: some View {
    HStack(alignment: .top, spacing: 8) {
      VStack(alignment: .leading, spacing: 2) {
        Text(title)
          .font(.title3.weight(.semibold))
        if let secondaryTitle = content.preview.secondaryTitle {
          Text(secondaryTitle)
            .font(.callout.weight(.medium))
        }
      }
      .textSelection(.enabled)
      .accessibilityElement(children: .combine)
      .accessibilityAddTraits(.isHeader)
      .frame(maxWidth: .infinity, alignment: .leading)
      buttons
    }
    .padding(.horizontal)
    .padding(.top, placement == .sheet ? 20 : MapPanelStyle.headerTopPadding)
    .padding(.bottom, 8)
    .contentShape(.rect)
    .onTapGesture {
      if placement == .sheet {
        content.expand()
      }
    }
  }

  private var title: AttributedString {
    let preview = content.preview
    var title = AttributedString(preview.titleWithoutBranch ?? preview.title ?? "")
    if let branch = preview.branch, !branch.isEmpty {
      var branchText = AttributedString(" " + branch)
      branchText.font = .title3
      title += branchText
    }
    return title
  }

  @ViewBuilder
  private var buttons: some View {
    if #available(iOS 26.0, *) {
      GlassEffectContainer {
        buttonRow
      }
    } else {
      buttonRow
    }
  }

  private var buttonRow: some View {
    HStack(spacing: 8) {
      shareButton
        .background(PopoverAnchor(view: $shareAnchor))
      MapPanelCloseButton(action: content.close)
        .keyboardShortcut(.cancelAction)
    }
  }

  @ViewBuilder
  private var shareButton: some View {
    if content.objectType == .track {
      Menu {
        Button(L("export_file")) {
          content.exportTrack(.text, from: shareAnchor)
        }
        Button(L("export_file_gpx")) {
          content.exportTrack(.gpx, from: shareAnchor)
        }
      } label: {
        MapPanelButtonLabel(systemImage: "square.and.arrow.up")
      }
      .mapPanelButtonStyle()
    } else {
      Button {
        content.share(from: shareAnchor)
      } label: {
        MapPanelButtonLabel(systemImage: "square.and.arrow.up")
      }
      .mapPanelButtonStyle()
    }
  }
}
