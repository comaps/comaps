import SwiftUI

/// The main view of the app
struct MainView: View {
    // MARK: Properties
    
    /// If the settings are being presented
    @State private var isPresentingSettings: Bool = false
    
    
    /// If the app information is being presented
    @State private var isPresentingInformation: Bool = false
    
    
    /// The publisher for receiving the request to present the settings
    private let presentSettingsPublisher = NotificationCenter.default.publisher(for: MapControls.presentSettingsNotificationName)
    
    
    /// The publisher for receiving the request to present the app information
    private let presentHelpPublisher = NotificationCenter.default.publisher(for: MapControls.presentInformationNotificationName)
    
    
    /// The actual view
    var body: some View {
        MapView(isPresentingSettings: $isPresentingSettings, isPresentingInformation: $isPresentingInformation)
            .sheet(isPresented: $isPresentingSettings) {
                SettingsView()
            }
            .sheet(isPresented: $isPresentingInformation) {
                InformationView()
            }
            .onReceive(presentSettingsPublisher) { _ in
                isPresentingSettings = true
                isPresentingInformation = false
            }
            .onReceive(presentHelpPublisher) { _ in
                isPresentingSettings = false
                isPresentingInformation = true
            }
    }
}
