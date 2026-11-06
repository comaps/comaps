import SwiftUI
import WebKit

/// Content of the view for Safari via a WebKit view
struct EmbeddedSafariViewContent: UIViewRepresentable {
    // MARK: Properties
    
    /// If the content is loading
    @Binding var isLoading: Bool
    
    
    /// The view height
    @Binding var height: CGFloat
    
    
    /// If the view should resize itself to the height of the website content
    var hasDynamicHeight: Bool = true
    
    
    /// The url
    let url: URL
    
    
    
    // MARK: Methods
    
    /// Create a coodindator for the WebKit view
    func makeCoordinator() -> EmbeddedSafariViewCoordinator {
        EmbeddedSafariViewCoordinator(self)
    }
    
    
    /// Create a WebKit view
    /// - Parameter context: The context
    /// - Returns: The WebKit view
    func makeUIView(context: UIViewRepresentableContext<EmbeddedSafariViewContent>) -> WKWebView {
        let uiView = WKWebView()
        uiView.navigationDelegate = context.coordinator
        uiView.scrollView.isScrollEnabled = !hasDynamicHeight
        uiView.scrollView.showsHorizontalScrollIndicator = false
        uiView.allowsBackForwardNavigationGestures = false
        uiView.allowsLinkPreview = false
        uiView.isFindInteractionEnabled = false
        uiView.isOpaque = false
        uiView.backgroundColor = .clear
        uiView.underPageBackgroundColor = .clear
        uiView.load(URLRequest(url: url))
        return uiView
    }
    
    
    /// Update the WebKit view
    /// - Parameter context: The context
    func updateUIView(_ uiView: WKWebView, context: UIViewRepresentableContext<EmbeddedSafariViewContent>) {
        uiView.load(URLRequest(url: url))
    }
}
