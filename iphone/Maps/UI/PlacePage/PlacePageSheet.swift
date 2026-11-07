import SwiftUI

@objc(PlacePageSheet)
final class PlacePageSheet: NSObject {
  @MainActor @objc static var isOpen: Bool {
    PlacePageModel.shared.isOpen
  }

  @MainActor @objc static func show(_ data: PlacePageData) {
    PlacePageModel.shared.show(data)
  }

  @MainActor @objc static func close() {
    PlacePageModel.shared.close()
  }

  @MainActor @objc static func setMapCovered(_ isCovered: Bool) {
    PlacePageModel.shared.isMapCovered = isCovered
  }
}
