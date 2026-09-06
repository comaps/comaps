import SwiftUI

/// View for a custom map button
struct MapCustomButton: View {
    // MARK: Properties
    
    /// The custom button kind
    @AppStorage(Settings.userDefaultsKeyMapCustomButtonKind) private var kind: MapCustomButton.Kind = .favourites
    
    
    /// If a badge is being shown
    @State private var hasBadge: Bool = false
    
    
    /// If a record badge is being shown
    @State private var hasRecordBadge: Bool = false
    
    
    /// The publisher for receiving map updates
    private let mapUpdatesPublisher = NotificationCenter.default.publisher(for: MapControls.mapUpdatesNotificationName)
    
    
    /// The publisher for receiving the updates on the track recording state
    private let changeChangeTrackRecordingPublisher = NotificationCenter.default.publisher(for: MapControls.changeTrackRecordingNotificationName)
    
    
    /// The actual view
    var body: some View {
        Button {
            kind.action
        } label: {
            Label {
                Text(kind.description)
            } icon: {
                kind.image
                    .symbolRenderingMode(hasBadge ? .palette : .monochrome)
                    .foregroundStyle((hasBadge ? Color.BaseColors.red : Color.primary), Color.primary)
                    .padding(.top, hasRecordBadge ? 3 : 0)
            }
        }
        .buttonStyle(MapButtonStyle())
        .contentShape(Rectangle())
        .onAppear {
            updateBadges()
        }
        .onChange(of: kind) { changedKind in
            updateBadges()
        }
        .onReceive(mapUpdatesPublisher) { _ in
            updateBadges()
        }
        .onReceive(changeChangeTrackRecordingPublisher) { _ in
            updateBadges()
        }
    }
    
    
    
    // MARK: Methods
    
    /// Update the badges
    func updateBadges() {
        hasBadge = kind.hasBadge
        hasRecordBadge = (kind == .recordTrack) && hasBadge
    }
}
