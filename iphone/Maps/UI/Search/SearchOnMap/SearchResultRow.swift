import SwiftUI

@available(iOS 17.0, *)
struct SearchResultRow: View {
  let result: SearchResult

  @Environment(\.dynamicTypeSize) private var dynamicTypeSize
  @ScaledMetric(relativeTo: .body) private var iconSize: CGFloat = 20
  @ScaledMetric(relativeTo: .body) private var iconBackgroundSize: CGFloat = 32

  var body: some View {
    switch result.itemType {
    case .suggestion:
      Label {
        Text(highlightedTitle(font: .body, emphasizedFont: .body.bold()))
          .foregroundStyle(.primary)
      } icon: {
        Image(systemName: "magnifyingglass")
          .foregroundStyle(.secondary)
      }
    default:
      HStack(alignment: .top, spacing: 12) {
        icon
        VStack(alignment: .leading, spacing: 2) {
          Text(highlightedTitle(font: .body.weight(.medium), emphasizedFont: .body.bold()))
            .foregroundStyle(.primary)
          secondaryText(result.infoText)
          secondaryText(result.addressText)
          if let openStatus = result.openStatusText, !openStatus.isEmpty {
            Text(openStatus)
              .font(.subheadline)
              .foregroundStyle(Color(uiColor: result.openStatusColor))
          }
          if dynamicTypeSize.isAccessibilitySize {
            distance
          }
        }
        Spacer(minLength: 0)
        if !dynamicTypeSize.isAccessibilitySize {
          distance
        }
      }
    }
  }

  @ViewBuilder
  private var icon: some View {
    if !result.iconImageName.isEmpty {
      Image(result.iconImageName)
        .renderingMode(.template)
        .resizable()
        .scaledToFit()
        .frame(width: iconSize, height: iconSize)
        .foregroundStyle(.white)
        .frame(width: iconBackgroundSize, height: iconBackgroundSize)
        .background(Color("Base Colors/Blue Color"), in: .circle)
        .accessibilityHidden(true)
    }
  }

  @ViewBuilder
  private var distance: some View {
    if let distance = result.distanceText, !distance.isEmpty {
      Text(distance)
        .font(.subheadline)
        .foregroundStyle(.secondary)
    }
  }

  @ViewBuilder
  private func secondaryText(_ text: String) -> some View {
    if !text.isEmpty {
      Text(text)
        .font(.subheadline)
        .foregroundStyle(.secondary)
    }
  }

  private func highlightedTitle(font: Font, emphasizedFont: Font) -> AttributedString {
    let title = result.titleWithoutBranch.flatMap { $0.isEmpty ? nil : $0 } ?? result.titleText
    let branch = result.branchText ?? ""
    let text = branch.isEmpty ? title : title + " " + branch

    var attributed = AttributedString(text)
    attributed.font = font
    if !branch.isEmpty,
       let range = text.range(of: branch, options: .backwards),
       let attributedRange = Range(range, in: attributed) {
      attributed[attributedRange].font = .body
    }
    for value in result.highlightRanges {
      guard let range = Range(value.rangeValue, in: text),
            let attributedRange = Range(range, in: attributed) else { continue }
      attributed[attributedRange].font = emphasizedFont
    }
    return attributed
  }
}
