import SwiftUI

/// View for a map search button (later to be replaced by an actual textfield)
struct MapSearchButton: View {
    // MARK: Properties
    
    /// The color scheme of the environment
    @Environment(\.colorScheme) private var colorScheme
    
    
    /// The actual view
    var body: some View {
        HStack(spacing: 0) {
            Spacer(minLength: 0)
            
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
                RoundedRectangle(cornerRadius: 28)
                    .stroke(Color.MapButtons.border, lineWidth: 1)
                    .background {
                        if colorScheme == .dark {
                            RoundedRectangle(cornerRadius: 28)
                                .fill(Color.black)
                        }
                        
                        RoundedRectangle(cornerRadius: 28)
                            .fill(Color.white.opacity(colorScheme == .dark ? 0.25 : 1))
                    }
                    .shadow(radius: 2)
                    .foregroundStyle(Color.secondary)
                    .compositingGroup()
            }
            .contentShape(Rectangle())
            
            Spacer(minLength: 0)
        }
    }
}
