import SwiftUI

/// View for a button with all other map actions
struct MapMoreButton: View {
    // MARK: Properties
    
    /// The color scheme of the environment
    @Environment(\.colorScheme) private var colorScheme
    
    
    /// The custom button kind
    @AppStorage(Settings.userDefaultsKeyMapCustomButtonKind) private var customButtonKind: MapCustomButton.Kind = .favourites
    
    
    /// If a badge is being shown
    @State private var hasBadge: Bool = false
    
    
    /// The publisher for receiving map updates
    private let mapUpdatesPublisher = NotificationCenter.default.publisher(for: MapControls.mapUpdatesNotificationName)
    
    
    /// The actual view
    var body: some View {
        Button {
            // Do nothing
        } label: {
            Label("more", image: (hasBadge ? "more.badge" : "more"))
                .hidden()
        }
        .buttonStyle(MapButtonStyle())
        .compositingGroup()
        .accessibilityHidden(true)
        .overlay {
            Menu {
                ForEach(MapCustomButton.Kind.allCases) { kind in
                    if kind != customButtonKind {
                        Button {
                            kind.action
                        } label: {
                            Label {
                                Text(kind.description)
                            } icon: {
                                kind.image
                                    .symbolRenderingMode(kind.hasBadge ? .palette : .monochrome)
                                    .foregroundStyle((kind.hasBadge ? Color.BaseColors.red : (colorScheme == .dark ? .white : .mapButtonForeground)), (colorScheme == .dark ? .white : .mapButtonForeground))
                                    // The use of explicitly setting the colour to white based on the color scheme above is to work around a weird case where the download map icons in the palette rendering mode isn't coloured white
                            }
                            .labelStyle(.titleAndIcon)
                        }
                    }
                }
            } label: {
                Label {
                    Text("more")
                } icon: {
                    Image(hasBadge ? "more.badge" : "more")
                        .padding(.top, hasBadge ? 10 : 0)
                }
                .labelStyle(.iconOnly)
                .font(.title2)
                .scaleEffect(1.1)
                .padding(12)
                .aspectRatio(1, contentMode: .fill)
                .symbolRenderingMode(hasBadge ? .palette : .monochrome)
                .foregroundStyle((hasBadge ? Color.BaseColors.red : Color.mapButtonForeground), Color.mapButtonForeground)
            }
        }
        .contentShape(Rectangle())
        .onAppear {
            updateBadge()
        }
        .onChange(of: customButtonKind) { _ in
            updateBadge()
        }
        .onReceive(mapUpdatesPublisher) { _ in
            updateBadge()
        }
    }
    
    
    
    // MARK: Methods
    
    /// Update the badge
    func updateBadge() {
        hasBadge = (customButtonKind != .downloadMaps && MapControls.hasMapUpdates)
    }
}
