import Foundation
import UIKit

enum CarPlayWindowScaleAdjuster {

  private static var appliedScale: CGFloat?

  static func updateAppearance(toWindow destinationWindow: UIWindow?, isCarplayActivated: Bool) {
    if isCarplayActivated {
      guard let destinationContentScale = destinationWindow?.screen.scale,
            appliedScale != destinationContentScale else { return }
      appliedScale = destinationContentScale
      updateVisualScale(to: destinationContentScale)
    } else {
      guard appliedScale != nil else { return }
      appliedScale = nil
      updateVisualScaleToMain()
    }
  }

  private static func updateVisualScale(to scale: CGFloat) {
    if isGraphicContextInitialized {
      mapViewController?.mapView.updateVisualScale(to: scale)
    } else {
      DispatchQueue.main.async {
        updateVisualScale(to: scale)
      }
    }
  }

  private static func updateVisualScaleToMain() {
    if isGraphicContextInitialized {
      mapViewController?.mapView.updateVisualScaleToMain()
    } else {
      DispatchQueue.main.async {
        updateVisualScaleToMain()
      }
    }
  }

  private static var isGraphicContextInitialized: Bool {
    return mapViewController?.mapView.graphicContextInitialized ?? false
  }

  private static var mapViewController: MapViewController? {
    return MapViewController.shared()
  }
}
