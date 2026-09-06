final class ModeButtonArea: AvailableArea {
  override func isAreaAffectingView(_ other: UIView) -> Bool {
    return !other.modeButtonAreaAffectDirections.isEmpty
  }

  override func addAffectingView(_ other: UIView) {
    let ov = other.modeButtonAreaAffectView
    let directions = ov.modeButtonAreaAffectDirections
    addConstraints(otherView: ov, directions: directions)
  }
}

extension UIView {
  @objc var modeButtonAreaAffectDirections: MWMAvailableAreaAffectDirections { return [] }

  var modeButtonAreaAffectView: UIView { return self }
}
