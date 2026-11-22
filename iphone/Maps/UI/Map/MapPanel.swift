import SwiftUI

extension EnvironmentValues {
  @Entry var mapPanelWidth: CGFloat = MapPanelStyle.width
  @Entry var mapPanelInset: CGFloat = 0
  @Entry var mapPanelBottomInset: CGFloat = 0
}

enum MapPanelStyle {
  static let width: CGFloat = 350
  static let maxWidthFraction: CGFloat = 0.5
  static let padding: CGFloat = 8
  static let cornerRadius: CGFloat = 16
  static let controlSize: CGFloat = 44
  static let headerTopPadding: CGFloat = 12
  static let bottomControlsClearance: CGFloat = 2 * controlSize
}

extension View {
  @ViewBuilder
  func mapPanelBackground(cornerRadius: CGFloat = MapPanelStyle.cornerRadius) -> some View {
    if #available(iOS 26.0, *) {
      glassEffect(.regular, in: .rect(cornerRadius: cornerRadius))
    } else {
      background {
        RoundedRectangle(cornerRadius: cornerRadius)
          .fill(BackgroundStyle())
          .shadow(color: .black.opacity(0.15), radius: 8)
      }
    }
  }

  @ViewBuilder
  func mapPanelButtonStyle() -> some View {
    if #available(iOS 26.0, *) {
      buttonStyle(.plain)
        .glassEffect(.regular.interactive(), in: .circle)
    } else {
      buttonStyle(.plain)
    }
  }
}
