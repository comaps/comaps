import CarPlay
import Contacts

struct CarPlayPanningInterfaceState {
  private var presentedTemplateIdentifier: ObjectIdentifier?

  var isPresented: Bool {
    presentedTemplateIdentifier != nil
  }

  mutating func didShow(_ template: CPMapTemplate) {
    presentedTemplateIdentifier = ObjectIdentifier(template)
  }

  @discardableResult
  mutating func didDismiss(_ template: CPMapTemplate) -> Bool {
    guard presentedTemplateIdentifier == ObjectIdentifier(template) else { return false }
    presentedTemplateIdentifier = nil
    return true
  }

  mutating func reset() {
    presentedTemplateIdentifier = nil
  }
}

struct CarPlaySearchContextState {
  enum Owner {
    case phone
    case car
  }

  private(set) var owner: Owner = .phone

  mutating func transition(to newOwner: Owner?) -> Bool {
    guard let newOwner, newOwner != owner else { return false }
    owner = newOwner
    return true
  }
}

@objc(MWMCarPlayService)
final class CarPlayService: NSObject {
  @objc static let shared = CarPlayService()
  @objc var isCarplayActivated: Bool = false

  private override init() {
    super.init()
    NotificationCenter.default.addObserver(self,
                                           selector: #selector(applicationDidBecomeActive),
                                           name: UIApplication.didBecomeActiveNotification,
                                           object: nil)
  }
  private var searchService: CarPlaySearchService?
  private var router: CarPlayRouter?
  private var window: CPWindow?
  private var interfaceController: CPInterfaceController?
  private var sessionConfiguration: CPSessionConfiguration?
  var currentPositionMode: MWMMyPositionMode = .pendingPosition
  var isSpeedCamActivated: Bool {
    set {
      router?.updateSpeedCameraMode(newValue ? .always: .never)
    }
    get {
      let mode: SpeedCameraManagerMode = router?.speedCameraMode ?? .never
      return mode == .always ? true : false
    }
  }
  var isKeyboardLimited: Bool {
    return sessionConfiguration?.limitedUserInterfaces.contains(.keyboard) ?? false
  }
  private var carplayVC: CarPlayMapViewController? {
    return window?.rootViewController as? CarPlayMapViewController
  }
  /// Where the single shared map view (EAGLView) is currently parented.
  private enum MapHost {
    case none
    case phone
    case carplay
    case dashboard
  }
  private var mapHost: MapHost = .none
  private var searchContextState = CarPlaySearchContextState()

  private var rootTemplateDidAppear = false
  private var panningInterfaceState = CarPlayPanningInterfaceState()
  private var hasEngagedInitialCarFollow = false
  private var isInitialCarHeadingModeDisabled = false
  private func resetCarSessionDefaults() {
    hasEngagedInitialCarFollow = false
    isInitialCarHeadingModeDisabled = false
    isCarMapViewportReady = false
    isWaitingForCarMapViewport = false
    carMapViewportReadinessAttempts = 0
    hasLoggedViewportExhaustion = false
    needsBaseMapNorthUp = false
    needsRecenterOnViewportReady = false
  }
  private weak var dashboardWindow: UIWindow?
  private var isDashboardActive = false
  private var dashboardVC: CarPlayDashboardMapViewController? {
    return dashboardWindow?.rootViewController as? CarPlayDashboardMapViewController
  }

  private var isPhoneModeRequested: Bool {
    return !isCarplayActivated && savedInterfaceController != nil
  }

  @objc var isHostingMapOnCarScreen: Bool {
    return mapHost == .carplay || mapHost == .dashboard
  }
  private var rootMapTemplate: CPMapTemplate? {
    return interfaceController?.rootTemplate as? CPMapTemplate
  }
  var preparedToPreviewTrips: [CPTrip] = []
  var isUserPanMap: Bool = false
  private var searchText = ""

  private var pendingDashboardBookmark: MWMCarPlayBookmarkObject?
  private var pendingDashboardNavigationTrip: CPTrip?

  @objc func setup(window: CPWindow, interfaceController: CPInterfaceController) {
    if pendingTeardown != nil {
      LOG(.info, "\(CarPlayLogging.carPlay) teardown cancelled reason=reconnect \(diagnosticConnectionContext) gracePeriod=\(Self.kTeardownGracePeriod)s")
    }
    diagnosticConnectionGeneration += 1
    diagnosticActivation += 1
    LOG(.info, "\(CarPlayLogging.carPlay) setup begin")
    pendingTeardown?.cancel()
    pendingTeardown = nil
    endTeardownBackgroundTask()
    let isRebind = isCarplayActivated && router != nil
    isCarplayActivated = true
    self.window = window
    self.interfaceController = interfaceController
    self.interfaceController?.delegate = self
    let configuration = CPSessionConfiguration(delegate: self)
    sessionConfiguration = configuration
    if isRebind {
      LOG(.info, "\(CarPlayLogging.carPlay) setup begin mode=rebind \(diagnosticConnectionContext)")
    } else {
      searchService = CarPlaySearchService()
      let router = CarPlayRouter(displayScale: interfaceController.carTraitCollection.displayScale)
      router.addListener(self)
      router.subscribeToEvents()
      router.setupCarPlaySpeedCameraMode()
      self.router = router
      MWMRouter.unsubscribeFromEvents()
    }
    applyRootViewController()
    if let sessionData = router?.restoredNavigationSession() {
      router?.cancelNavigationSession()
      applyNavigationRootTemplate(trip: sessionData.0, routeInfo: sessionData.1)
    } else {
      applyBaseRootTemplate()
      router?.restoreTripPreviewOnCarplay(beforeRootTemplateDidAppear: true)
    }
    updateContentStyle(configuration.contentStyle)
    applyHostAppearanceIfActive()
    if let bookmark = pendingDashboardBookmark {
      LOG(.info, "\(CarPlayLogging.carPlay) dashboardNavigation begin reason=setupReady")
      pendingDashboardBookmark = nil
      navigateToBookmarkFromDashboard(bookmark: bookmark)
    }
    logStateSnapshot("setup completed")
  }

  private var savedInterfaceController: CPInterfaceController?

  func showOnPhone() {
    defer { logStateSnapshot("showOnPhone completed") }
    LOG(.info, "\(CarPlayLogging.carPlay) showOnPhone begin")
    savedInterfaceController = interfaceController
    switchScreenToPhone()
    showPhoneModeAlert()
  }

  private func showOnCarplay() {
    LOG(.info, "\(CarPlayLogging.carPlay) showOnCarplay begin")
    guard let window, let savedInterfaceController else {
      LOG(.warning, "\(CarPlayLogging.carPlay) showOnCarplay failed window=\(CarPlayLogging.diagnosticIdentity(window)) savedController=\(CarPlayLogging.diagnosticIdentity(savedInterfaceController))")
      return
    }
    setup(window: window, interfaceController: savedInterfaceController)
    LOG(.info, "\(CarPlayLogging.carPlay) showOnCarplay completed \(diagnosticConnectionContext)")
  }

  private func showPhoneModeAlert() {
    let switchToCarAction = CPAlertAction(
      title: L("car_continue_in_the_car"),
      style: .default,
      handler: { [weak self] _ in
        self?.savedInterfaceController?.dismissTemplate(animated: false)
        self?.showOnCarplay()
      }
    )
    let alert = CPAlertTemplate(
      titleVariants: [L("car_used_on_the_phone_screen")],
      actions: [switchToCarAction]
    )
    savedInterfaceController?.dismissTemplate(animated: false)
    savedInterfaceController?.presentTemplate(alert, animated: false)
  }

  private func switchScreenToPhone() {
    defer { logStateSnapshot("switchScreenToPhone completed") }
    router?.removeListener(self)
    router?.unsubscribeFromEvents()
    router?.setupInitialSpeedCameraMode()
    MWMRouter.subscribeToEvents()
    isCarplayActivated = false
    if router?.currentTrip != nil {
      MWMRouter.showNavigationMapControls()
    } else if router?.previewTrip != nil {
      MWMRouter.rebuild(withBestRouter: true)
    }
    router?.cancelNavigationSession()
    searchService = nil
    router = nil
    sessionConfiguration = nil
    interfaceController = nil
    pendingDashboardBookmark = nil
    pendingDashboardNavigationTrip = nil
    // Apply the visual-scale change (and its GPU context reset) before the theme switch,
    // so the context teardown doesn't race with an in-flight route recache from the style change.
    updateMapHost()
    ThemeManager.invalidate()
  }

