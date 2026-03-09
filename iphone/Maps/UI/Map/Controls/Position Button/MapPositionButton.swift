import SwiftUI

/// View for a map position button
struct MapPositionButton: View {
    // MARK: Properties
    
    /// The position mode
    @State private var positionMode: MapPositionButton.Mode = .locate
    
    
    /// The publisher for receiving the updates on the position mode
    private let switchPositionModePublisher = NotificationCenter.default.publisher(for: MapControls.switchPositionModeNotificationName)
    
    
    /// The actual view
    var body: some View {
        Button {
            MapControls.switchToNextPositionMode()
        } label: {
            Label {
                Text(positionMode.description)
            } icon: {
                positionMode.image
                    .foregroundStyle(positionMode == .following || positionMode == .followingAndRotated ? Color.BaseColors.blue : Color.secondary)
                    .padding(.top, positionMode == .locate || positionMode == .following ? 1 : 0)
            }
        }
        .buttonStyle(MapButtonStyle())
        .contentShape(Rectangle())
        .onAppear {
            positionMode = MapControls.positionMode
        }
        .onReceive(switchPositionModePublisher) { _ in
            positionMode = MapControls.positionMode
        }
    }
}
