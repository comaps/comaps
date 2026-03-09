import SwiftUI

/// The floating button style used for map buttons
struct MapButtonStyle: ButtonStyle {
    func makeBody(configuration: Configuration) -> some View {
        GeometryReader { geometry in
            configuration.label
                .labelStyle(.iconOnly)
                .font(.title2)
                .scaleEffect(1.1)
                .aspectRatio(1, contentMode: .fill)
                .frame(width: geometry.size.width, height: geometry.size.width)
                .background {
                    RoundedRectangle(cornerRadius: 28)
                        .stroke(Color.MapButtons.border, lineWidth: 1)
                        .background {
                            RoundedRectangle(cornerRadius: 28)
                                .fill(EllipticalGradient(colors: [Color.MapButtons.backgroundGlow, Color.MapButtons.background]))
                                .opacity(configuration.isPressed ? 0.9 : 1)
                        }
                        .aspectRatio(1, contentMode: .fill)
                        .shadow(radius: 2)
                        .compositingGroup()
                }
                .foregroundStyle(configuration.role == .destructive ? Color(.BaseColors.red) : Color.secondary)
                .scaleEffect(configuration.isPressed ? 0.96 : 1)
                .contentShape(Rectangle())
                .animation(.smooth, value: configuration.isPressed)
        }
    }
}
