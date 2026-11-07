import SwiftUI

struct PlacePageBookmarkSection: View {
  let content: PlacePageContent
  let mark: PlacePageContent.UserMark

  @ScaledMetric(relativeTo: .body) private var colorSize: CGFloat = 28

  var body: some View {
    VStack(alignment: .leading, spacing: 8) {
      HStack(spacing: 12) {
        Button(action: content.pickColor) {
          Image(uiImage: circleImageForColor(mark.color, frameSize: 28, diameter: 22, iconName: "ic_bm_none"))
            .resizable()
            .frame(width: colorSize, height: colorSize)
        }
        .accessibilityLabel(L("change_color"))
        Button(action: content.pickCategory) {
          Text(mark.category ?? "")
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .accessibilityHint(L("bookmark_lists"))
        Button(action: content.editBookmarkOrTrack) {
          Image(.ic24PxEdit)
            .renderingMode(.template)
        }
        .accessibilityLabel(L("edit"))
      }
      .buttonStyle(.borderless)
      if let description = mark.description, !description.isEmpty {
        PlacePageDescriptionText(text: description, isHTML: mark.isHTMLDescription)
          .font(.subheadline)
      }
    }
    .padding()
  }
}
