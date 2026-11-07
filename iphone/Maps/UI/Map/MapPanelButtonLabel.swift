import SwiftUI

struct MapPanelButtonLabel: View {
  var title: String?
  let systemImage: String

  @ScaledMetric(relativeTo: .body) private var size = MapPanelStyle.controlSize

  var body: some View {
    if let title {
      image
        .accessibilityLabel(title)
    } else {
      image
    }
  }

  @ViewBuilder
  private var image: some View {
    if #available(iOS 26.0, *) {
      icon
    } else {
      icon
        .background(Color(uiColor: .tertiarySystemFill), in: .circle)
    }
  }

  private var icon: some View {
    Image(systemName: systemImage)
      .font(.body.weight(.semibold))
      .foregroundStyle(.primary)
      .frame(width: size, height: size)
      .contentShape(.circle)
  }
}
