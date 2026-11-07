import SwiftUI

struct PlacePageDividedStack<Content: View>: View {
  var leadingDivider = false
  var inset: CGFloat = 0
  @ViewBuilder let content: Content

  var body: some View {
    VStack(alignment: .leading, spacing: 0) {
      Group(subviews: content) { subviews in
        ForEach(subviews) { subview in
          if leadingDivider || subview.id != subviews.first?.id {
            Divider()
              .padding(.leading, inset)
          }
          subview
        }
      }
    }
  }
}
