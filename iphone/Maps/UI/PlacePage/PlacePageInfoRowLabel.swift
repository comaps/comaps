import SwiftUI

struct PlacePageInfoRowLabel: View {
  let item: PlacePageInfoItem

  @ScaledMetric(relativeTo: .body) private var iconWidth = PlacePageInfoRowLayout.iconWidth

  var body: some View {
    HStack(spacing: PlacePageInfoRowLayout.spacing) {
      icon
        .frame(width: iconWidth)
        .foregroundStyle(item.isLink ? AnyShapeStyle(.tint) : AnyShapeStyle(.primary))
        .accessibilityHidden(true)
      Text(item.text)
        .foregroundStyle(item.isLink ? AnyShapeStyle(.tint) : AnyShapeStyle(item.color ?? .primary))
        .frame(maxWidth: .infinity, alignment: .leading)
    }
    .padding(.horizontal)
    .padding(.vertical, 12)
    .contentShape(.rect)
    .accessibilityElement(children: .combine)
    .accessibilityLabel(item.title ?? item.text)
    .accessibilityValue(item.title == nil ? "" : item.text)
  }

  @ViewBuilder
  private var icon: some View {
    switch item.icon {
    case .system(let name):
      Image(systemName: name)
    case .symbol(let name):
      Image(name)
    case .brand(let resource):
      Image(resource)
        .renderingMode(.template)
        .resizable()
        .scaledToFit()
        .frame(height: iconWidth)
    }
  }
}
