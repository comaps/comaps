import SwiftUI

struct PlacePageWikiSection: View {
  let html: String

  @State private var text: String?

  var body: some View {
    VStack(alignment: .leading, spacing: 8) {
      Text(text ?? "")
        .font(.subheadline)
        .lineLimit(8)
      Button(L("text_more_button")) {
        MWMPlacePageManagerHelper.showPlaceDescription(html)
      }
      .buttonStyle(.borderless)
      .font(.subheadline)
    }
    .frame(maxWidth: .infinity, alignment: .leading)
    .padding()
    .task(id: html) {
      text = NSAttributedString.plainText(fromHTML: html)
    }
  }
}
