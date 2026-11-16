import SwiftUI

struct SearchField: View {
  @Bindable var model: SearchModel
  let isFocused: FocusState<Bool>.Binding
  let height: CGFloat

  var body: some View {
    HStack(spacing: 6) {
      Image(systemName: "magnifyingglass")
        .foregroundStyle(.secondary)
        .accessibilityHidden(true)
      TextField(L("search"), text: $model.query)
        .focused(isFocused)
        .submitLabel(.search)
        .onSubmit(model.submit)
        .accessibilityAddTraits(.isSearchField)
      if !model.text.isEmpty {
        Button {
          model.query = ""
        } label: {
          Image(systemName: "xmark.circle.fill")
            .foregroundStyle(.secondary)
            .frame(minWidth: 44, minHeight: 44)
            .contentShape(.rect)
        }
        .buttonStyle(.plain)
        .accessibilityLabel(L("clear"))
      }
    }
    .padding(.leading, 12)
    .padding(.trailing, model.text.isEmpty ? 12 : 4)
    .frame(minHeight: height)
    .searchFieldBackground()
  }
}

private extension View {
  @ViewBuilder
  func searchFieldBackground() -> some View {
    if #available(iOS 26.0, *) {
      glassEffect(.regular.interactive(), in: .capsule)
    } else {
      background(Color(uiColor: .tertiarySystemFill), in: .capsule)
    }
  }
}
