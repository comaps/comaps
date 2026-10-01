final class VisibleArea: AvailableArea {
  override func isAreaAffectingView(_ other: UIView) -> Bool {
    return !other.visibleAreaAffectDirections.isEmpty
  }

  override func addAffectingView(_ other: UIView) {
    let ov = other.visibleAreaAffectView
    let directions = ov.visibleAreaAffectDirections
    addConstraints(otherView: ov, directions: directions)
  }

  override func notifyObserver() {
    if CarPlayService.shared.isCarplayActivated {
      LOG(.info, "\(CarPlayLogging.carPlay) [ViewportDiag] phone VisibleArea skipped reason=carplayActivated areaFrame=\(areaFrame) carHosting=\(CarPlayService.shared.isHostingMapOnCarScreen)")
      return
    }
    LOG(.info, "\(CarPlayLogging.carPlay) [ViewportDiag] phone VisibleArea setVisibleViewport frame=\(areaFrame)")
    FrameworkHelper.setVisibleViewport(areaFrame, scaleFactor: MapViewController.shared()?.mapView.contentScaleFactor ?? 1.0)
  }
}

extension UIView {
  @objc var visibleAreaAffectDirections: MWMAvailableAreaAffectDirections { return [] }

  var visibleAreaAffectView: UIView { return self }
}
