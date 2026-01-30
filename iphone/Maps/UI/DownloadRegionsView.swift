import SwiftUI

/// View for the about information
struct DownloadRegionsView: View {
    // MARK: Properties
    
    
    /// The actual view
    var body: some View {
        RegionsView(isOnlyShowingDownloaded: false)
        .toolbar {
            ToolbarItemGroup(placement: .bottomBar) {
                Button {
                    
                } label: {
                    Text("Progress")
                }
            }
        }
    }
}
