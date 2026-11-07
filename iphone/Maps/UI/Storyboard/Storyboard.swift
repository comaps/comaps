@objc(MWMStoryboard)
enum Storyboard: Int {
  case launchScreen
  case main
  case welcome
  case sharing
  case categorySettings
  case carPlay
}

extension UIStoryboard {
  @objc static func instance(_ id: Storyboard) -> UIStoryboard {
    let name: String
    switch id {
    case .launchScreen: name = "LaunchScreen"
    case .main: name = "Main"
    case .welcome: name = "Welcome"
    case .sharing: name = "BookmarksSharingFlow"
    case .categorySettings: name = "CategorySettings"
    case .carPlay: name = "CarPlayStoryboard"
    }
    return UIStoryboard(name: name, bundle: nil)
  }

  func instantiateViewController<T: UIViewController>(ofType: T.Type) -> T {
    let name = String(describing: ofType);
    return self.instantiateViewController(withIdentifier: name) as! T;
  }
}
