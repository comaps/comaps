import SwiftUI

struct SearchHeader: View {
  static let controlHeight: CGFloat = 44
  static let panelTopPadding: CGFloat = 12

  let model: SearchModel
  let placement: SearchView.Placement
  let isFieldFocused: FocusState<Bool>.Binding

  @ScaledMetric(relativeTo: .body) private var controlHeight = SearchHeader.controlHeight

  var body: some View {
    controls
      .padding(.horizontal, SearchView.horizontalMargin)
      .padding(.top, placement == .sheet ? 20 : SearchHeader.panelTopPadding)
      .padding(.bottom, 12)
  }

  @ViewBuilder
  private var controls: some View {
    if #available(iOS 26.0, *) {
      GlassEffectContainer {
        row
      }
    } else {
      row
    }
  }

  private var row: some View {
    HStack(spacing: 12) {
      SearchField(model: model, isFocused: isFieldFocused, height: controlHeight)
      SearchCloseButton(size: controlHeight, action: model.didTapClose)
        .keyboardShortcut(.cancelAction)
    }
  }
}