  private var pendingTeardown: DispatchWorkItem?
  private var teardownBackgroundTask: UIBackgroundTaskIdentifier = .invalid
  private static let kTeardownGracePeriod: TimeInterval = 2.0

  private enum TeardownTrigger {
    case gracePeriodElapsed(requestedConnection: Int)
    case expired
  }

  private func teardownIfDisconnected(_ trigger: TeardownTrigger) {
    // Flaky USB, wireless CarPlay connections, and quick scene changes that pass
    // the drape engine between map hosts can cause crashes and UI issues
    // Use a 2 second grace period to avoid this
    // Once multiple drape engines are supported, this whole CarPlay implementation should be simplified
    
    if case .gracePeriodElapsed = trigger {
      pendingTeardown = nil
    }
    guard interfaceController == nil else {
      if case .gracePeriodElapsed(let requestedConnection) = trigger {
        LOG(.info, "\(CarPlayLogging.carPlay) teardown cancelled reason=controllerAvailable requestedConnection=\(requestedConnection) \(diagnosticConnectionContext) gracePeriod=\(Self.kTeardownGracePeriod)s")
      }
      endTeardownBackgroundTask()
      return
    }

    let reason: String
    switch trigger {
    case .gracePeriodElapsed(let requestedConnection):
      LOG(.info, "\(CarPlayLogging.carPlay) teardown executed requestedConnection=\(requestedConnection) \(diagnosticConnectionContext) gracePeriod=\(Self.kTeardownGracePeriod)s")
      reason = "gracePeriodElapsed"
    case .expired:
      LOG(.warning, "\(CarPlayLogging.carPlay) teardown expired \(diagnosticConnectionContext) gracePeriod=\(Self.kTeardownGracePeriod)s; tearing down immediately")
      pendingTeardown?.cancel()
      pendingTeardown = nil
      reason = "expired"
    }
    destroy()
    window = nil
    logStateSnapshot("teardown completed reason=\(reason)")
  }

  private func beginTeardownBackgroundTask() {
    endTeardownBackgroundTask()
    teardownBackgroundTask = UIApplication.shared.beginBackgroundTask(withName: "CarPlay scene teardown") { [weak self] in
      self?.teardownIfDisconnected(.expired)
    }
  }

  private func endTeardownBackgroundTask() {
    guard teardownBackgroundTask != .invalid else { return }
    let task = teardownBackgroundTask
    teardownBackgroundTask = .invalid
    UIApplication.shared.endBackgroundTask(task)
  }

  func appSceneDidDisconnect() {
    diagnosticActivation += 1
    defer { logStateSnapshot("appSceneDidDisconnect completed") }
    diagnosticAppScene = nil
    interfaceController?.delegate = nil
    interfaceController = nil
    sessionConfiguration = nil
    if isDashboardActive {
      updateMapHost()
    }
    beginTeardownBackgroundTask()
    let disconnectedConnection = diagnosticConnectionGeneration
    let teardown = DispatchWorkItem { [weak self] in
      self?.teardownIfDisconnected(.gracePeriodElapsed(requestedConnection: disconnectedConnection))
    }
    if pendingTeardown != nil {
      LOG(.info, "\(CarPlayLogging.carPlay) teardown cancelled reason=rescheduled \(diagnosticConnectionContext) gracePeriod=\(Self.kTeardownGracePeriod)s")
    }
    pendingTeardown?.cancel()
    pendingTeardown = teardown
    LOG(.info, "\(CarPlayLogging.carPlay) teardown scheduled \(diagnosticConnectionContext) gracePeriod=\(Self.kTeardownGracePeriod)s")
    DispatchQueue.main.asyncAfter(deadline: .now() + Self.kTeardownGracePeriod, execute: teardown)
  }

  @objc func destroy() {
    diagnosticActivation += 1
    defer { logStateSnapshot("destroy completed") }
    if pendingTeardown != nil {
      LOG(.info, "\(CarPlayLogging.carPlay) teardown cancelled reason=destroy \(diagnosticConnectionContext) gracePeriod=\(Self.kTeardownGracePeriod)s")
    }
    panningInterfaceState.reset()
    pendingTeardown?.cancel()
    pendingTeardown = nil
    endTeardownBackgroundTask()
    pendingDashboardNavigationTrip = nil
    if isCarplayActivated {
      switchScreenToPhone()
    }
    savedInterfaceController = nil
    resetCarSessionDefaults()
    LocationManager.refreshBackgroundLocationPolicy()
  }

  @objc func interfaceStyle() -> UIUserInterfaceStyle {
    if let window = window,
      window.traitCollection.userInterfaceIdiom == .carPlay {
      return rootTemplateStyle == .dark ? .dark : .light
    }
    return .unspecified
  }

  @available(iOS 13.0, *)
  private func updateContentStyle(_ contentStyle: CPContentStyle) {
    rootTemplateStyle = contentStyle == .dark ? .dark : .light
    // Update the current map style in accordance with the CarPLay content theme.
    ThemeManager.invalidate()
  }

  private var rootTemplateStyle: CPTripEstimateStyle = .light {
    didSet {
      (interfaceController?.rootTemplate as? CPMapTemplate)?.tripEstimateStyle = rootTemplateStyle
    }
  }

  // MARK: - Diagnostics

  private var diagnosticConnectionGeneration = 0
  private var diagnosticRootRequest = 0
  private var diagnosticActivation = 0
  weak var diagnosticAppScene: CPTemplateApplicationScene?

  var diagnosticConnectionContext: String {
    "connection=\(diagnosticConnectionGeneration) controller=\(CarPlayLogging.diagnosticIdentity(interfaceController))"
  }

  @objc(logSceneEvent:scene:controller:window:)
  func logSceneEvent(_ event: String, scene: UIScene, controller: AnyObject? = nil, window: UIWindow? = nil) {
    guard Logger.canLog(.info) else { return }
    assert(Thread.isMainThread)
    let matchesController = (controller as? CPInterfaceController).map { $0 === interfaceController }
    LOG(.info, "\(CarPlayLogging.scene) \(event) received: id=\(scene.session.persistentIdentifier) scene=\(CarPlayLogging.diagnosticIdentity(scene)) role=\(CarPlayLogging.sceneRole(scene.session.role)) state=\(CarPlayLogging.sceneState(scene.activationState)) callbackController=\(CarPlayLogging.diagnosticIdentity(controller)) callbackWindow=\(CarPlayLogging.diagnosticIdentity(window)) matchesCurrentController=\(String(describing: matchesController)) \(diagnosticConnectionContext)")
  }

  func scheduleDiagnosticSnapshots(for scene: UIScene) {
    diagnosticActivation += 1
    guard Logger.canLog(.info) else { return }
    let activation = diagnosticActivation
    let connection = diagnosticConnectionGeneration
    for delay in [0.3, 1.0, 3.0] {
      DispatchQueue.main.asyncAfter(deadline: .now() + delay) { [weak self, weak scene] in
        guard let self, let scene,
              self.diagnosticActivation == activation,
              self.diagnosticConnectionGeneration == connection,
              scene.activationState == .foregroundActive,
              self.diagnosticAppScene === scene,
              self.interfaceController != nil else { return }
        self.logStateSnapshot("activation checkpoint delay=\(delay)s")
      }
    }
  }

  private func logTemplateEvent(_ event: String, template: CPTemplate, animated: Bool) {
    LOG(.info, "\(CarPlayLogging.carPlay) \(event) received callbackTemplate=\(CarPlayLogging.diagnosticTemplate(template)) isRoot=\(template === interfaceController?.rootTemplate) animated=\(animated) \(diagnosticConnectionContext)")
  }

