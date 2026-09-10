import CarPlay
import UIKit

enum CarPlayLogging {
  static let carPlay = "[CarPlay]"
  static let scene = "[Scene]"

  static func diagnosticIdentity(_ object: AnyObject?) -> String {
    guard let object else { return "nil" }
    return "\(type(of: object))@\(ObjectIdentifier(object))"
  }

  static func diagnosticTemplate(_ template: CPTemplate?) -> String {
    let identity = diagnosticIdentity(template)
    guard let map = template as? CPMapTemplate else { return identity }
    let type = (map.userInfo as? MapInfo)?.type ?? "?"
    return "\(identity){type=\(type) mapButtons=\(map.mapButtons.count) leading=\(map.leadingNavigationBarButtons.count) trailing=\(map.trailingNavigationBarButtons.count)}"
  }

  static func sceneRole(_ role: UISceneSession.Role) -> String {
    if role.rawValue.contains("Dashboard") { return "dashboard" }
    if role.rawValue.hasPrefix("CP") { return "carplay" }
    if role == .windowApplication { return "phone" }
    return role.rawValue
  }

  static func sceneState(_ state: UIScene.ActivationState) -> String {
    switch state {
    case .unattached: return "unattached"
    case .foregroundActive: return "foregroundActive"
    case .foregroundInactive: return "foregroundInactive"
    case .background: return "background"
    @unknown default: return "unknown(\(state.rawValue))"
    }
  }

  static func appState(_ state: UIApplication.State) -> String {
    switch state {
    case .active: return "active"
    case .inactive: return "inactive"
    case .background: return "background"
    @unknown default: return "unknown(\(state.rawValue))"
    }
  }
}
