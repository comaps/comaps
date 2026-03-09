import SwiftUI

/// View for a map zoom button
struct MapZoomButton: View {
    // MARK: Properties
    
    /// The kind
    var kind: MapZoomButton.Kind
    
    
    /// The actual view
    var body: some View {
        Button {
            if kind == .in {
                MapControls.zoomIn()
            } else if kind == .out {
                MapControls.zoomOut()
            }
        } label: {
            Label {
                Text(kind.description)
            } icon: {
                kind.image
            }
        }
        .buttonStyle(MapButtonStyle())
        .contentShape(Rectangle())
    }
}
