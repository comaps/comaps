import SwiftUI

struct PlacePageInfoRow<Accessory: View>: View {
  let item: PlacePageInfoItem
  let copy: @MainActor (String) -> Void
  @ViewBuilder var accessory: Accessory

  var body: some View {
    HStack(spacing: 0) {
      if let action = item.action, item.isLink {
        Button(action: action) {
          PlacePageInfoRowLabel(item: item)
        }
        .buttonStyle(PlacePageInfoRowButtonStyle())
      } else {
        PlacePageInfoRowLabel(item: item)
      }
      accessory
        .foregroundStyle(.tint)
        .padding(.trailing, 8)
    }
    .contextMenu {
      if let copyText = item.copyText {
        Button(L("copy_to_clipboard"), systemImage: "doc.on.doc") {
          copy(copyText)
        }
      }
    }
  }
}

extension PlacePageInfoRow where Accessory == EmptyView {
  init(item: PlacePageInfoItem, copy: @escaping @MainActor (String) -> Void) {
    self.init(item: item, copy: copy) {
      EmptyView()
    }
  }
}