  func logStateSnapshot(_ reason: @autoclosure () -> String, level: LogLevel = .info) {
    guard Logger.canLog(level) else { return }
    assert(Thread.isMainThread)
    var hosting = "mapHost=\(mapHost) mapViewLoaded=false"
    if let mapVC = MapViewController.shared(), mapVC.isViewLoaded {
      hosting = "mapHost=\(mapHost) mapViewLoaded=true mapView=\(CarPlayLogging.diagnosticIdentity(mapVC.mapView)) parent=\(CarPlayLogging.diagnosticIdentity(mapVC.mapView.superview)) attachedWindow=\(CarPlayLogging.diagnosticIdentity(mapVC.mapView.window))"
    }
    let scenes = UIApplication.shared.connectedScenes
      .map { "\(CarPlayLogging.sceneRole($0.session.role)):\(CarPlayLogging.sceneState($0.activationState)):\($0.session.persistentIdentifier)" }
      .sorted()
      .joined(separator: " ")
    let state = [
      diagnosticConnectionContext,
      "activation=\(diagnosticActivation)",
      "latestRequest=\(diagnosticRootRequest)",
      "activated=\(isCarplayActivated)",
      "phoneModeRequested=\(isPhoneModeRequested)",
      "dashboardActive=\(isDashboardActive)",
      "savedController=\(CarPlayLogging.diagnosticIdentity(savedInterfaceController))",
      "delegate=\(CarPlayLogging.diagnosticIdentity(interfaceController?.delegate))",
      "rootTemplate=\(CarPlayLogging.diagnosticTemplate(interfaceController?.rootTemplate))",
      "rootDidAppear=\(rootTemplateDidAppear)",
      "topTemplate=\(CarPlayLogging.diagnosticTemplate(interfaceController?.topTemplate))",
      "presentedTemplate=\(CarPlayLogging.diagnosticTemplate(interfaceController?.presentedTemplate))",
      "carWindow=\(CarPlayLogging.diagnosticIdentity(window))",
      "carScene=\(diagnosticAppScene?.session.persistentIdentifier ?? "nil")",
      "rootVC=\(CarPlayLogging.diagnosticIdentity(window?.rootViewController))",
      "dashboardWindow=\(CarPlayLogging.diagnosticIdentity(dashboardWindow))",
      "trip=\(CarPlayLogging.diagnosticIdentity(router?.currentTrip))",
      "preview=\(CarPlayLogging.diagnosticIdentity(router?.previewTrip))",
      "navigation={\(router?.diagnosticNavigationContext ?? "session=nil origin=none")}",
      "pendingTeardown=\(pendingTeardown != nil)",
      "teardownBackgroundTask=\(teardownBackgroundTask != .invalid)",
      "pendingDashboardBookmark=\(pendingDashboardBookmark != nil)",
      "pendingDashboardTrip=\(CarPlayLogging.diagnosticIdentity(pendingDashboardNavigationTrip))",
      "preparedPreviewCount=\(preparedToPreviewTrips.count)",
      "appearanceDeferred=\(needsHostAppearanceRefresh)",
      "viewportReady=\(isCarMapViewportReady)",
      "viewportWaiting=\(isWaitingForCarMapViewport)",
      "viewportAttempts=\(carMapViewportReadinessAttempts)",
      "pendingRecenter=\(needsRecenterOnViewportReady)",
      "pendingNorthUp=\(needsBaseMapNorthUp)"
    ].joined(separator: " ")
    LOG(level, "\(CarPlayLogging.carPlay) \(reason()): \(hosting) appState=\(CarPlayLogging.appState(UIApplication.shared.applicationState)) scenes=[\(scenes)] \(state)")
  }

  private func setRootTemplate(_ template: CPMapTemplate, reason: String) {
    rootTemplateDidAppear = false
    panningInterfaceState.reset()
    diagnosticRootRequest += 1
    let request = diagnosticRootRequest
    let connection = diagnosticConnectionGeneration
    let controller = interfaceController
    let requestedController = CarPlayLogging.diagnosticIdentity(controller)
    let requestedTemplate = CarPlayLogging.diagnosticTemplate(template)
    let started = ProcessInfo.processInfo.systemUptime
    logStateSnapshot("setRootTemplate begin request=\(request) requestedConnection=\(connection) reason=\(reason) requestedTemplate=\(requestedTemplate)")
    if controller == nil {
      logStateSnapshot("setRootTemplate failed request=\(request) reason=missingController", level: .warning)
    }
    controller?.setRootTemplate(template, animated: false) { [weak controller] success, error in
      let level: LogLevel = success ? .info : .warning
      guard Logger.canLog(level) else { return }
      let elapsed = ProcessInfo.processInfo.systemUptime - started
      let logCompletion = { [weak controller] in
        let matchesCurrentController = controller != nil && controller === self.interfaceController
        let isCurrentConnection = connection == self.diagnosticConnectionGeneration && matchesCurrentController
        self.logStateSnapshot("setRootTemplate \(success ? "completed" : "failed") request=\(request) requestedConnection=\(connection) currentConnection=\(self.diagnosticConnectionGeneration) requestedController=\(requestedController) requestedTemplate=\(requestedTemplate) reason=\(reason) elapsed=\(elapsed)s matchesCurrentController=\(matchesCurrentController) isLatestRequest=\(request == self.diagnosticRootRequest) stale=\(!isCurrentConnection || request != self.diagnosticRootRequest) success=\(success) error=\(String(describing: error))", level: level)
      }
      if Thread.isMainThread {
        logCompletion()
      } else {
        DispatchQueue.main.async(execute: logCompletion)
      }
    }
  }

