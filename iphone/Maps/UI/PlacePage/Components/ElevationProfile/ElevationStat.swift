import SwiftUI

struct ElevationStat: View {
  let title: String
  let value: String
  let image: String

  @ScaledMetric(relativeTo: .subheadline) private var iconSize: CGFloat = 20

  var body: some View {
    VStack(alignment: .leading, spacing: 0) {
      HStack(spacing: 6) {
        Image(image)
          .renderingMode(.template)
          .resizable()
          .scaledToFit()
          .frame(width: iconSize, height: iconSize)
          .foregroundStyle(.primary)
        Text(value)
          .font(.subheadline.weight(.medium))
          .lineLimit(1)
          .minimumScaleFactor(0.7)
      }
      Text(title)
        .font(.caption2)
        .lineLimit(1)
        .minimumScaleFactor(0.7)
    }
    .foregroundStyle(.secondary)
    .accessibilityElement(children: .ignore)
    .accessibilityLabel(title)
    .accessibilityValue(value)
  }
}
