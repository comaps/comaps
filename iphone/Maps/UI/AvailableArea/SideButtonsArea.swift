final class SideButtonsArea: AvailableArea {
  override var deferNotification: Bool { return false }

  override func isAreaAffectingView(_ other: UIView) -> Bool {
    return !other.sideButtonsAreaAffectDirections.isEmpty
  }

  override func addAffectingView(_ other: UIView) {
    let ov = other.sideButtonsAreaAffectView
    let directions = ov.sideButtonsAreaAffectDirections
    addConstraints(otherView: ov, directions: directions)
  }

  override func notifyObserver() {
    MWMSideButtons.updateAvailableArea(areaFrame)
    // The indoor level picker shares this right-side region with the zoom/side buttons.
    IndoorLevelPickerViewController.updateAvailableArea(areaFrame)
  }
}

extension UIView {
  @objc var sideButtonsAreaAffectDirections: MWMAvailableAreaAffectDirections { return [] }

  var sideButtonsAreaAffectView: UIView { return self }
}