  func appSceneDidBecomeActive() {
    defer { logStateSnapshot("appSceneDidBecomeActive completed") }
    reconcileMapHostIfOrphaned()
    resumeLocationForActiveCarSceneIfNeeded()
    engageCarFollowIfNeeded(currentPositionMode, allowReengagement: true)
    guard isCarplayActivated, let controller = interfaceController else { return }
    if rootTemplateDidAppear {
      return
    }
    DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) { [weak self] in
      guard let self,
        self.isCarplayActivated,
        self.interfaceController === controller,
        !self.rootTemplateDidAppear else { return }
      self.reconcileUnpresentedRootTemplate()
    }
  }

  private func reconcileUnpresentedRootTemplate() {
    guard let router else { return }
    LOG(.warning, "\(CarPlayLogging.carPlay) rootPresentation failed reason=missingAppearance; recovery begin")
    defer { logStateSnapshot("rootPresentation recovery handling completed") }
    if let sessionData = router.restoredNavigationSession() {
      router.cancelNavigationSession()
      applyNavigationRootTemplate(trip: sessionData.0, routeInfo: sessionData.1)
    } else {
      applyBaseRootTemplate()
    }
  }

  private func applyRootViewController() {
    guard let window = window else { return }
    let wasHostingMapOnCarScreen = isHostingMapOnCarScreen
    let carplaySotyboard = UIStoryboard.instance(.carPlay)
    let carplayVC = carplaySotyboard.instantiateInitialViewController() as! CarPlayMapViewController
    window.rootViewController = carplayVC
    if mapHost == .carplay {
      mapHost = .none
    }
    updateMapHost()
    refreshLocationPolicyIfHostingChanged(from: wasHostingMapOnCarScreen, reason: "applyRootViewController")
  }

  @objc func attachMapIfNeeded() {
    updateMapHost()
  }

  // MARK: - Map host management

  private static let kCarPositionArrowOffset: Int32 = 120

  private func updateMapHost() {
    let wasHostingMapOnCarScreen = isHostingMapOnCarScreen
    var desired: MapHost
    if isDashboardActive, dashboardVC != nil, !isPhoneModeRequested {
      desired = .dashboard
    } else if isCarplayActivated, window != nil, carplayVC != nil {
      desired = .carplay
    } else if MapsAppDelegate.theApp().window != nil {
      desired = .phone
    } else if mapHost == .carplay || mapHost == .dashboard {
      desired = .none
    } else {
      return
    }
    guard desired != mapHost else { return }
    isCarMapViewportReady = false
    isWaitingForCarMapViewport = false
    carMapViewportReadinessAttempts = 0
    hasLoggedViewportExhaustion = false

    MapsAppDelegate.theApp().ensureMapNavigationController()
    guard let mapVC = MapViewController.shared() else {
      LOG(.warning, "\(CarPlayLogging.carPlay) mapHost failed requestedHost=\(desired) reason=missingMapViewController")
      return
    }

    var attachToCarScreen: (() -> Void)?
    switch desired {
    case .carplay:
      if let window, let carplayVC {
        attachToCarScreen = { [self] in
          attachMapToCarScreen(mapVC) {
            carplayVC.addMapView(mapVC.mapView, mapButtonSafeAreaLayoutGuide: window.mapButtonSafeAreaLayoutGuide)
          }
        }
      }
    case .dashboard:
      if let dashboardWindow, let dashboardVC {
        attachToCarScreen = { [self] in
          attachMapToCarScreen(mapVC) {
            dashboardVC.addMapView(mapVC.mapView)
          }
        }
      }
    case .phone, .none:
      break
    }
    if attachToCarScreen == nil, desired == .carplay || desired == .dashboard {
      LOG(.warning, "\(CarPlayLogging.carPlay) mapHost failed requestedHost=\(desired) reason=hostVanished; fallback begin")
      desired = MapsAppDelegate.theApp().window != nil ? .phone : .none
      guard desired != mapHost else { return }
    }
    LOG(.info, "\(CarPlayLogging.carPlay) mapHost begin from=\(mapHost) to=\(desired) \(diagnosticConnectionContext)")
    updateSearchContext(for: desired)

    switch mapHost {
    case .carplay:
      carplayVC?.removeMapView()
    case .dashboard:
      dashboardVC?.removeMapView()
    case .phone, .none:
      break
    }

    var appearanceChanged = true
    if let attachToCarScreen {
      attachToCarScreen()
    } else {
      let wasHostedElsewhere = mapHost != .none || (mapVC.isViewLoaded && mapVC.mapView.superview == nil)
      if wasHostedElsewhere {
        mapVC.disableCarPlayRepresentation()
        mapVC.remove(self)
      } else {
        appearanceChanged = false
      }
    }
    mapHost = desired
    if appearanceChanged {
      applyHostAppearanceIfActive()
    }
    if desired == .phone || desired == .none {
      resetCarSessionDefaults()
    }
    engageCarFollowIfNeeded(currentPositionMode)
    logStateSnapshot("mapHost completed")
    refreshLocationPolicyIfHostingChanged(from: wasHostingMapOnCarScreen, reason: "updateMapHost")
  }

  private func updateSearchContext(for host: MapHost) {
    let owner: CarPlaySearchContextState.Owner?
    switch host {
    case .carplay, .dashboard:
      owner = .car
    case .phone:
      owner = .phone
    case .none:
      owner = nil
    }
    let previousOwner = searchContextState.owner
    guard searchContextState.transition(to: owner) else { return }
    LOG(.info, "\(CarPlayLogging.carPlay) searchContext switch from=\(previousOwner) to=\(searchContextState.owner)")
    Search.clear()
  }

  private func reconcileMapHostIfOrphaned() {
    guard let mapVC = MapViewController.shared(),
          mapVC.isViewLoaded,
          mapVC.mapView.window == nil,
          mapHost != .none else { return }
    let wasHostingMapOnCarScreen = isHostingMapOnCarScreen
    LOG(.warning, "\(CarPlayLogging.carPlay) mapHost failed host=\(mapHost) reason=missingWindow; recovery begin")
    mapHost = .none
    updateMapHost()
    refreshLocationPolicyIfHostingChanged(from: wasHostingMapOnCarScreen, reason: "reconcileMapHostIfOrphaned")
  }

  private func refreshLocationPolicyIfHostingChanged(from wasHostingMapOnCarScreen: Bool, reason: String) {
    guard wasHostingMapOnCarScreen != isHostingMapOnCarScreen else { return }
    LOG(.info, "\(CarPlayLogging.carPlay) locationPolicy begin carHosting=\(wasHostingMapOnCarScreen)->\(isHostingMapOnCarScreen) reason=\(reason)")
    LocationManager.refreshBackgroundLocationPolicy()
  }

  private func resumeLocationForActiveCarSceneIfNeeded() {
    guard LocationManager.shouldKeepRunningInBackground() else {
      LocationManager.refreshBackgroundLocationPolicy()
      return
    }
    LocationManager.applicationDidBecomeActive()
  }

  private func attachMapToCarScreen(_ mapVC: MapViewController, addMapView: () -> Void) {
    currentPositionMode = mapVC.currentPositionMode
    mapVC.enableCarPlayRepresentation()
    addMapView()
    mapVC.add(self)
    refreshMyPositionModeButton()
  }

  private var needsHostAppearanceRefresh = false

  private func applyHostAppearanceIfActive() {
    guard UIApplication.shared.applicationState != .background else {
      if !needsHostAppearanceRefresh {
        LOG(.info, "\(CarPlayLogging.carPlay) hostAppearance deferred host=\(mapHost) reason=appBackgrounded")
      }
      needsHostAppearanceRefresh = true
      return
    }
    let wasDeferred = needsHostAppearanceRefresh
    needsHostAppearanceRefresh = false
    defer {
      if wasDeferred { logStateSnapshot("hostAppearance completed reason=appActive") }
    }
    switch mapHost {
    case .carplay, .dashboard:
      FrameworkHelper.setCarScreenMode(true)
      FrameworkHelper.updatePositionArrowOffset(false, offset: Self.kCarPositionArrowOffset)
      if let carWindow = mapHost == .carplay ? window : dashboardWindow {
        CarPlayWindowScaleAdjuster.updateAppearance(toWindow: carWindow, isCarplayActivated: true)
      }
    case .phone, .none:
      FrameworkHelper.updatePositionArrowOffset(true, offset: 0)
      FrameworkHelper.setCarScreenMode(false)
      CarPlayWindowScaleAdjuster.updateAppearance(toWindow: nil, isCarplayActivated: false)
    }
  }

  @objc private func applicationDidBecomeActive() {
    guard needsHostAppearanceRefresh else { return }
    DispatchQueue.main.async { [weak self] in
      guard let self, self.needsHostAppearanceRefresh else { return }
      LOG(.info, "\(CarPlayLogging.carPlay) hostAppearance begin host=\(self.mapHost) reason=appActive")
      self.applyHostAppearanceIfActive()
    }
  }

  /// Re-sync the car map button glyph after re-attach so it doesn't show a stale position mode
  private func refreshMyPositionModeButton() {
    guard let rootMapTemplate else { return }
    MapTemplateBuilder.updateMyPositionModeButton(mapTemplate: rootMapTemplate)
  }

  private var isCarMapViewportReady = false
  private var isWaitingForCarMapViewport = false
  private var carMapViewportReadinessAttempts = 0
  private var hasLoggedViewportExhaustion = false
  private var needsBaseMapNorthUp = false
  private var needsRecenterOnViewportReady = false

  private func engageCarFollowIfNeeded(_ mode: MWMMyPositionMode, allowReengagement: Bool = false) {
    guard (!hasEngagedInitialCarFollow || allowReengagement),
          mapHost == .carplay || mapHost == .dashboard,
          !MWMRouter.isRoutingActive(),
          !panningInterfaceState.isPresented
    else { return }
    guard isCarMapViewportReady else {
      if allowReengagement {
        needsRecenterOnViewportReady = true
      }
      return
    }
    switch mode {
    case .notFollow:
      FrameworkHelper.switchMyPositionMode()
    case .follow:
      hasEngagedInitialCarFollow = true
      if !isInitialCarHeadingModeDisabled {
        FrameworkHelper.switchMyPositionMode()
      }
    case .followAndRotate:
      hasEngagedInitialCarFollow = true
    case .pendingPosition, .notFollowNoPosition:
      if mode == .notFollowNoPosition {
        FrameworkHelper.switchMyPositionMode()
      }
    }
  }

  func mapViewportDidBecomeReady(_ mapView: EAGLView) {
    guard !MapsAppDelegate.isTestsEnvironment() else { return }
    guard isHostingMapOnCarScreen,
          mapView === MapViewController.shared()?.mapView,
          mapView.window != nil else {
      isWaitingForCarMapViewport = false
      return
    }
    guard mapView.graphicContextInitialized else {
      guard !isWaitingForCarMapViewport else { return }
      guard carMapViewportReadinessAttempts < 100 else {
        if !hasLoggedViewportExhaustion {
          hasLoggedViewportExhaustion = true
          logStateSnapshot("viewport failed reason=graphicsContextTimeout; map defaults deferred until next layout", level: .warning)
        }
        return
      }
      carMapViewportReadinessAttempts += 1
      isWaitingForCarMapViewport = true
      DispatchQueue.main.asyncAfter(deadline: .now() + 0.05) { [weak self, weak mapView] in
        guard let self, let mapView else { return }
        self.isWaitingForCarMapViewport = false
        self.mapViewportDidBecomeReady(mapView)
      }
      return
    }

    isWaitingForCarMapViewport = false
    carMapViewportReadinessAttempts = 0
    hasLoggedViewportExhaustion = false
    let becameReady = !isCarMapViewportReady
    isCarMapViewportReady = true
    defer {
      if becameReady { logStateSnapshot("viewport completed") }
    }
    if needsBaseMapNorthUp {
      needsBaseMapNorthUp = false
      FrameworkHelper.rotateMap(0.0, animated: false)
    }
    let allowReengagement = needsRecenterOnViewportReady
    needsRecenterOnViewportReady = false
    engageCarFollowIfNeeded(currentPositionMode, allowReengagement: allowReengagement)
  }

  func switchMyPositionModeFromCarPlayControl() {
    isInitialCarHeadingModeDisabled = true
    FrameworkHelper.switchMyPositionMode()
  }

  // MARK: - Dashboard scene

  @objc func dashboardConnected(window: UIWindow) {
    defer { logStateSnapshot("dashboardConnected completed") }
    dashboardWindow = window
    window.rootViewController = CarPlayDashboardMapViewController()
  }

  @objc func dashboardDisconnected() {
    defer { logStateSnapshot("dashboardDisconnected completed") }
    let wasHostingMapOnCarScreen = isHostingMapOnCarScreen
    if mapHost == .dashboard {
      dashboardVC?.removeMapView()
      mapHost = .none
    }
    dashboardWindow = nil
    isDashboardActive = false
    updateMapHost()
    refreshLocationPolicyIfHostingChanged(from: wasHostingMapOnCarScreen, reason: "dashboardDisconnected")
  }

  @objc func dashboardDidBecomeActive() {
    defer { logStateSnapshot("dashboardDidBecomeActive completed") }
    isDashboardActive = true
    reconcileMapHostIfOrphaned()
    updateMapHost()
    resumeLocationForActiveCarSceneIfNeeded()
    engageCarFollowIfNeeded(currentPositionMode, allowReengagement: true)
  }

  @objc func dashboardDidResignActive() {
    defer { logStateSnapshot("dashboardDidResignActive completed") }
    isDashboardActive = false
    updateMapHost()
  }

  private func applyBaseRootTemplate(reason: String = #function) {
    let mapTemplate = MapTemplateBuilder.buildBaseTemplate(positionMode: currentPositionMode)
    mapTemplate.mapDelegate = self
    mapTemplate.tripEstimateStyle = rootTemplateStyle
    setRootTemplate(mapTemplate, reason: reason)
    needsBaseMapNorthUp = true
    if let mapView = MapViewController.shared()?.mapView {
      mapViewportDidBecomeReady(mapView)
    }
  }

  private func applyNavigationRootTemplate(trip: CPTrip, routeInfo: RouteInfo, reason: String = #function) {
    let mapTemplate = MapTemplateBuilder.buildNavigationTemplate()
    needsBaseMapNorthUp = false
    mapTemplate.mapDelegate = self
    setRootTemplate(mapTemplate, reason: reason)
    router?.startNavigationSession(forTrip: trip, template: mapTemplate)
    if let estimates = createEstimates(routeInfo: routeInfo) {
      mapTemplate.tripEstimateStyle = rootTemplateStyle
      mapTemplate.updateEstimates(estimates, for: trip)
    }

    if let carplayVC = carplayVC {
      carplayVC.updateCurrentSpeed(routeInfo.speedMps, speedLimitMps: routeInfo.speedLimitMps)
      carplayVC.showSpeedControl()
    }
  }

  func navigateToBookmarkFromDashboard(bookmark: MWMCarPlayBookmarkObject) {
    defer { logStateSnapshot("dashboardNavigation handling completed") }
    LOG(.info, "\(CarPlayLogging.carPlay) dashboardNavigation received bookmark=\(bookmark.bookmarkId) router=\(router != nil) interfaceController=\(interfaceController != nil)")
    guard let router = router, interfaceController != nil else {
      if !isPhoneModeRequested {
        LOG(.info, "\(CarPlayLogging.carPlay) dashboardNavigation deferred reason=appSceneNotReady")
        pendingDashboardBookmark = bookmark
      } else {
        LOG(.info, "\(CarPlayLogging.carPlay) dashboardNavigation cancelled reason=phoneModeRequested")
      }
      return
    }
    guard let startPoint = MWMRoutePoint(lastLocationAndType: .start, intermediateIndex: 0),
          let endPoint = MWMRoutePoint(cgPoint: bookmark.mercatorPoint,
                                       title: bookmark.prefferedName,
                                       subtitle: bookmark.address,
                                       type: .finish,
                                       intermediateIndex: 0) else {
      LOG(.warning, "\(CarPlayLogging.carPlay) dashboardNavigation failed reason=noPositionFix")
      return
    }
    if router.currentTrip != nil {
      cancelCurrentTrip()
    }
    let trip = router.createTrip(startPoint: startPoint, endPoint: endPoint)
    pendingDashboardNavigationTrip = trip
    LOG(.info, "\(CarPlayLogging.carPlay) dashboardNavigation begin bookmark=\(bookmark.bookmarkId)")
    router.buildRoute(trip: trip)
  }

  func pushTemplate(_ templateToPush: CPTemplate, animated: Bool) {
    if let interfaceController = interfaceController {
      switch templateToPush {
      case let list as CPListTemplate:
        list.delegate = self
      case let search as CPSearchTemplate:
        search.delegate = self
      case let map as CPMapTemplate:
        map.mapDelegate = self
      default:
        break
      }
      interfaceController.pushTemplate(templateToPush, animated: animated)
    }
  }

  func popTemplate(animated: Bool) {
    interfaceController?.popTemplate(animated: animated)
  }

  func presentAlert(_ template: CPAlertTemplate, animated: Bool) {
    interfaceController?.dismissTemplate(animated: false)
    interfaceController?.presentTemplate(template, animated: animated)
  }

  func cancelCurrentTrip() {
    defer { logStateSnapshot("cancelCurrentTrip completed") }
    LOG(.info, "\(CarPlayLogging.carPlay) cancelCurrentTrip begin")
    pendingDashboardNavigationTrip = nil
    router?.cancelTrip()
    if let carplayVC = carplayVC {
      carplayVC.hideSpeedControl()
    }
    updateMapTemplateUIToBase()
  }

  func updateCameraUI(isCameraOnRoute: Bool, speedLimitMps limit: Double?) {
    if let carplayVC = carplayVC {
      carplayVC.updateCameraInfo(isCameraOnRoute: isCameraOnRoute, speedLimitMps: limit)
    }
  }

  func updateMapTemplateUIToBase() {
    guard let mapTemplate = rootMapTemplate else {
        return
    }
    MapTemplateBuilder.configureBaseUI(mapTemplate: mapTemplate)
    if currentPositionMode == .pendingPosition {
      mapTemplate.leadingNavigationBarButtons = []
    } else if currentPositionMode == .follow || currentPositionMode == .followAndRotate {
      MapTemplateBuilder.setupDestinationButton(mapTemplate: mapTemplate)
    } else {
      MapTemplateBuilder.setupRecenterButton(mapTemplate: mapTemplate)
    }
    updateVisibleViewPortState(.default)
    FrameworkHelper.rotateMap(0.0, animated: true)
  }

  func updateMapTemplateUIToTripFinished(_ trip: CPTrip) {
    guard let mapTemplate = rootMapTemplate else {
        return
    }
    LOG(.info, "\(CarPlayLogging.carPlay) arrivalPresentation begin appState=\(CarPlayLogging.appState(UIApplication.shared.applicationState))")
    updateMapTemplateUIToBase()
    let doneAction = CPAlertAction(title: L("done"), style: .default) { [unowned self] _ in
      self.updateMapTemplateUIToBase()
    }
    var subtitle = ""
    if let locationName = trip.destination.name {
      subtitle = locationName
    }
    if let address = trip.destination.placemark.postalAddress?.street {
      subtitle = subtitle + "\n" + address
    }

    let alert = CPNavigationAlert(titleVariants: [L("trip_finished")],
                                  subtitleVariants: [subtitle],
                                  image: nil,
                                  primaryAction: doneAction,
                                  secondaryAction: nil,
                                  duration: 0)
    mapTemplate.present(navigationAlert: alert, animated: true)
  }

  func updateVisibleViewPortState(_ state: CPViewPortState) {
    guard let carplayVC = carplayVC else {
      return
    }
    carplayVC.updateVisibleViewPortState(state)
  }

  func updateRouteAfterChangingSettings() {
    router?.rebuildRoute()
  }

  @objc func showNoMapAlert() {
    guard let mapTemplate = interfaceController?.topTemplate as? CPMapTemplate,
      let info = mapTemplate.userInfo as? MapInfo,
      info.type == CPConstants.TemplateType.main else {
      return
    }
    let alert = CPAlertTemplate(titleVariants: [L("download_map_carplay")], actions: [])
    alert.userInfo = [CPConstants.TemplateKey.alert: CPConstants.TemplateType.downloadMap]
    presentAlert(alert, animated: true)
  }

  @objc func hideNoMapAlert() {
    if let presentedTemplate = interfaceController?.presentedTemplate,
      let info = presentedTemplate.userInfo as? [String: String],
      let alertType = info[CPConstants.TemplateKey.alert],
      alertType == CPConstants.TemplateType.downloadMap {
      interfaceController?.dismissTemplate(animated: true)
    }
  }
}

