import SwiftUI

struct PlacePageDescriptionText: View {
  let text: String
  let isHTML: Bool

  @State private var plainText: String?
  @State private var isExpanded = false
  @State private var limitedHeight: CGFloat = 0
  @State private var fullHeight: CGFloat = 0

  var body: some View {
    let text = plainText ?? (isHTML ? "" : self.text)
    VStack(alignment: .leading, spacing: 4) {
      Text(text)
        .lineLimit(isExpanded ? nil : 5)
        .textSelection(.enabled)
        .onGeometryChange(for: CGFloat.self) { $0.size.height } action: { limitedHeight = $0 }
        .background {
          Text(text)
            .fixedSize(horizontal: false, vertical: true)
            .hidden()
            .onGeometryChange(for: CGFloat.self) { $0.size.height } action: { fullHeight = $0 }
        }
      if !isExpanded && fullHeight > limitedHeight + 1 {
        Button(L("placepage_more_button")) {
          withAnimation {
            isExpanded = true
          }
        }
        .buttonStyle(.borderless)
      }
    }
    .task(id: self.text) {
      plainText = isHTML ? NSAttributedString.plainText(fromHTML: self.text) : nil
    }
  }
}

extension NSAttributedString {
  static func plainText(fromHTML html: String) -> String {
    string(withHtml: html, defaultAttributes: [:])?.string.trimmingCharacters(in: .whitespacesAndNewlines) ?? html
  }
}
