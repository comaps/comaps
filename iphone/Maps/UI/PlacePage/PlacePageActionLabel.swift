import SwiftUI

struct PlacePageActionLabel<Icon: View>: View {
  let title: String
  @ViewBuilder let icon: Icon

  @ScaledMetric(relativeTo: .title3) private var iconHeight: CGFloat = 28

  var body: some View {
    VStack(spacing: 2) {
      icon
        .font(.title3)
        .frame(height: iconHeight)
      Text(title)
        .font(.caption)
        .lineLimit(1)
        .minimumScaleFactor(0.8)
    }
    .frame(maxWidth: .infinity, minHeight: 44)
    .contentShape(.rect)
  }
}
