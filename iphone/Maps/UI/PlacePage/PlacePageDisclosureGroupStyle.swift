import SwiftUI

struct PlacePageDisclosureGroupStyle: DisclosureGroupStyle {
  static let indicatorWidth: CGFloat = 28

  func makeBody(configuration: Configuration) -> some View {
    VStack(alignment: .leading, spacing: 0) {
      Button {
        withAnimation {
          configuration.isExpanded.toggle()
        }
      } label: {
        HStack(alignment: .firstTextBaseline, spacing: 0) {
          configuration.label
          Image(systemName: "chevron.down")
            .font(.footnote.weight(.semibold))
            .foregroundStyle(.tint)
            .rotationEffect(.degrees(configuration.isExpanded ? 180 : 0))
            .frame(width: Self.indicatorWidth, alignment: .trailing)
            .accessibilityHidden(true)
        }
        .contentShape(.rect)
      }
      .buttonStyle(.plain)
      .accessibilityRepresentation {
        DisclosureGroup(isExpanded: configuration.$isExpanded) {
          EmptyView()
        } label: {
          configuration.label
        }
      }
      if configuration.isExpanded {
        configuration.content
          .padding(.trailing, Self.indicatorWidth)
      }
    }
  }
}
