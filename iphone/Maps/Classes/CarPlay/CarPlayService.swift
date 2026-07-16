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

  private var rootTemplateDidAppear = false
  private var panningInterfaceState = CarPlayPanningInterfaceState()
  private var hasAppliedDefaultCarZoom = false
  private var hasEngagedInitialCarFollow = false
  private var isInitialCarHeadingModeDisabled = false
  private func resetCarSessionDefaults() {
    hasAppliedDefaultCarZoom = false
    hasEngagedInitialCarFollow = false
    isInitialCarHeadingModeDisabled = false
    isCarMapViewportReady = false
    isWaitingForCarMapViewport = false
    carMapViewportReadinessAttempts = 0
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

  private enum PendingDashboardAction {
    case navigateBookmark(MWMCarPlayBookmarkObject)
    case destinationPicker
  }
  private var pendingDashboardAction: PendingDashboardAction?
  private var pendingDashboardNavigationTrip: CPTrip?

  @objc func setup(window: CPWindow, interfaceController: CPInterfaceController) {
    LOG(.info, "Settting up service...")
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
      LOG(.info, "[CarPlayHost] setup(): rebinding to a new connection within the teardown grace period")
    } else {
      searchService = CarPlaySearchService()
      let router = CarPlayRouter()
      router.addListener(self)
      router.subscribeToEvents()
      router.setupCarPlaySpeedCameraMode()
      self.router = router
      MWMRouter.unsubscribeFromEvents()
    }
    startObservingTTS()
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
    if let action = pendingDashboardAction {
      LOG(.info, "[CarPlayHost] setup() firing deferred dashboard action")
      pendingDashboardAction = nil
      switch action {
      case .navigateBookmark(let bookmark): navigateToBookmarkFromDashboard(bookmark: bookmark)
      case .destinationPicker: showDestinationPickerFromDashboard()
      }
    }
    logStateSnapshot("setup() end")
  }

  private var savedInterfaceController: CPInterfaceController?
  private var isObservingTTS = false

  func showOnPhone() {
    LOG(.info, "Show on the Phone screen")
    savedInterfaceController = interfaceController
    switchScreenToPhone()
    showPhoneModeAlert()
  }

  private func showOnCarplay() {
    LOG(.info, "Show on the Car screen")
    guard let window, let savedInterfaceController else {
      LOG(.warning, "Failed to show on carplay: the `window` is \(String(describing: window)), the `savedInterfaceController` is \(String(describing: savedInterfaceController))")
      return
    }
    setup(window: window, interfaceController: savedInterfaceController)
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
    stopObservingTTS()
    sessionConfiguration = nil
    interfaceController = nil
    pendingDashboardAction = nil
    pendingDashboardNavigationTrip = nil
    // Apply the visual-scale change (and its GPU context reset) before the theme switch,
    // so the context teardown doesn't race with an in-flight route recache from the style change.
    updateMapHost()
    ThemeManager.invalidate()
  }

  private var pendingTeardown: DispatchWorkItem?
  private var teardownBackgroundTask: UIBackgroundTaskIdentifier = .invalid
  private static let kTeardownGracePeriod: TimeInterval = 2.0

  private func beginTeardownBackgroundTask() {
    endTeardownBackgroundTask()
    teardownBackgroundTask = UIApplication.shared.beginBackgroundTask(withName: "CarPlay scene teardown") { [weak self] in
      guard let self else { return }
      if self.interfaceController == nil {
        LOG(.warning, "[CarPlayHost] teardown background task expired; tearing down immediately")
        self.pendingTeardown?.cancel()
        self.pendingTeardown = nil
        self.destroy()
        self.window = nil
      } else {
        self.endTeardownBackgroundTask()
      }
    }
  }

  private func endTeardownBackgroundTask() {
    guard teardownBackgroundTask != .invalid else { return }
    let task = teardownBackgroundTask
    teardownBackgroundTask = .invalid
    UIApplication.shared.endBackgroundTask(task)
  }

  func appSceneDidDisconnect() {
    logStateSnapshot("appSceneDidDisconnect")
    interfaceController?.delegate = nil
    interfaceController = nil
    sessionConfiguration = nil
    if isDashboardActive {
      updateMapHost()
    }
    beginTeardownBackgroundTask()
    let teardown = DispatchWorkItem { [weak self] in
      guard let self else { return }
      self.pendingTeardown = nil
      guard self.interfaceController == nil else {
        self.endTeardownBackgroundTask()
        return
      }
      LOG(.info, "[CarPlayHost] app scene stayed disconnected; tearing down the CarPlay service")
      self.destroy()
      self.window = nil
    }
    pendingTeardown?.cancel()
    pendingTeardown = teardown
    DispatchQueue.main.asyncAfter(deadline: .now() + Self.kTeardownGracePeriod, execute: teardown)
  }

  @objc func destroy() {
    logStateSnapshot("destroy()")
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

  func logStateSnapshot(_ reason: String) {
    var template = "root=nil"
    if let root = rootMapTemplate {
      let type = (root.userInfo as? MapInfo)?.type ?? "?"
      template = "root=\(type) didAppear=\(rootTemplateDidAppear) mapButtons=\(root.mapButtons.count) leading=\(root.leadingNavigationBarButtons.count) trailing=\(root.trailingNavigationBarButtons.count)"
    }
    var hosting = "mapHost=\(mapHost) mapView=unloaded"
    if let mapVC = MapViewController.shared(), mapVC.isViewLoaded {
      let superview = mapVC.mapView.superview.map { String(describing: type(of: $0)) } ?? "nil"
      let window = mapVC.mapView.window.map { String(describing: type(of: $0)) } ?? "nil"
      hosting = "mapHost=\(mapHost) superview=\(superview) window=\(window)"
    }
    let service = "activated=\(isCarplayActivated) controller=\(interfaceController != nil) trip=\(router?.currentTrip != nil) dashActive=\(isDashboardActive)"
    let scenes = UIApplication.shared.connectedScenes
      .map { "\(Self.shortSceneRole($0.session.role)):\($0.activationState.rawValue)" }
      .sorted()
      .joined(separator: " ")
    LOG(.info, "[CarPlayDiag] \(reason): \(template) | \(hosting) | \(service) | appState=\(UIApplication.shared.applicationState.rawValue) scenes=[\(scenes)]")
  }

  private static func shortSceneRole(_ role: UISceneSession.Role) -> String {
    if role.rawValue.contains("Dashboard") { return "dash" }
    if role.rawValue.hasPrefix("CP") { return "car" }
    return "phone"
  }

  private func setRootTemplate(_ template: CPMapTemplate) {
    rootTemplateDidAppear = false
    panningInterfaceState.reset()
    interfaceController?.setRootTemplate(template, animated: false) { success, error in
      self.logStateSnapshot("setRootTemplate completion success=\(success) error=\(String(describing: error))")
    }
  }

  func appSceneDidBecomeActive() {
    logStateSnapshot("appSceneDidBecomeActive")
    reconcileMapHostIfOrphaned()
    resumeLocationForActiveCarSceneIfNeeded()
    engageCarFollowIfNeeded(currentPositionMode, request: .sceneReactivation)
    guard isCarplayActivated, let controller = interfaceController else { return }
    if rootTemplateDidAppear {
      LOG(.info, "app scene active: root template already presented, nothing to reconcile")
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
    LOG(.warning, "app scene active but root template never presented; re-applying root template")
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

    MapsAppDelegate.theApp().ensureMapNavigationController()
    guard let mapVC = MapViewController.shared() else {
      LOG(.warning, "[CarPlayHost] Failed to host the map in \(desired): MapViewController is missing")
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
      LOG(.warning, "[CarPlayHost] \(desired) host vanished before attach; falling back to the phone representation")
      desired = MapsAppDelegate.theApp().window != nil ? .phone : .none
      guard desired != mapHost else { return }
    }
    LOG(.info, "[CarPlayHost] Map host switch: \(mapHost) -> \(desired)")

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
    applyDefaultCarZoomIfNeeded()
    engageCarFollowIfNeeded(currentPositionMode)
    logStateSnapshot("after map host switch")
    refreshLocationPolicyIfHostingChanged(from: wasHostingMapOnCarScreen, reason: "updateMapHost")
  }

  private func reconcileMapHostIfOrphaned() {
    guard let mapVC = MapViewController.shared(),
          mapVC.isViewLoaded,
          mapVC.mapView.window == nil,
          mapHost != .none else { return }
    let wasHostingMapOnCarScreen = isHostingMapOnCarScreen
    LOG(.warning, "[CarPlayHost] Map view is in no window while mapHost=\(mapHost); re-attaching")
    mapHost = .none
    updateMapHost()
    refreshLocationPolicyIfHostingChanged(from: wasHostingMapOnCarScreen, reason: "reconcileMapHostIfOrphaned")
  }

  private func refreshLocationPolicyIfHostingChanged(from wasHostingMapOnCarScreen: Bool, reason: String) {
    guard wasHostingMapOnCarScreen != isHostingMapOnCarScreen else { return }
    LOG(.info, "[CarPlayHost] car screen hosting changed \(wasHostingMapOnCarScreen) -> \(isHostingMapOnCarScreen); refreshing location policy (\(reason))")
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
      LOG(.info, "[CarPlayHost] app is backgrounded; deferring the \(mapHost) appearance until it becomes active")
      needsHostAppearanceRefresh = true
      return
    }
    needsHostAppearanceRefresh = false
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
      LOG(.info, "[CarPlayHost] app became active; applying the deferred \(self.mapHost) appearance")
      self.applyHostAppearanceIfActive()
    }
  }

  /// Re-sync the car map button glyph after re-attach so it doesn't show a stale position mode
  private func refreshMyPositionModeButton() {
    guard let rootMapTemplate else { return }
    MapTemplateBuilder.updateMyPositionModeButton(mapTemplate: rootMapTemplate)
  }

  /// Set default zoom to 15 on car screens
  private static let kDefaultCarZoomLevel: Int32 = 15
  private var isCarMapViewportReady = false
  private var isWaitingForCarMapViewport = false
  private var carMapViewportReadinessAttempts = 0
  private var needsBaseMapNorthUp = false
  private var needsRecenterOnViewportReady = false

  private func applyDefaultCarZoomIfNeeded() {
    guard !hasAppliedDefaultCarZoom,
          isCarMapViewportReady,
          mapHost == .carplay || mapHost == .dashboard,
          !MWMRouter.isRoutingActive(),
          currentPositionMode == .follow || currentPositionMode == .followAndRotate
    else { return }
    hasAppliedDefaultCarZoom = true
    FrameworkHelper.setZoomLevel(Self.kDefaultCarZoomLevel, animated: true)
  }

  private enum CarFollowRequest {
    case initialEngagement
    case sceneReactivation

    var allowsReengagement: Bool {
      switch self {
      case .initialEngagement: return false
      case .sceneReactivation: return true
      }
    }
  }

  private func engageCarFollowIfNeeded(_ mode: MWMMyPositionMode,
                                       request: CarFollowRequest = .initialEngagement) {
    guard (!hasEngagedInitialCarFollow || request.allowsReengagement),
          mapHost == .carplay || mapHost == .dashboard,
          !MWMRouter.isRoutingActive(),
          !panningInterfaceState.isPresented
    else { return }
    guard isCarMapViewportReady else {
      if request.allowsReengagement {
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
        LOG(.warning, "[CarPlayHost] graphics context did not initialize in time; deferring map defaults until the next layout")
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
    isCarMapViewportReady = true
    if needsBaseMapNorthUp {
      needsBaseMapNorthUp = false
      FrameworkHelper.rotateMap(0.0, animated: false)
    }
    applyDefaultCarZoomIfNeeded()
    let request: CarFollowRequest = needsRecenterOnViewportReady ? .sceneReactivation : .initialEngagement
    needsRecenterOnViewportReady = false
    engageCarFollowIfNeeded(currentPositionMode, request: request)
  }

  func switchMyPositionModeFromCarPlayControl() {
    isInitialCarHeadingModeDisabled = true
    FrameworkHelper.switchMyPositionMode()
  }

  // MARK: - Dashboard scene

  @objc func dashboardConnected(window: UIWindow) {
    logStateSnapshot("dashboardConnected")
    dashboardWindow = window
    window.rootViewController = CarPlayDashboardMapViewController()
  }

  @objc func dashboardDisconnected() {
    logStateSnapshot("dashboardDisconnected")
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
    logStateSnapshot("dashboardDidBecomeActive")
    isDashboardActive = true
    reconcileMapHostIfOrphaned()
    updateMapHost()
    resumeLocationForActiveCarSceneIfNeeded()
    engageCarFollowIfNeeded(currentPositionMode, request: .sceneReactivation)
  }

  @objc func dashboardDidResignActive() {
    logStateSnapshot("dashboardDidResignActive")
    isDashboardActive = false
    updateMapHost()
  }

  private func applyBaseRootTemplate() {
    let mapTemplate = MapTemplateBuilder.buildBaseTemplate(positionMode: currentPositionMode)
    mapTemplate.mapDelegate = self
    mapTemplate.tripEstimateStyle = rootTemplateStyle
    setRootTemplate(mapTemplate)
    needsBaseMapNorthUp = true
    if let mapView = MapViewController.shared()?.mapView {
      mapViewportDidBecomeReady(mapView)
    }
  }

  private func applyNavigationRootTemplate(trip: CPTrip, routeInfo: RouteInfo) {
    let mapTemplate = MapTemplateBuilder.buildNavigationTemplate()
    needsBaseMapNorthUp = false
    mapTemplate.mapDelegate = self
    setRootTemplate(mapTemplate)
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
    LOG(.info, "[CarPlayHost] navigateToBookmarkFromDashboard id=\(bookmark.bookmarkId) router=\(router != nil) interfaceController=\(interfaceController != nil)")
    guard let router = router, interfaceController != nil else {
      if !isPhoneModeRequested {
        LOG(.info, "[CarPlayHost] App scene not ready; deferring bookmark navigation to setup()")
        pendingDashboardAction = .navigateBookmark(bookmark)
      }
      return
    }
    guard let startPoint = MWMRoutePoint(lastLocationAndType: .start, intermediateIndex: 0),
          let endPoint = MWMRoutePoint(cgPoint: bookmark.mercatorPoint,
                                       title: bookmark.prefferedName,
                                       subtitle: bookmark.address,
                                       type: .finish,
                                       intermediateIndex: 0) else {
      LOG(.warning, "[CarPlayHost] Cannot navigate to bookmark: no current position fix")
      return
    }
    if router.currentTrip != nil {
      cancelCurrentTrip()
    }
    let trip = router.createTrip(startPoint: startPoint, endPoint: endPoint)
    pendingDashboardNavigationTrip = trip
    LOG(.info, "[CarPlayHost] Building route to bookmark '\(bookmark.prefferedName)'")
    router.buildRoute(trip: trip)
  }

  func showDestinationPickerFromDashboard() {
    guard interfaceController != nil else {
      if !isPhoneModeRequested {
        pendingDashboardAction = .destinationPicker
      }
      return
    }
    pushTemplate(ListTemplateBuilder.buildListTemplate(for: .history), animated: true)
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
    LOG(.info, "Cancel current trip")
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

  private func startObservingTTS() {
    guard !isObservingTTS else { return }
    isObservingTTS = true
    MWMTextToSpeech.add(self)
  }

  private func stopObservingTTS() {
    guard isObservingTTS else { return }
    isObservingTTS = false
    MWMTextToSpeech.remove(self)
  }

  private func refreshNavigationAudioButtons() {
    guard let rootMapTemplate,
          let info = rootMapTemplate.userInfo as? MapInfo,
          info.type == CPConstants.TemplateType.navigation,
          !panningInterfaceState.isPresented
    else { return }
    MapTemplateBuilder.updateNavigationAudioButtons(mapTemplate: rootMapTemplate)
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
    LOG(.info, "Trip finished; restoring base UI then presenting arrival alert (app state=\(UIApplication.shared.applicationState.rawValue))")
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
    guard let info = aTemplate.userInfo as? MapInfo else {
        return
    }
    if let mapTemplate = aTemplate as? CPMapTemplate {
      LOG(.info, "templateWillAppear type=\(info.type) mapButtons=\(mapTemplate.mapButtons.count) leading=\(mapTemplate.leadingNavigationBarButtons.count) trailing=\(mapTemplate.trailingNavigationBarButtons.count) isRoot=\(mapTemplate === rootMapTemplate) host=\(mapHost)")
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
    guard let mapTemplate = aTemplate as? CPMapTemplate,
      let info = aTemplate.userInfo as? MapInfo else {
        return
    }
    LOG(.info, "templateDidAppear type=\(info.type) mapButtons=\(mapTemplate.mapButtons.count) leading=\(mapTemplate.leadingNavigationBarButtons.count) trailing=\(mapTemplate.trailingNavigationBarButtons.count) navigating=\(router?.currentTrip != nil) host=\(mapHost)")
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
      LOG(.warning, "Main template appeared with no map buttons; restoring base UI")
      updateMapTemplateUIToBase()
      return
    }

    if info.type == CPConstants.TemplateType.preview, let trips = info.trips {
      showPreview(mapTemplate: mapTemplate, trips: trips)
    }
  }

  func templateWillDisappear(_ aTemplate: CPTemplate, animated: Bool) {
    guard let info = aTemplate.userInfo as? MapInfo else {
        return
    }
    if info.type == CPConstants.TemplateType.preview {
      router?.completeRouteAndRemovePoints()
    }
  }

  func templateDidDisappear(_ aTemplate: CPTemplate, animated: Bool) {
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
        LOG(.warning, "[CarPlayHost] startNavigation aborted: router=\(router != nil) interfaceController=\(interfaceController != nil) rootMapTemplate=\(rootMapTemplate != nil)")
        return
    }
    LOG(.info, "[CarPlayHost] startNavigation: beginning navigation session + route guidance")

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
      LOG(.info, "[CarPlayHost] Route built for dashboard shortcut; starting navigation")
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
      LOG(.warning, "[CarPlayHost] Route build failed for dashboard shortcut: code=\(code.rawValue)")
      pendingDashboardNavigationTrip = nil
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
    applyDefaultCarZoomIfNeeded()
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

// MARK: - MWMTextToSpeechObserver implementation
extension CarPlayService: MWMTextToSpeechObserver {
  func onTTSStatusUpdated() {
    refreshNavigationAudioButtons()
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
