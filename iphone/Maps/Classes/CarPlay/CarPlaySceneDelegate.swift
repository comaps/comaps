import CarPlay

@objc(CarPlaySceneDelegate)
final class CarPlaySceneDelegate: UIResponder, CPTemplateApplicationSceneDelegate {

  func templateApplicationScene(
    _ templateApplicationScene: CPTemplateApplicationScene,
    didConnect interfaceController: CPInterfaceController,
    to window: CPWindow
  ) {
    LOG(.info, "[CarPlayHost] app scene didConnect")
    CarPlayService.shared.setup(window: window, interfaceController: interfaceController)
  }

  func templateApplicationScene(
    _ templateApplicationScene: CPTemplateApplicationScene,
    didDisconnect interfaceController: CPInterfaceController,
    from window: CPWindow
  ) {
    LOG(.info, "[CarPlayHost] app scene didDisconnect")
    CarPlayService.shared.appSceneDidDisconnect()
  }

  func sceneDidBecomeActive(_ scene: UIScene) {
    LOG(.info, "[CarPlayHost] app scene sceneDidBecomeActive")
    CarPlayService.shared.appSceneDidBecomeActive()
  }

  func sceneWillEnterForeground(_ scene: UIScene) {
    LOG(.info, "[CarPlayHost] app scene sceneWillEnterForeground")
  }
}
