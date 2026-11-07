import SwiftUI

struct PopoverAnchor: UIViewRepresentable {
  @Binding var view: UIView?

  func makeUIView(context: Context) -> UIView {
    let anchor = UIView()
    anchor.isUserInteractionEnabled = false
    DispatchQueue.main.async {
      view = anchor
    }
    return anchor
  }

  func updateUIView(_ uiView: UIView, context: Context) {}
}