// MARK: - CPInterfaceControllerDelegate implementation
extension CarPlayService: CPInterfaceControllerDelegate {
  func templateWillAppear(_ aTemplate: CPTemplate, animated: Bool) {
    logTemplateEvent("templateWillAppear", template: aTemplate, animated: animated)
    defer { logStateSnapshot("templateWillAppear completed callbackTemplate=\(CarPlayLogging.diagnosticTemplate(aTemplate))") }
    guard let info = aTemplate.userInfo as? MapInfo else {
        return
    }
    switch info.type {
    case CPConstants.TemplateType.main:
      updateVisibleViewPortState(.default)
    case CPConstants.TemplateType.preview:
      updateVisibleViewPortState(.preview)
    case CPConstants.TemplateType.navigation:
      updateVisibleViewPortState(.navigation)
    case CPConstants.TemplateType.previewSettings:
      aTemplate.userInfo = MapInfo(type: CPConstants.TemplateType.preview)
    default:
      break
    }
  }

  func templateDidAppear(_ aTemplate: CPTemplate, animated: Bool) {
    logTemplateEvent("templateDidAppear", template: aTemplate, animated: animated)
    defer { logStateSnapshot("templateDidAppear completed callbackTemplate=\(CarPlayLogging.diagnosticTemplate(aTemplate))") }
    guard let mapTemplate = aTemplate as? CPMapTemplate,
      let info = aTemplate.userInfo as? MapInfo else {
        return
    }
    if mapTemplate === rootMapTemplate {
      rootTemplateDidAppear = true
    }
    if !preparedToPreviewTrips.isEmpty && info.type == CPConstants.TemplateType.main {
      preparePreview(trips: preparedToPreviewTrips)
      preparedToPreviewTrips = []
      return
    }

    if info.type == CPConstants.TemplateType.main,
      router?.currentTrip == nil,
      mapTemplate.mapButtons.isEmpty {
      LOG(.warning, "\(CarPlayLogging.carPlay) templateUI failed reason=missingMapButtons; recovery begin")
      updateMapTemplateUIToBase()
      return
    }

    if info.type == CPConstants.TemplateType.preview, let trips = info.trips {
      showPreview(mapTemplate: mapTemplate, trips: trips)
    }
  }

