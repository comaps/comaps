import SwiftUI

struct SearchView: View {
  enum Placement {
    case sheet
    case panel
  }

  static let horizontalMargin: CGFloat = 16
  static let rowInsets = EdgeInsets(top: 10, leading: horizontalMargin, bottom: 10, trailing: horizontalMargin)

  let model: SearchModel
  let placement: Placement

  @FocusState private var isFieldFocused: Bool

  var body: some View {
    VStack(spacing: 0) {
      SearchHeader(model: model, placement: placement, isFieldFocused: $isFieldFocused)
      SearchContent(model: model)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
    .task {
      isFieldFocused = model.isTyping
    }
    .onChange(of: model.isTyping) {
      isFieldFocused = model.isTyping
    }
    .onChange(of: isFieldFocused) {
      model.didChangeFocus(isFieldFocused)
    }
  }
}
