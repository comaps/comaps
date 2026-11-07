import SwiftUI

struct PlacePageInfoRowButtonStyle: ButtonStyle {
  func makeBody(configuration: Configuration) -> some View {
    configuration.label
      .background(configuration.isPressed ? Color(uiColor: .systemFill) : .clear)
  }
}
