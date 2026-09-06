import SwiftUI

/// View for the frequently asked questions
struct FaqView: View {
    // MARK: Properties
    
    /// The actual view
    var body: some View {
        EmbeddedSafariView(url: URL(fileURLWithPath: Bundle.main.path(forResource: "faq", ofType: "html")!), hasDynamicHeight: false)
        .accentColor(.accent)
        .navigationViewStyle(StackNavigationViewStyle())
        .navigationTitle("faq")
    }
}
