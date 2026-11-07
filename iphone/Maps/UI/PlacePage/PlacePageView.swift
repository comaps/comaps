import SwiftUI

struct PlacePageView: View {
  let content: PlacePageContent
  let placement: PlacePageModel.Placement

  @State private var heights = Heights()

  private struct Heights: Equatable {
    var header: CGFloat = 0
    var preview: CGFloat = 0
    var scrollContent: CGFloat = 0
    var actionBar: CGFloat = 0
    var bottomInset: CGFloat = 0
  }

  var body: some View {
    VStack(spacing: 0) {
      PlacePageHeader(content: content, placement: placement)
        .fixedSize(horizontal: false, vertical: true)
        .onGeometryChange(for: CGFloat.self) { $0.size.height } action: { heights.header = $0 }
      ScrollView {
        VStack(alignment: .leading, spacing: 0) {
          if content.hasPreview {
            PlacePagePreview(content: content)
              .onGeometryChange(for: CGFloat.self) { $0.size.height } action: { heights.preview = $0 }
          }
          PlacePageSections(content: content)
        }
      }
      .scrollBounceBehavior(.basedOnSize)
      .onScrollGeometryChange(for: CGFloat.self) { $0.contentSize.height } action: { _, height in
        heights.scrollContent = height
      }
      PlacePageActionBar(content: content)
        .fixedSize(horizontal: false, vertical: true)
        .onGeometryChange(for: CGFloat.self) { $0.size.height } action: { heights.actionBar = $0 }
    }
    .onGeometryChange(for: CGFloat.self) { $0.safeAreaInsets.bottom } action: { heights.bottomInset = $0 }
    .onChange(of: heights, initial: true) {
      let previewHeight = content.hasPreview && heights.preview > 0
        ? heights.header + heights.preview + heights.actionBar + heights.bottomInset
        : nil
      let contentHeight = heights.header + heights.scrollContent + heights.actionBar
      PlacePageModel.shared.update(previewHeight: previewHeight, contentHeight: contentHeight)
    }
  }
}