  func templateWillDisappear(_ aTemplate: CPTemplate, animated: Bool) {
    logTemplateEvent("templateWillDisappear", template: aTemplate, animated: animated)
    defer { logStateSnapshot("templateWillDisappear completed callbackTemplate=\(CarPlayLogging.diagnosticTemplate(aTemplate))") }
    guard let info = aTemplate.userInfo as? MapInfo else {
        return
    }
    if info.type == CPConstants.TemplateType.preview {
      router?.completeRouteAndRemovePoints()
    }
  }

  func templateDidDisappear(_ aTemplate: CPTemplate, animated: Bool) {
    logTemplateEvent("templateDidDisappear", template: aTemplate, animated: animated)
    defer { logStateSnapshot("templateDidDisappear completed callbackTemplate=\(CarPlayLogging.diagnosticTemplate(aTemplate))") }
    guard !preparedToPreviewTrips.isEmpty,
      let info = aTemplate.userInfo as? [String: String],
      let alertType = info[CPConstants.TemplateKey.alert],
      alertType == CPConstants.TemplateType.redirectRoute ||
        alertType == CPConstants.TemplateType.restoreRoute else {
        return
    }
    preparePreview(trips: preparedToPreviewTrips)
    preparedToPreviewTrips = []
  }
}

// MARK: - CPSessionConfigurationDelegate implementation
extension CarPlayService: CPSessionConfigurationDelegate {
  func sessionConfiguration(_ sessionConfiguration: CPSessionConfiguration,
                            limitedUserInterfacesChanged limitedUserInterfaces: CPLimitableUserInterface) {

  }
  @available(iOS 13.0, *)
  func sessionConfiguration(_ sessionConfiguration: CPSessionConfiguration,
                            contentStyleChanged contentStyle: CPContentStyle) {
    // Handle the CarPlay content style changing triggered by the 'Always Show Dark Maps' toggle.
    updateContentStyle(contentStyle)
  }
}

// MARK: - CPMapTemplateDelegate implementation
extension CarPlayService: CPMapTemplateDelegate {
  // Instrument cluster maneuver display
  @available(iOS 17.4, *)
  public func mapTemplateShouldProvideNavigationMetadata(_ mapTemplate: CPMapTemplate) -> Bool {
    return true
  }

  public func mapTemplateDidShowPanningInterface(_ mapTemplate: CPMapTemplate) {
    guard mapTemplate === rootMapTemplate else { return }
    panningInterfaceState.didShow(mapTemplate)
    isUserPanMap = false
    isInitialCarHeadingModeDisabled = true
    MapTemplateBuilder.configurePanUI(mapTemplate: mapTemplate)
    FrameworkHelper.stopLocationFollow()
  }

  public func mapTemplateDidDismissPanningInterface(_ mapTemplate: CPMapTemplate) {
    guard mapTemplate === rootMapTemplate,
          panningInterfaceState.didDismiss(mapTemplate) else { return }
    if let info = mapTemplate.userInfo as? MapInfo,
      info.type == CPConstants.TemplateType.navigation {
      MapTemplateBuilder.configureNavigationUI(mapTemplate: mapTemplate)
    } else {
      MapTemplateBuilder.configureBaseUI(mapTemplate: mapTemplate)
    }
    switchMyPositionModeFromCarPlayControl()
  }

  @objc(mapTemplate:panEndedWithDirection:)
  func mapTemplate(_ mapTemplate: CPMapTemplate, panEndedWith direction: Int) {
    var offset = UIOffset(horizontal: 0.0, vertical: 0.0)
    let offsetStep: CGFloat = 0.25
    let panDirection = CPMapTemplate.PanDirection(rawValue: direction)
    if panDirection.contains(.up) { offset.vertical -= offsetStep }
    if panDirection.contains(.down) { offset.vertical += offsetStep }
    if panDirection.contains(.left) { offset.horizontal += offsetStep }
    if panDirection.contains(.right) { offset.horizontal -= offsetStep }
    FrameworkHelper.moveMap(offset)
    isUserPanMap = true
  }

  
  @objc(mapTemplate:panWithDirection:)
  func mapTemplate(_ mapTemplate: CPMapTemplate, panWith direction: Int) {
    var offset = UIOffset(horizontal: 0.0, vertical: 0.0)
    let offsetStep: CGFloat = 0.1
    let panDirection = CPMapTemplate.PanDirection(rawValue: direction)
    if panDirection.contains(.up) { offset.vertical -= offsetStep }
    if panDirection.contains(.down) { offset.vertical += offsetStep }
    if panDirection.contains(.left) { offset.horizontal += offsetStep }
    if panDirection.contains(.right) { offset.horizontal -= offsetStep }
    FrameworkHelper.moveMap(offset)
    isUserPanMap = true
  }

  func mapTemplate(_ mapTemplate: CPMapTemplate, didUpdatePanGestureWithTranslation translation: CGPoint, velocity: CGPoint) {
    let scaleFactor = self.carplayVC?.mapView?.contentScaleFactor ?? 1
    FrameworkHelper.scrollMap(toDistanceX:-scaleFactor * translation.x, andY:-scaleFactor * translation.y);
  }

  func mapTemplate(_ mapTemplate: CPMapTemplate, startedTrip trip: CPTrip, using routeChoice: CPRouteChoice) {
    guard let info = routeChoice.userInfo as? RouteInfo else {
      if let info = routeChoice.userInfo as? [String: Any],
        let code = info[CPConstants.Trip.errorCode] as? RouterResultCode,
        let countries = info[CPConstants.Trip.missedCountries] as? [String] {
        showErrorAlert(code: code, countries: countries)
      }
      return
    }
    mapTemplate.userInfo = MapInfo(type: CPConstants.TemplateType.previewAccepted)
    mapTemplate.hideTripPreviews()
    startNavigation(trip: trip, routeInfo: info)
  }

