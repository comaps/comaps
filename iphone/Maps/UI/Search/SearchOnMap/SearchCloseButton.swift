import SwiftUI

struct SearchCloseButton: View {
  let size: CGFloat
  let action: @MainActor () -> Void

  var body: some View {
    if #available(iOS 26.0, *) {
      Button(role: .close, action: action) {
        icon
      }
      .buttonStyle(.plain)
      .glassEffect(.regular.interactive(), in: .circle)
      .accessibilityLabel(L("close"))
    } else {
      Button(action: action) {
        icon
          .background(Color(uiColor: .tertiarySystemFill), in: .circle)
      }
      .buttonStyle(.plain)
      .accessibilityLabel(L("close"))
    }
  }

  private var icon: some View {
    Image(systemName: "xmark")
      .font(.body.weight(.semibold))
      .foregroundStyle(.primary)
      .frame(width: size, height: size)
      .contentShape(.circle)
  }
}
