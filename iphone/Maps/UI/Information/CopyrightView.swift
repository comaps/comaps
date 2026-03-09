import SwiftUI

/// View for the copyright info
struct CopyrightView: View {
    // MARK: Properties
    
    /// The actual view
    var body: some View {
        EmbeddedSafariView(url: URL(fileURLWithPath: Bundle.main.path(forResource: "copyright", ofType: "html")!), hasDynamicHeight: false)
        .accentColor(.accent)
        .navigationViewStyle(StackNavigationViewStyle())
        .navigationTitle("copyright")
    }
}
