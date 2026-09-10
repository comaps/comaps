import CarPlay

@objc(CarPlaySceneDelegate)
final class CarPlaySceneDelegate: UIResponder, CPTemplateApplicationSceneDelegate {

  func templateApplicationScene(
    _ templateApplicationScene: CPTemplateApplicationScene,
    didConnect interfaceController: CPInterfaceController,
    to window: CPWindow
  ) {
    LOG(.info, "[CarPlayHost] app scene didConnect")
    CarPlayService.shared.logSceneEvent("didConnect", scene: templateApplicationScene, controller: interfaceController, window: window)
    CarPlayService.shared.diagnosticAppScene = templateApplicationScene
    CarPlayService.shared.setup(window: window, interfaceController: interfaceController)
  }

  func templateApplicationScene(
    _ templateApplicationScene: CPTemplateApplicationScene,
    didDisconnect interfaceController: CPInterfaceController,
    from window: CPWindow
  ) {
    LOG(.info, "[CarPlayHost] app scene didDisconnect")
    CarPlayService.shared.logSceneEvent("didDisconnect", scene: templateApplicationScene, controller: interfaceController, window: window)
    CarPlayService.shared.appSceneDidDisconnect()
  }

  func sceneDidBecomeActive(_ scene: UIScene) {
    CarPlayService.shared.logSceneEvent("sceneDidBecomeActive", scene: scene)
    CarPlayService.shared.scheduleDiagnosticSnapshots(for: scene)
    LOG(.info, "[CarPlayHost] app scene sceneDidBecomeActive")
    CarPlayService.shared.appSceneDidBecomeActive()
  }

  func sceneWillEnterForeground(_ scene: UIScene) {
    CarPlayService.shared.logSceneEvent("sceneWillEnterForeground", scene: scene)
    LOG(.info, "[CarPlayHost] app scene sceneWillEnterForeground")
  }

  func sceneWillResignActive(_ scene: UIScene) {
    CarPlayService.shared.logSceneEvent("sceneWillResignActive", scene: scene)
  }

  func sceneDidEnterBackground(_ scene: UIScene) {
    CarPlayService.shared.logSceneEvent("sceneDidEnterBackground", scene: scene)
  }

  func sceneDidDisconnect(_ scene: UIScene) {
    CarPlayService.shared.logSceneEvent("sceneDidDisconnect", scene: scene)
  }
}
