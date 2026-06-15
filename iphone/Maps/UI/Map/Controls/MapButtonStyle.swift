import SwiftUI

/// The floating button style used for map buttons
struct MapButtonStyle: ButtonStyle {
    var isClear = false
    
    func makeBody(configuration: Configuration) -> some View {
        GeometryReader { geometry in
            configuration.label
                .labelStyle(.iconOnly)
                .font(.title2)
                .scaleEffect(1.1)
                .aspectRatio(1, contentMode: .fill)
                .frame(width: geometry.size.width, height: geometry.size.width)
                .background {
                    if #unavailable(iOS 27, macOS 27) {
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
                    } else {
                        RoundedRectangle(cornerRadius: 28)
                            .fill(Color.MapButtons.background.opacity(isClear ? 0 : 0.3))
                            .glassEffect(isClear ? .clear : .regular, in: RoundedRectangle(cornerRadius: 28))
                            .aspectRatio(1, contentMode: .fill)
                            .shadow(radius: 2)
                            .opacity(configuration.isPressed ? 0.9 : 1)
                    }
                }
                .foregroundStyle(configuration.role == .destructive ? Color(.BaseColors.red) : Color.primary)
                .scaleEffect(configuration.isPressed ? 0.96 : 1)
                .contentShape(Rectangle())
                .animation(.smooth, value: configuration.isPressed)
        }
    }
}
