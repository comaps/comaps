import SwiftUI
import UIKit

/// Class for accesing SwiftUI views from Objective-C code
@objc class BridgeControllers: NSObject {
    /// The `ProfileView` for presentation in an alert
    @objc static func profileAsAlert() -> UIViewController {
        let profileBridgeController = UIHostingController(rootView: ProfileView(isPresentedAsAlert: true))
        profileBridgeController.view.backgroundColor = .systemGroupedBackground
        return profileBridgeController
    }
    
    
    /// The `MapPositionButton` for presentation in an alert
    @objc static func mapPositionButton() -> UIViewController {
        let mapPositionButtonBridgeController = UIHostingController(rootView: MapPositionButton())
        mapPositionButtonBridgeController.view.isUserInteractionEnabled = true
        mapPositionButtonBridgeController.view.isOpaque = false
        mapPositionButtonBridgeController.view.backgroundColor = .clear
        return mapPositionButtonBridgeController
    }
    
    
    /// The `RoutingOptionsView` for presentation in an alert
    @objc static func routingOptions() -> UIViewController {
        let routinOptionsBridgeController = UIHostingController(rootView: RoutingOptionsView())
        routinOptionsBridgeController.view.backgroundColor = .systemGroupedBackground
        return routinOptionsBridgeController
    }
}



/// Class for using the SwiftUI `MainView` in the interface builder
class MainBridgeController: UIHostingController<MainView> {
    required init?(coder aDecoder: NSCoder) {
        super.init(coder: aDecoder, rootView: MainView())
        view.isOpaque = false
        view.backgroundColor = .clear
    }
}
