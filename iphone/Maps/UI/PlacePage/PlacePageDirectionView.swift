import SwiftUI

struct PlacePageDirectionView: View {
  let content: PlacePageContent

  @Environment(\.dismiss) private var dismiss
  @Environment(\.verticalSizeClass) private var verticalSizeClass

  var body: some View {
    let preview = content.preview
    let layout = verticalSizeClass == .compact
      ? AnyLayout(HStackLayout(spacing: 32))
      : AnyLayout(VStackLayout(spacing: 24))
    layout {
      Image(systemName: "location.north.fill")
        .resizable()
        .scaledToFit()
        .frame(maxWidth: 200, maxHeight: 200)
        .foregroundStyle(Color.BaseColors.green)
        .rotationEffect(content.direction ?? .zero)
        .animation(.easeInOut, value: content.direction)
        .accessibilityHidden(true)
      VStack(spacing: 8) {
        if let title = preview.title {
          Text(title)
            .font(.title2.weight(.semibold))
        }
        if let subtitle = preview.subtitle ?? preview.coordinates {
          Text(subtitle)
            .foregroundStyle(.secondary)
        }
        if let distance = content.distance {
          Text(distance)
            .font(.system(.largeTitle, design: .rounded).weight(.medium))
            .monospacedDigit()
        }
      }
      .multilineTextAlignment(.center)
      .accessibilityElement(children: .combine)
    }
    .padding()
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .contentShape(.rect)
    .onTapGesture {
      dismiss()
    }
    .overlay(alignment: .topTrailing) {
      MapPanelCloseButton {
        dismiss()
      }
      .padding()
    }
    .onAppear {
      MapsAppDelegate.theApp().disableStandby()
    }
    .onDisappear {
      MapsAppDelegate.theApp().enableStandby()
    }
  }
}
