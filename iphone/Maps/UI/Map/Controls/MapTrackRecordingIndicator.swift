import SwiftUI

/// View for a map track recording indicator
struct MapTrackRecordingIndicator: View {
    // MARK: Properties
    
    /// The custom button kind
    @AppStorage(Settings.userDefaultsKeyMapCustomButtonKind) private var customButtonKind: MapCustomButton.Kind = .favourites
    
    
    /// If the track is currently being recorded
    @State private var isRecording: Bool = (TrackRecordingManager.shared.recordingState == .active)
    
    
    /// The publisher for receiving the updates on the track recording state
    private let changeChangeTrackRecordingPublisher = NotificationCenter.default.publisher(for: MapControls.changeTrackRecordingNotificationName)
    
    
    /// The actual view
    var body: some View {
        GeometryReader { geometry in
            HStack(spacing: 0) {
                if customButtonKind != .recordTrack, isRecording {
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
                        Button(role: .destructive) {
                            MapCustomButton.Kind.recordTrack.action
                        } label: {
                            HStack(spacing: 4) {
                                Image(systemName: "inset.filled.circle")
                                
                                Text(MapCustomButton.Kind.recordTrack.description)
                            }
                            .font(.callout)
                            .foregroundStyle(.white)
                        }
                        .buttonStyle(.plain)
                        .frame(height: geometry.size.height / 1.8)
                    }
                    .background {
                        RoundedRectangle(cornerRadius: 28)
                            .stroke(Color.black.opacity(0.1), lineWidth: 1)
                            .background {
                                Capsule()
                                    .fill(Color.BaseColors.red)
                                    .opacity(0.9)
                            }
                            .shadow(radius: 2)
                            .compositingGroup()
                            .frame(height: geometry.size.height / 1.8)
                    }
                    .contentShape(Rectangle())
                    
                    Spacer(minLength: 0)
                }
            }
            .onReceive(changeChangeTrackRecordingPublisher) { _ in
                isRecording = (TrackRecordingManager.shared.recordingState == .active)
            }
        }
    }
}
