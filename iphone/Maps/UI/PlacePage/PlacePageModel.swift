import SwiftUI

@MainActor
@Observable
final class PlacePageModel {
  static let shared = PlacePageModel()

  enum Placement {
    case sheet
    case panel
  }

  enum Stop {
    case preview
    case medium
    case large
  }

  private(set) var content: PlacePageContent?
  private(set) var isOpen = false
  private(set) var previewHeight: CGFloat?
  private(set) var contentHeight: CGFloat = 0
  var stop = Stop.preview
  var placement = Placement.sheet
  var isSuspended = false
  var isSheetOnScreen = false
  var isMapCovered = false {
    didSet {
      if oldValue && !isMapCovered {
        content?.refresh()
      }
    }
  }

  private init() {}

  var detents: Set<PresentationDetent> {
    guard let previewHeight else { return [.medium, .large] }
    return [.height(previewHeight), .medium, .large]
  }

  var detent: PresentationDetent {
    get {
      switch stop {
      case .preview:
        return previewHeight.map { .height($0) } ?? .medium
      case .medium:
        return .medium
      case .large:
        return .large
      }
    }
    set {
      switch newValue {
      case .large:
        stop = .large
      case .medium:
        stop = .medium
      default:
        stop = .preview
      }
    }
  }

  var isSheetPresented: Bool {
    get {
      placement == .sheet && isOpen && content != nil && !isMapCovered && !isSuspended
        && !SearchModel.shared.isSheetOnScreen
    }
    set {
      if !newValue && isSheetPresented {
        FrameworkHelper.deactivateMapSelection()
      }
    }
  }

  var isPanelPresented: Bool {
    placement == .panel && isOpen && content != nil
  }

  func show(_ data: PlacePageData) {
    content?.stop()
    content = PlacePageContent(data: data)
    isOpen = true
    stop = data.isPreviewPlus ? .medium : .preview
  }

  func close() {
    content?.stop()
    isOpen = false
    setMapTopBound(0)
    if !isSheetOnScreen {
      content = nil
    }
  }

  func expand() {
    switch stop {
    case .preview:
      stop = .medium
    case .medium:
      stop = .large
    case .large:
      break
    }
  }

  func update(previewHeight: CGFloat?, contentHeight: CGFloat) {
    if abs(contentHeight - self.contentHeight) > 0.5 {
      self.contentHeight = contentHeight
    }
    switch (previewHeight, self.previewHeight) {
    case let (new?, old?) where abs(new - old) <= 0.5:
      break
    case (nil, nil):
      break
    default:
      self.previewHeight = previewHeight
    }
  }

  func sheetDidAppear() {
    isSheetOnScreen = true
  }

  func sheetDidDisappear() {
    isSheetOnScreen = false
    setMapTopBound(0)
    if !isOpen {
      content = nil
    }
  }

  func sheetTopDidChange(_ top: CGFloat) {
    guard isSheetOnScreen, let mapHeight = MapViewController.shared()?.view.bounds.height else { return }
    setMapTopBound(max(0, mapHeight - top))
  }

  private func setMapTopBound(_ bound: CGFloat) {
    MapViewController.shared()?.setPlacePageTopBound(bound)
  }
}