  private func startNavigation(trip: CPTrip, routeInfo info: RouteInfo) {
    guard let router = router,
      let interfaceController = interfaceController,
      let rootMapTemplate = rootMapTemplate else {
        LOG(.warning, "\(CarPlayLogging.carPlay) startNavigation failed router=\(router != nil) interfaceController=\(interfaceController != nil) rootMapTemplate=\(rootMapTemplate != nil)")
        return
    }
    LOG(.info, "\(CarPlayLogging.carPlay) startNavigation begin")
    defer { logStateSnapshot("startNavigation handling completed") }

    MapTemplateBuilder.configureNavigationUI(mapTemplate: rootMapTemplate)

    if interfaceController.templates.count > 1 {
      interfaceController.popToRootTemplate(animated: false)
    }
    router.startNavigationSession(forTrip: trip, template: rootMapTemplate)
    router.startRoute()
    if let estimates = createEstimates(routeInfo: info) {
      rootMapTemplate.updateEstimates(estimates, for: trip)
    }

    if let carplayVC = carplayVC {
      carplayVC.updateCurrentSpeed(info.speedMps, speedLimitMps: info.speedLimitMps)
      carplayVC.showSpeedControl()
    }
    updateVisibleViewPortState(.navigation)
  }

  func mapTemplate(_ mapTemplate: CPMapTemplate, displayStyleFor maneuver: CPManeuver) -> CPManeuverDisplayStyle {
    if let type = maneuver.userInfo as? String {
      switch type {
      case CPConstants.Maneuvers.lanes: return .symbolOnly
      case CPConstants.Maneuvers.secondary: return .trailingSymbol
      default: break
      }
    }
    return .leadingSymbol
  }

  func mapTemplate(_ mapTemplate: CPMapTemplate,
                   selectedPreviewFor trip: CPTrip,
                   using routeChoice: CPRouteChoice) {
    guard let previewTrip = router?.previewTrip, previewTrip == trip else {
      applyUndefinedEstimates(template: mapTemplate, trip: trip)
      router?.buildRoute(trip: trip)
      return
    }
    guard let info = routeChoice.userInfo as? RouteInfo,
      let estimates = createEstimates(routeInfo: info) else {
      applyUndefinedEstimates(template: mapTemplate, trip: trip)
      router?.rebuildRoute()
      return
    }
    mapTemplate.updateEstimates(estimates, for: trip)
    routeChoice.userInfo = nil
    router?.rebuildRoute()
  }
}


// MARK: - CPListTemplateDelegate implementation
extension CarPlayService: CPListTemplateDelegate {
  func listTemplate(_ listTemplate: CPListTemplate, didSelect item: CPListItem, completionHandler: @escaping () -> Void) {
    if let userInfo = item.userInfo as? ListItemInfo {
      switch userInfo.type {
      case CPConstants.ListItemType.history:
        let locale = window?.textInputMode?.primaryLanguage ?? "en"
        guard let searchService = searchService else {
          completionHandler()
          return
        }
        searchService.searchText(item.text ?? "", forInputLocale: locale, completionHandler: { [weak self] results in
          guard let self = self else { return }
          let template = ListTemplateBuilder.buildListTemplate(for: .searchResults(results: results))
          completionHandler()
          self.pushTemplate(template, animated: true)
        })
      case CPConstants.ListItemType.bookmarkLists where userInfo.metadata is CategoryInfo:
        let metadata = userInfo.metadata as! CategoryInfo
        let template = ListTemplateBuilder.buildListTemplate(for: .bookmarks(category: metadata.category))
        completionHandler()
        pushTemplate(template, animated: true)
      case CPConstants.ListItemType.bookmarks where userInfo.metadata is BookmarkInfo:
        let metadata = userInfo.metadata as! BookmarkInfo
        let bookmark = MWMCarPlayBookmarkObject(bookmarkId: metadata.bookmarkId)
        preparePreview(forBookmark: bookmark)
        completionHandler()
      case CPConstants.ListItemType.searchResults where userInfo.metadata is SearchResultInfo:
        let metadata = userInfo.metadata as! SearchResultInfo
        preparePreviewForSearchResults(selectedRow: metadata.originalRow)
        completionHandler()
      default:
        completionHandler()
      }
    }
  }
}

// MARK: - CPSearchTemplateDelegate implementation
extension CarPlayService: CPSearchTemplateDelegate {
  func searchTemplate(_ searchTemplate: CPSearchTemplate, updatedSearchText searchText: String, completionHandler: @escaping ([CPListItem]) -> Void) {
    self.searchText = searchText
    let locale = window?.textInputMode?.primaryLanguage ?? "en"
    guard let searchService = searchService else {
      completionHandler([])
      return
    }
    searchService.searchText(self.searchText, forInputLocale: locale, completionHandler: { results in
      var items = [CPListItem]()
      for object in results {
        let item = CPListItem(text: object.title, detailText: object.address)
        item.userInfo = ListItemInfo(type: CPConstants.ListItemType.searchResults,
                                     metadata: SearchResultInfo(originalRow: object.originalRow))
        items.append(item)
      }
      completionHandler(items)
    })
  }

  func searchTemplate(_ searchTemplate: CPSearchTemplate, selectedResult item: CPListItem, completionHandler: @escaping () -> Void) {
    searchService?.saveLastQuery()
    if let info = item.userInfo as? ListItemInfo,
      let metadata = info.metadata as? SearchResultInfo {
      preparePreviewForSearchResults(selectedRow: metadata.originalRow)
    }
    completionHandler()
  }

  func searchTemplateSearchButtonPressed(_ searchTemplate: CPSearchTemplate) {
    let locale = window?.textInputMode?.primaryLanguage ?? "en"
    guard let searchService = searchService else {
      return
    }
    searchService.searchText(searchText, forInputLocale: locale, completionHandler: { [weak self] results in
      guard let self = self else { return }
      let template = ListTemplateBuilder.buildListTemplate(for: .searchResults(results: results))
      self.pushTemplate(template, animated: true)
    })
  }
}

// MARK: - CarPlayRouterListener implementation
extension CarPlayService: CarPlayRouterListener {
  func didCreateRoute(routeInfo: RouteInfo, trip: CPTrip) {
    if pendingDashboardNavigationTrip === trip {
      LOG(.info, "\(CarPlayLogging.carPlay) dashboardRoute completed; startNavigation begin")
      pendingDashboardNavigationTrip = nil
      startNavigation(trip: trip, routeInfo: routeInfo)
      return
    }
    guard let currentTemplate = interfaceController?.topTemplate as? CPMapTemplate,
      let info = currentTemplate.userInfo as? MapInfo,
      info.type == CPConstants.TemplateType.preview else {
        return
    }
    if let estimates = createEstimates(routeInfo: routeInfo) {
      currentTemplate.updateEstimates(estimates, for: trip)
    }
  }

  func didUpdateRouteInfo(_ routeInfo: RouteInfo, forTrip trip: CPTrip) {
    if let carplayVC = carplayVC {
      carplayVC.updateCurrentSpeed(routeInfo.speedMps, speedLimitMps: routeInfo.speedLimitMps)
    }
    guard let router = router,
      let template = rootMapTemplate else {
        return
    }
    router.updateEstimates()
    if let estimates = createEstimates(routeInfo: routeInfo) {
      template.updateEstimates(estimates, for: trip)
    }
    trip.routeChoices.first?.userInfo = routeInfo
  }

  func didFailureBuildRoute(forTrip trip: CPTrip, code: RouterResultCode, countries: [String]) {
    if pendingDashboardNavigationTrip === trip {
      LOG(.warning, "\(CarPlayLogging.carPlay) dashboardRoute failed code=\(code.rawValue)")
      pendingDashboardNavigationTrip = nil
      defer { logStateSnapshot("dashboardRoute failure handling completed") }
      showErrorAlert(code: code, countries: countries)
      return
    }
    guard let template = interfaceController?.topTemplate as? CPMapTemplate else { return }
    trip.routeChoices.first?.userInfo = [CPConstants.Trip.errorCode: code, CPConstants.Trip.missedCountries: countries]
    applyUndefinedEstimates(template: template, trip: trip)
  }

  func routeDidFinish(_ trip: CPTrip) {
    pendingDashboardNavigationTrip = nil
    if router?.currentTrip == nil { return }
    router?.finishTrip()
    if let carplayVC = carplayVC {
      carplayVC.hideSpeedControl()
    }
    updateMapTemplateUIToTripFinished(trip)
  }
}

