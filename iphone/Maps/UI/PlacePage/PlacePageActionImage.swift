import SwiftUI

struct PlacePageActionImage: View {
  let name: String

  @ScaledMetric(relativeTo: .title3) private var size: CGFloat = 24

  var body: some View {
    Image(name)
      .resizable()
      .scaledToFit()
      .frame(width: size, height: size)
  }
}
