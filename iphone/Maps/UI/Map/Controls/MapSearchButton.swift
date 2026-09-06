import SwiftUI

/// View for a map search button (later to be replaced by an actual textfield)
struct MapSearchButton: View {
    // MARK: Properties
    
    /// The vertical size class of the environment
    @Environment(\.verticalSizeClass) private var verticalSizeClass
    
    
    /// The color scheme of the environment
    @Environment(\.colorScheme) private var colorScheme
    
    
    /// The default height of a map control
    var controlHeight: CGFloat
    
    
    /// The actual view
    var body: some View {
        HStack(spacing: 0) {
            if verticalSizeClass != .compact {
                Spacer(minLength: 0)
            }
            
            HStack(spacing: 0) {
                ForEach(Mode.allCases) { mode in
                    Circle()
                        .fill(.clear)
                }
            }
            .padding(4)
            .accessibilityHidden(true)
            .overlay {
                Button {
                    NotificationCenter.default.post(Notification(name: MapControls.presentSearchNotificationName))
                } label: {
                    Label("search", systemImage: "magnifyingglass")
                        .foregroundStyle(.secondary)
                        .padding(.leading)
                        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .leading)
                        .compositingGroup()
                        .background(.white.opacity(0.01))
                }
                .buttonStyle(.plain)
            }
            .background {
                if #unavailable(iOS 27, macOS 27) {
                    RoundedRectangle(cornerRadius: 28, style: .continuous)
                        .stroke(Color.MapButtons.border, lineWidth: 1)
                        .background {
                            if colorScheme == .dark {
                                RoundedRectangle(cornerRadius: 28, style: .continuous)
                                    .fill(Color.black)
                            }

                            RoundedRectangle(cornerRadius: 28, style: .continuous)
                                .fill(Color.white.opacity(colorScheme == .dark ? 0.25 : 1))
                        }
                        .shadow(radius: 2)
                        .foregroundStyle(Color.secondary)
                        .compositingGroup()
                } else {
                    RoundedRectangle(cornerRadius: 28, style: .continuous)
                        .fill(Color.MapButtons.background.opacity(0.3))
                        .glassEffect(.regular, in: RoundedRectangle(cornerRadius: 28, style: .continuous))
                        .shadow(radius: 2)
                }
            }
            .contentShape(Rectangle())
            .padding(.leading, verticalSizeClass == .compact ? (controlHeight + 24) : 0)
            
            Spacer(minLength: 0)
        }
    }
}