// MARK: - LocationModeListener implementation
extension CarPlayService: LocationModeListener {
  func processMyPositionStateModeEvent(_ mode: MWMMyPositionMode) {
    currentPositionMode = mode
    engageCarFollowIfNeeded(mode)

    // make sure we have a rootMapTemplate
    guard let rootMapTemplate = rootMapTemplate else {
      return
    }
    
    // exit if we're navigating
    guard let info = rootMapTemplate.userInfo as? MapInfo,
              info.type == CPConstants.TemplateType.main else {
        MapTemplateBuilder.updateMyPositionModeButton(mapTemplate: rootMapTemplate)
        return
    }
    switch mode {
    case .follow, .followAndRotate:
      if !panningInterfaceState.isPresented {
        MapTemplateBuilder.setupDestinationButton(mapTemplate: rootMapTemplate)
        MapTemplateBuilder.updateMyPositionModeButton(mapTemplate: rootMapTemplate)
      }
    case .notFollow:
      if !panningInterfaceState.isPresented {
        MapTemplateBuilder.setupRecenterButton(mapTemplate: rootMapTemplate)
        MapTemplateBuilder.updateMyPositionModeButton(mapTemplate: rootMapTemplate)
      }
    case .pendingPosition, .notFollowNoPosition:
      rootMapTemplate.leadingNavigationBarButtons = []
      MapTemplateBuilder.updateMyPositionModeButton(mapTemplate: rootMapTemplate)
    }
  }
}

// MARK: - Alerts and Trip Previews
extension CarPlayService {
  func preparePreviewForSearchResults(selectedRow row: Int) {
    var results = searchService?.lastResults ?? []
    if let currentItemIndex = results.firstIndex(where: { $0.originalRow == row }) {
      let item = results.remove(at: currentItemIndex)
      results.insert(item, at: 0)
    } else {
      results.insert(MWMCarPlaySearchResultObject(forRow: row), at: 0)
    }
    if let router = router,
      let startPoint = MWMRoutePoint(lastLocationAndType: .start,
                                     intermediateIndex: 0) {
      let endPoints = results.compactMap({ MWMRoutePoint(cgPoint: $0.mercatorPoint,
                                                         title: $0.title,
                                                         subtitle: $0.address,
                                                         type: .finish,
                                                         intermediateIndex: 0) })
      let trips = endPoints.map({ router.createTrip(startPoint: startPoint, endPoint: $0) })
      if router.currentTrip == nil {
        preparePreview(trips: trips)
      } else {
        showRerouteAlert(trips: trips)
      }
    }
  }

  func preparePreview(forBookmark bookmark: MWMCarPlayBookmarkObject) {
    if let router = router,
      let startPoint = MWMRoutePoint(lastLocationAndType: .start,
                                      intermediateIndex: 0),
      let endPoint = MWMRoutePoint(cgPoint: bookmark.mercatorPoint,
                                   title: bookmark.prefferedName,
                                   subtitle: bookmark.address,
                                   type: .finish,
                                   intermediateIndex: 0) {
      let trip = router.createTrip(startPoint: startPoint, endPoint: endPoint)
      if router.currentTrip == nil {
        preparePreview(trips: [trip])
      } else {
        showRerouteAlert(trips: [trip])
      }
    }
  }

  func preparePreview(trips: [CPTrip]) {
    let mapTemplate = MapTemplateBuilder.buildTripPreviewTemplate(forTrips: trips)
    if let interfaceController = interfaceController {
      mapTemplate.mapDelegate = self

      if interfaceController.templates.count > 1 {
        interfaceController.popToRootTemplate(animated: false)
      }
      interfaceController.pushTemplate(mapTemplate, animated: false)
    }
  }

  func showPreview(mapTemplate: CPMapTemplate, trips: [CPTrip]) {
    let tripTextConfig = CPTripPreviewTextConfiguration(startButtonTitle: L("trip_start"),
                                                        additionalRoutesButtonTitle: nil,
                                                        overviewButtonTitle: nil)
    mapTemplate.showTripPreviews(trips, textConfiguration: tripTextConfig)
  }

  func createEstimates(routeInfo: RouteInfo) -> CPTravelEstimates? {
    let measurement = Measurement(value: routeInfo.targetDistance, unit: routeInfo.targetUnits)
    return CPTravelEstimates(distanceRemaining: measurement, timeRemaining: routeInfo.timeToTarget)
  }

  func applyUndefinedEstimates(template: CPMapTemplate, trip: CPTrip) {
    let measurement = Measurement(value: -1,
                                  unit: UnitLength.meters)
    let estimates = CPTravelEstimates(distanceRemaining: measurement,
                                      timeRemaining: -1)
    template.updateEstimates(estimates, for: trip)
  }

  func showRerouteAlert(trips: [CPTrip]) {
    let yesAction = CPAlertAction(title: L("yes"), style: .default, handler: { [unowned self] _ in
      self.router?.cancelTrip()
      self.updateMapTemplateUIToBase()
      self.preparedToPreviewTrips = trips
      self.interfaceController?.dismissTemplate(animated: true)
    })
    let noAction = CPAlertAction(title: L("no"), style: .cancel, handler: { [unowned self] _ in
      self.interfaceController?.dismissTemplate(animated: true)
    })
    let alert = CPAlertTemplate(titleVariants: [L("redirect_route_alert")], actions: [noAction, yesAction])
    alert.userInfo = [CPConstants.TemplateKey.alert: CPConstants.TemplateType.redirectRoute]
    presentAlert(alert, animated: true)
  }

  func showKeyboardAlert() {
    let okAction = CPAlertAction(title: L("ok"), style: .default, handler: { [unowned self] _ in
      self.interfaceController?.dismissTemplate(animated: true)
    })
    let alert = CPAlertTemplate(titleVariants: [L("keyboard_availability_alert")], actions: [okAction])
    presentAlert(alert, animated: true)
  }

  func showErrorAlert(code: RouterResultCode, countries: [String]) {
    var titleVariants = [String]()
    switch code {
    case .noCurrentPosition:
      titleVariants = ["\(L("dialog_routing_check_gps_carplay"))"]
    case .startPointNotFound:
      titleVariants = ["\(L("dialog_routing_change_start_carplay"))"]
    case .endPointNotFound:
      titleVariants = ["\(L("dialog_routing_change_end_carplay"))"]
    case .routeNotFoundRedressRouteError,
         .routeNotFound,
         .inconsistentMWMandRoute:
      titleVariants = ["\(L("dialog_routing_unable_locate_route_carplay"))"]
    case .routeFileNotExist,
         .fileTooOld,
         .needMoreMaps,
         .pointsInDifferentMWM:
      titleVariants = ["\(L("dialog_routing_download_files_carplay"))"]
    case .internalError,
         .intermediatePointNotFound:
      titleVariants = ["\(L("dialog_routing_system_error_carplay"))"]
    case .noError,
         .cancelled,
         .hasWarnings,
         .transitRouteNotFoundNoNetwork,
         .transitRouteNotFoundTooLongPedestrian:
      return
    }

    let okAction = CPAlertAction(title: L("ok"), style: .cancel, handler: { [unowned self] _ in
      self.interfaceController?.dismissTemplate(animated: true)
    })
    let alert = CPAlertTemplate(titleVariants: titleVariants, actions: [okAction])
    presentAlert(alert, animated: true)
  }

  func showRecoverRouteAlert(trip: CPTrip, isTypeCorrect: Bool) {
    let yesAction = CPAlertAction(title: L("ok"), style: .default, handler: { [unowned self] _ in
      var info = trip.userInfo as? [String: MWMRoutePoint]

      if let startPoint = MWMRoutePoint(lastLocationAndType: .start,
                                        intermediateIndex: 0) {
        info?[CPConstants.Trip.start] = startPoint
      }
      trip.userInfo = info
      self.preparedToPreviewTrips = [trip]
      self.router?.updateStartPointAndRebuild(trip: trip)
      self.interfaceController?.dismissTemplate(animated: true)
    })
    let noAction = CPAlertAction(title: L("cancel"), style: .cancel, handler: { [unowned self] _ in
      FrameworkHelper.rotateMap(0.0, animated: false)
      self.router?.completeRouteAndRemovePoints()
      self.interfaceController?.dismissTemplate(animated: true)
    })
    let title = isTypeCorrect ? L("dialog_routing_rebuild_from_current_location_carplay") : L("dialog_routing_rebuild_for_vehicle_carplay")
    let alert = CPAlertTemplate(titleVariants: [title], actions: [noAction, yesAction])
    alert.userInfo = [CPConstants.TemplateKey.alert: CPConstants.TemplateType.restoreRoute]
    presentAlert(alert, animated: true)
  }

  @objc func windowHeight() -> CGFloat {
    if let window {
      return window.height
    }
    return 0
  }
}
