import SwiftUI

/// View for the about information
struct DownloadedRegionsView: View {
    // MARK: Properties
    
    /// The dismiss action of the environment
    @Environment(\.dismiss) private var dismiss
    
    /// The actual view
    var body: some View {
        NavigationView {
            RegionsView(isOnlyShowingDownloaded: true)
            .toolbar {
                ToolbarItem(placement: .confirmationAction) {
                    Button {
                        dismiss()
                    } label: {
                        Text("close")
                    }
                }
                
                ToolbarItem(placement: .cancellationAction) {
                    EditButton()
                }
                
                ToolbarItemGroup(placement: .bottomBar) {
                    Button {
                        dismiss()
                    } label: {
                        Text("Progress")
                    }
                    
                    Spacer()
                    
                    NavigationLink {
                        DownloadRegionsView()
                    } label: {
                        Text("Add")
                    }
                }
            }
        }
        .navigationViewStyle(StackNavigationViewStyle())
        .navigationBarTitleDisplayMode(.inline)
        .accentColor(.toolbarAccent)
    }
}
