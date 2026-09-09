import UIKit

@objc public class MWMAccessibilityDelegate : NSObject {
  private var explorationEnabled: Bool = false
  private let container: UIView
  private let expandElement: UIAccessibilityElement
  private var mapElements: [NSNumber: UIAccessibilityElement] = [:]

  @objc public init(container: UIView) {
    self.container = container
    self.expandElement = UIAccessibilityElement(accessibilityContainer: self.container)
      super.init()
      self.setEnabled(true)
  }

  @objc public var mwmAccessibilityElements: NSArray {
    if (!self.explorationEnabled) {
      self.expandElement.accessibilityFrameInContainerSpace = self.container.bounds
      self.expandElement.accessibilityLabel = String(localized: "Map. Use rotor to explore by touch.")
      return [self.expandElement]
    } else {
        self.expandElement.accessibilityFrame = CGRectNull
      self.expandElement.accessibilityLabel = String(localized: "Map. Tap and hold to explore by touch. Places will be read out as your finger reaches them. To stop, use rotor to dismiss.")
        return ([self.expandElement] + mapElements.values) as NSArray
    }
  }

  // @MainActor implied
  private func update(removed: [NSNumber], updated: [NSNumber], added: [NSNumber]) {
    for id in removed {
        mapElements.removeValue(forKey:id)
    }
    for id in added {
        mapElements.updateValue(UIAccessibilityElement(accessibilityContainer: self.container), forKey:id)
    }
    for id in added + updated {
      let context = AccessibilityBridging.getAccessibilityNodeContext(id.int32Value)!
      let element = mapElements[id]!
      element.accessibilityLabel = String(context.pointee.GetNodeInfo().m_accessibilityLabel)
        let rect = context.pointee.GetBounds()
        element.accessibilityFrameInContainerSpace = CGRect(x: rect.minX(), y: rect.minY(), width: rect.maxX()-rect.minX(), height: rect.maxY()-rect.minY())
    }
  }
    
    private func setEnabled(_ enabled: Bool) {
        if (enabled) {
            AccessibilityBridging.setAccessibilityUpdateCallback(update)
        } else {
            AccessibilityBridging.setAccessibilityUpdateCallback(nil)
        }
        self.explorationEnabled = enabled
    }
}

