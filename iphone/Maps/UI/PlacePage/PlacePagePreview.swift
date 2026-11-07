import SwiftUI

struct PlacePagePreview: View {
  let content: PlacePageContent

  @State private var isShowingDirection = false

  var body: some View {
    let preview = content.preview
    HStack(alignment: .top, spacing: 12) {
      VStack(alignment: .leading, spacing: 4) {
        if let subtitle {
          Text(subtitle)
            .foregroundStyle(.secondary)
        }
        if let address = preview.secondarySubtitle {
          Text(address)
        }
        if let schedule = PlacePageSchedule(preview.schedule) {
          schedule.text
        }
      }
      .font(.subheadline)
      .textSelection(.enabled)
      .frame(maxWidth: .infinity, alignment: .leading)
      if let distance = content.distance, !preview.isMyPosition {
        directionButton(distance)
      }
    }
    .padding(.horizontal)
    .padding(.bottom, 12)
    .fullScreenCover(isPresented: $isShowingDirection) {
      PlacePageDirectionView(content: content)
    }
  }

  private var subtitle: String? {
    let preview = content.preview
    if preview.isMyPosition {
      return content.speedAndAltitude
    }
    return preview.subtitle ?? preview.coordinates
  }

  private func directionButton(_ distance: String) -> some View {
    Button {
      isShowingDirection = content.direction != nil
    } label: {
      HStack(spacing: 4) {
        if let direction = content.direction {
          Image(systemName: "location.north.fill")
            .rotationEffect(direction)
            .animation(.easeInOut, value: direction)
            .accessibilityHidden(true)
        }
        Text(distance)
      }
      .font(.subheadline)
      .foregroundStyle(.secondary)
    }
    .buttonStyle(.plain)
    .accessibilityLabel(distance)
  }
}
