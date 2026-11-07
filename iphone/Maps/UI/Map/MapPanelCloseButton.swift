import SwiftUI

struct MapPanelCloseButton: View {
  let action: @MainActor () -> Void

  var body: some View {
    if #available(iOS 26.0, *) {
      Button(role: .close, action: action) {
        MapPanelButtonLabel(title: L("close"), systemImage: "xmark")
      }
      .mapPanelButtonStyle()
    } else {
      Button(action: action) {
        MapPanelButtonLabel(title: L("close"), systemImage: "xmark")
      }
      .mapPanelButtonStyle()
    }
  }
}
