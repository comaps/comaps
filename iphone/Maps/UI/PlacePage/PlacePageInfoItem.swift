import SwiftUI

struct PlacePageInfoItem {
  enum Icon {
    case system(String)
    case symbol(String)
    case brand(ImageResource)
  }

  let icon: Icon
  let text: String
  var title: String?
  var color: Color?
  var isLink = false
  var copyText: String?
  var action: (@MainActor () -> Void)?
}
