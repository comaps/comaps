import SwiftUI

/// The view for the map (for now only yhe controls)
struct MapView: View {
    // MARK: Properties
    
    /// If the settings are being presented
    @Binding var isPresentingSettings: Bool
    
    
    /// If the app information is being presented
    @Binding var isPresentingInformation: Bool
    
    
    /// The actual view
    var body: some View {
        MapOverlayView(isPresentingSettings: $isPresentingSettings, isPresentingInformation: $isPresentingInformation)
    }
}
