import AVFoundation
import CarPlay
import Contacts

protocol CarPlayRouterListener: AnyObject {
  func didCreateRoute(routeInfo: RouteInfo,
                      trip: CPTrip)
  func didUpdateRouteInfo(_ routeInfo: RouteInfo, forTrip trip: CPTrip)
  func didFailureBuildRoute(forTrip trip: CPTrip, code: RouterResultCode, countries: [String])
  func routeDidFinish(_ trip: CPTrip)
}

enum CarPlayManeuverSymbol {
  static func image(named name: String,
                    exitNumber: Int? = nil,
                    displayScale: CGFloat) -> UIImage? {
    guard let base = UIImage(named: name) else { return nil }

    let black = render(base, tint: .black, exitNumber: exitNumber, displayScale: displayScale)
    let white = render(base, tint: .white, exitNumber: exitNumber, displayScale: displayScale)

    let asset = UIImageAsset()
    asset.register(black, with: traits(for: .light, scale: displayScale))
    asset.register(white, with: traits(for: .dark, scale: displayScale))
    return asset.image(with: traits(for: .light, scale: displayScale))
  }

  static func resolvedVariant(of image: UIImage, style: UIUserInterfaceStyle) -> UIImage {
    guard let asset = image.imageAsset else { return image }
    return asset.image(with: traits(for: style, scale: image.scale))
  }

  private static func traits(for style: UIUserInterfaceStyle, scale: CGFloat) -> UITraitCollection {
    return UITraitCollection(traitsFrom: [
      UITraitCollection(userInterfaceStyle: style),
      UITraitCollection(displayScale: scale),
    ])
  }

  private static func render(_ base: UIImage,
                             tint: UIColor,
                             exitNumber: Int?,
                             displayScale: CGFloat) -> UIImage {
    let format = UIGraphicsImageRendererFormat()
    format.scale = displayScale
    format.opaque = false
    let renderer = UIGraphicsImageRenderer(size: base.size, format: format)
    let image = renderer.image { _ in
      base.withRenderingMode(.alwaysTemplate)
        .withTintColor(tint, renderingMode: .alwaysOriginal)
        .draw(in: CGRect(origin: .zero, size: base.size))

      // Render the exit number on the roundabout symbol, until we have a better main symbol
      guard let exitNumber else { return }
      let text = String(exitNumber) as NSString
      let font = UIFont.systemFont(ofSize: base.size.height * 0.30, weight: .bold)
      let attributes: [NSAttributedString.Key: Any] = [.font: font, .foregroundColor: tint]
      let textSize = text.size(withAttributes: attributes)
      // Centre on the cap height (not the line box) so the digit is optically centred.
      let origin = CGPoint(x: (base.size.width - textSize.width) / 2,
                           y: base.size.height / 2 - font.ascender + font.capHeight / 2)
      text.draw(at: origin, withAttributes: attributes)
    }
    return image.withRenderingMode(.alwaysOriginal)
  }
}

enum CarPlayLaneSymbol {
  static func imageSet(for lanes: [LaneInfo], displayScale: CGFloat) -> CPImageSet? {
    guard !lanes.isEmpty,
      let lightContentImage = stripImage(for: lanes, tint: .white, displayScale: displayScale),
      let darkContentImage = stripImage(for: lanes, tint: .black, displayScale: displayScale) else {
      return nil
    }
    return CPImageSet(lightContentImage: lightContentImage,
                      darkContentImage: darkContentImage)
  }

  /// Draws the upcoming turn's lanes as one horizontal strip, centered in a 120x18pt canvas (max per Apple).
  /// The recommended lane(s) use `tint` at full opacity; others are dimmed, mirroring Android.
  private static func stripImage(for lanes: [LaneInfo],
                                 tint: UIColor,
                                 displayScale: CGFloat) -> UIImage? {
    guard !lanes.isEmpty else { return nil }
    let maxWidth: CGFloat = 120
    let height: CGFloat = 18
    let count = CGFloat(lanes.count)
    let cell = min(height, maxWidth / count)
    let xOffset = (maxWidth - cell * count) / 2
    let config = UIImage.SymbolConfiguration(pointSize: cell * 0.85, weight: .semibold)
    let format = UIGraphicsImageRendererFormat()
    format.scale = displayScale
    format.opaque = false
    let renderer = UIGraphicsImageRenderer(size: CGSize(width: maxWidth, height: height), format: format)
    return renderer.image { _ in
      for (i, lane) in lanes.enumerated() {
        let recommended = LaneWay(rawValue: lane.recommendedWay)
        let isActive = recommended != nil && recommended != LaneWay.none
        let way = isActive ? recommended!
          : (lane.laneWays.compactMap { LaneWay(rawValue: $0) }.first ?? .through)
        let color = isActive ? tint : tint.withAlphaComponent(0.38)
        guard let symbol = UIImage(systemName: way.symbolName, withConfiguration: config)?
          .withTintColor(color, renderingMode: .alwaysOriginal) else { continue }
        let cellRect = CGRect(x: xOffset + CGFloat(i) * cell, y: 0, width: cell, height: height)
        symbol.draw(in: AVMakeRect(aspectRatio: symbol.size, insideRect: cellRect))
      }
    }
  }
}

struct CarPlayLaneManeuverContent: Equatable {
  let laneWays: [UInt8]
  let recommendedWay: UInt8

  init(_ lane: LaneInfo) {
    laneWays = lane.laneWays
    recommendedWay = lane.recommendedWay
  }
}

struct CarPlayManeuverContent: Equatable {
  let secondaryTurnImageName: String?
  let lanes: [CarPlayLaneManeuverContent]

  init(routeInfo: RouteInfo) {
    switch routeInfo.carDirection {
    case .enterRoundAbout, .stayOnRoundAbout:
      // The numbered primary maneuver already represents entering and leaving the roundabout.
      secondaryTurnImageName = nil
    default:
      secondaryTurnImageName = routeInfo.nextTurnImageName
    }
    lanes = routeInfo.lanes.map(CarPlayLaneManeuverContent.init)
  }
}

struct CarPlayManeuverRefreshState {
  private(set) var hasPendingPrimary = false
  private(set) var displayedContent: CarPlayManeuverContent?

  mutating func markPrimaryChanged() {
    hasPendingPrimary = true
  }

  func needsRefresh(for content: CarPlayManeuverContent) -> Bool {
    return hasPendingPrimary || displayedContent != content
  }

  mutating func didDisplay(_ content: CarPlayManeuverContent) {
    hasPendingPrimary = false
    displayedContent = content
  }

  mutating func reset() {
    self = CarPlayManeuverRefreshState()
  }
}


@objc(MWMCarPlayRouter)
final class CarPlayRouter: NSObject {
  private let listenerContainer: ListenerContainer<CarPlayRouterListener>
  private let displayScale: CGFloat
  private var routeSession: CPNavigationSession?
  private var initialSpeedCamSettings: SpeedCameraManagerMode
  private var maneuverRefreshState = CarPlayManeuverRefreshState()
  /// Typed `AnyObject?` until we target iOS 18
  private var activeLaneGuidance: AnyObject?
  var currentTrip: CPTrip? {
    return routeSession?.trip
  }
  var previewTrip: CPTrip?
  var speedCameraMode: SpeedCameraManagerMode {
    return RoutingManager.routingManager.speedCameraMode
  }

  init(displayScale: CGFloat) {
    listenerContainer = ListenerContainer<CarPlayRouterListener>()
    self.displayScale = displayScale
    initialSpeedCamSettings = RoutingManager.routingManager.speedCameraMode
    super.init()
  }

  func addListener(_ listener: CarPlayRouterListener) {
    listenerContainer.addListener(listener)
  }

  func removeListener(_ listener: CarPlayRouterListener) {
    listenerContainer.removeListener(listener)
  }

  func subscribeToEvents() {
    RoutingManager.routingManager.add(self)
  }

  func unsubscribeFromEvents() {
    RoutingManager.routingManager.remove(self)
  }

  func completeRouteAndRemovePoints() {
    let manager = RoutingManager.routingManager
    manager.stopRoutingAndRemoveRoutePoints(true)
    manager.deleteSavedRoutePoints()
    manager.apply(routeType: .vehicle)
    previewTrip = nil
  }

  func rebuildRoute() {
    guard let trip = previewTrip else { return }
    do {
      try RoutingManager.routingManager.buildRoute()
    } catch let error as NSError {
      listenerContainer.forEach({
        let code = RouterResultCode(rawValue: UInt(error.code)) ?? .internalError
        $0.didFailureBuildRoute(forTrip: trip, code: code, countries: [])
      })
    }
  }

  func buildRoute(trip: CPTrip) {
    completeRouteAndRemovePoints()
    previewTrip = trip
    guard let info = trip.userInfo as? [String: MWMRoutePoint] else {
      listenerContainer.forEach({
        $0.didFailureBuildRoute(forTrip: trip, code: .routeNotFound, countries: [])
      })
      return
    }
    guard let startPoint = info[CPConstants.Trip.start],
      let endPoint = info[CPConstants.Trip.end] else {
        listenerContainer.forEach({
          var code: RouterResultCode!
          if info[CPConstants.Trip.end] == nil {
            code = .endPointNotFound
          } else {
            code = .startPointNotFound
          }
          $0.didFailureBuildRoute(forTrip: trip, code: code, countries: [])
        })
        return
    }

    let manager = RoutingManager.routingManager
    manager.add(routePoint: startPoint)
    manager.add(routePoint: endPoint)

    do {
      try manager.buildRoute()
    } catch let error as NSError {
      listenerContainer.forEach({
        let code = RouterResultCode(rawValue: UInt(error.code)) ?? .internalError
        $0.didFailureBuildRoute(forTrip: trip, code: code, countries: [])
      })
    }
  }

  func updateStartPointAndRebuild(trip: CPTrip) {
    let manager = RoutingManager.routingManager
    previewTrip = trip
    guard let info = trip.userInfo as? [String: MWMRoutePoint] else {
      listenerContainer.forEach({
        $0.didFailureBuildRoute(forTrip: trip, code: .routeNotFound, countries: [])
      })
      return
    }
    guard let startPoint = info[CPConstants.Trip.start] else {
        listenerContainer.forEach({
          $0.didFailureBuildRoute(forTrip: trip, code: .startPointNotFound, countries: [])
        })
        return
    }
    manager.add(routePoint: startPoint)
    manager.apply(routeType: .vehicle)
    do {
      try manager.buildRoute()
    } catch let error as NSError {
      listenerContainer.forEach({
        let code = RouterResultCode(rawValue: UInt(error.code)) ?? .internalError
        $0.didFailureBuildRoute(forTrip: trip, code: code, countries: [])
      })
    }
  }

  func startRoute() {
    let manager = RoutingManager.routingManager
    manager.startRoute()
  }

  func setupCarPlaySpeedCameraMode() {
    if case .auto = initialSpeedCamSettings {
      RoutingManager.routingManager.speedCameraMode = .always
    }
  }

  func setupInitialSpeedCameraMode() {
    RoutingManager.routingManager.speedCameraMode = initialSpeedCamSettings
  }

  func updateSpeedCameraMode(_ mode: SpeedCameraManagerMode) {
    initialSpeedCamSettings = mode
    RoutingManager.routingManager.speedCameraMode = mode
  }

  func restoreTripPreviewOnCarplay(beforeRootTemplateDidAppear: Bool) {
    guard MWMRouter.isRestoreProcessCompleted() else {
      DispatchQueue.main.async { [weak self] in
        self?.restoreTripPreviewOnCarplay(beforeRootTemplateDidAppear: false)
      }
      return
    }
    let manager = RoutingManager.routingManager
    MWMRouter.hideNavigationMapControls()
    guard manager.isRoutingActive,
      let startPoint = manager.startPoint,
      let endPoint = manager.endPoint else {
        completeRouteAndRemovePoints()
        return
    }
    let trip = createTrip(startPoint: startPoint,
                          endPoint: endPoint,
                          routeInfo: manager.routeInfo)
    previewTrip = trip
    if manager.type != .vehicle {
      CarPlayService.shared.showRecoverRouteAlert(trip: trip, isTypeCorrect: false)
      return
    }
    if !startPoint.isMyPosition {
      CarPlayService.shared.showRecoverRouteAlert(trip: trip, isTypeCorrect: true)
      return
    }
    if beforeRootTemplateDidAppear {
      CarPlayService.shared.preparedToPreviewTrips = [trip]
    } else {
      CarPlayService.shared.preparePreview(trips: [trip])
    }
  }

  func restoredNavigationSession() -> (CPTrip, RouteInfo)? {
    let manager = RoutingManager.routingManager
    if manager.isOnRoute,
      manager.type == .vehicle,
      let startPoint = manager.startPoint,
      let endPoint = manager.endPoint,
      let routeInfo = manager.routeInfo {
      MWMRouter.hideNavigationMapControls()
      let trip = createTrip(startPoint: startPoint,
                            endPoint: endPoint,
                            routeInfo: routeInfo)
      previewTrip = trip
      return (trip, routeInfo)
    }
    return nil
  }
}

// MARK: - Navigation session management
extension CarPlayRouter {
  func startNavigationSession(forTrip trip: CPTrip, template: CPMapTemplate) {
    guard routeSession == nil else {
      let errorMessage = "Route session is already running."
      LOG(.error, errorMessage)
      Toast.show(withText: errorMessage, alignment: .top)
      return
    }
    LOG(.info, "Starting a new navigation session")
    routeSession = template.startNavigationSession(for: trip)
    routeSession?.pauseTrip(for: .loading, description: nil)
    maneuverRefreshState.reset()
    if let routeInfo = RoutingManager.routingManager.routeInfo {
      updateUpcomingManeuvers(with: routeInfo)
    }
    RoutingManager.routingManager.setOnNewTurnCallback { [weak self] in
      self?.maneuverRefreshState.markPrimaryChanged()
    }
  }

  func cancelNavigationSession() {
    LOG(.info, "Cancelling navigation session")
    routeSession?.cancelTrip()
    routeSession = nil
    activeLaneGuidance = nil
    maneuverRefreshState.reset()
    RoutingManager.routingManager.resetOnNewTurnCallback()
  }

  func cancelTrip() {
    LOG(.info, "Cancelling trip")
    cancelNavigationSession()
    completeRouteAndRemovePoints()
  }

  func finishTrip() {
    LOG(.info, "Finishing trip")
    routeSession?.finishTrip()
    routeSession = nil
    activeLaneGuidance = nil
    maneuverRefreshState.reset()
    completeRouteAndRemovePoints()
    RoutingManager.routingManager.resetOnNewTurnCallback()
  }

  func updateUpcomingManeuvers(with routeInfo: RouteInfo) {
    guard let routeSession else { return }
    let content = CarPlayManeuverContent(routeInfo: routeInfo)
    let maneuvers = createUpcomingManeuvers(with: routeInfo, content: content)
    if #available(iOS 17.4, *) {
      if let guidance = activeLaneGuidance as? CPLaneGuidance {
        routeSession.add([guidance])
      }
      routeSession.add(maneuvers)
    }
    routeSession.upcomingManeuvers = maneuvers
    maneuverRefreshState.didDisplay(content)
    if #available(iOS 17.4, *) {
      routeSession.maneuverState = maneuverState(forDistanceToTurn: routeInfo.distanceToTurn,
                                                 units: routeInfo.turnUnits)
      routeSession.currentLaneGuidance = activeLaneGuidance as? CPLaneGuidance
      let roadName = routeInfo.currentRoadName.trimmingCharacters(in: .whitespacesAndNewlines)
      routeSession.currentRoadNameVariants = roadName.isEmpty ? [] : [roadName]
    }
  }

  @available(iOS 17.4, *)
  private func maneuverState(forDistanceToTurn distance: Double, units: UnitLength) -> CPManeuverState {
    let meters = Measurement(value: distance, unit: units).converted(to: .meters).value
    switch meters {
    case ..<30: return .execute
    case ..<150: return .prepare
    case ..<400: return .initial
    default: return .continue
    }
  }

  func updateEstimates(with routeInfo: RouteInfo) {
    guard let routeSession = routeSession,
          let primaryManeuver = routeSession.upcomingManeuvers.first,
          let estimates = createEstimates(routeInfo) else {
      return
    }
    routeSession.updateEstimates(estimates, for: primaryManeuver)
  }

  private func createEstimates(_ routeInfo: RouteInfo) -> CPTravelEstimates? {
    let measurement = Measurement(value: routeInfo.distanceToTurn, unit: routeInfo.turnUnits)
    return CPTravelEstimates(distanceRemaining: measurement, timeRemaining: 0.0)
  }

  private func createUpcomingManeuvers(with routeInfo: RouteInfo,
                                       content: CarPlayManeuverContent) -> [CPManeuver] {
    var maneuvers = [CPManeuver]()
    let primaryManeuver = CPManeuver()
    primaryManeuver.userInfo = CPConstants.Maneuvers.primary
    let formattedVariants = NavigationInstructionFormatter.carPlayInstructionVariants(
      roadName: routeInfo.roadName,
      roadRef: routeInfo.roadRef,
      junctionRef: routeInfo.junctionRef,
      destinationRef: routeInfo.destinationRef,
      destination: routeInfo.destination,
      isLink: routeInfo.isLink,
      isLeftHandTraffic: routeInfo.isLeftHandTraffic,
      shields: routeInfo.roadShields)
    var variants = formattedVariants.text
    var attributedVariants = formattedVariants.attributed
    // On a roundabout, prefix each variant with the exit to take, e.g. "3rd exit, Main Street"
    // (or "3rd exit" alone when there's no road name).
    if routeInfo.roundExitNumber != 0 {
      let ordinalExitNumber = NumberFormatter.localizedString(from: NSNumber(value: routeInfo.roundExitNumber),
                                                              number: .ordinal)
      let exitNumber = String(format: L("carplay_roundabout_exit"), arguments: [ordinalExitNumber])
      let prefixed = NavigationInstructionFormatter.prefixCarPlayInstructionVariants(
        .init(text: variants, attributed: attributedVariants), with: exitNumber)
      variants = prefixed.text
      attributedVariants = prefixed.attributed
    }
    // CarPlay requires at least one variant; use "" when the turn has no road name.
    primaryManeuver.instructionVariants = variants.isEmpty ? [""] : variants
    if !attributedVariants.isEmpty {
      primaryManeuver.attributedInstructionVariants = attributedVariants
    }
    if let imageName = routeInfo.turnImageName,
      let symbol = CarPlayManeuverSymbol.image(
        named: imageName,
        exitNumber: routeInfo.roundExitNumber == 0 ? nil : routeInfo.roundExitNumber,
        displayScale: displayScale) {
      primaryManeuver.symbolImage = symbol
    }
    if let estimates = createEstimates(routeInfo) {
      primaryManeuver.initialTravelEstimates = estimates
    }
    // Lane guidance for the instrument cluster / any surface that consumes it (not the app screen).
    if #available(iOS 18.0, *) {
      if routeInfo.lanes.isEmpty {
        activeLaneGuidance = nil
      } else {
        let guidance = laneGuidance(for: routeInfo)
        activeLaneGuidance = guidance
        primaryManeuver.linkedLaneGuidance = guidance
      }
    }
    // Structured metadata for the instrument cluster / HUD on supported vehicles.
    if #available(iOS 17.4, *) {
      primaryManeuver.maneuverType = routeInfo.carDirection.cpManeuverType
      primaryManeuver.junctionType = routeInfo.carDirection.cpJunctionType
      // Route-level driving side (from the route's start region)
      primaryManeuver.trafficSide = routeInfo.isLeftHandTraffic ? .left : .right
      if !routeInfo.junctionRef.isEmpty {
        primaryManeuver.highwayExitLabel = routeInfo.junctionRef
      }
    }
    maneuvers.append(primaryManeuver)
    // Lanes must always be the second maneuver supplied to CarPlay, per Developer guidance 2026
    // https://developer.apple.com/download/files/CarPlay-Developer-Guide.pdf
    if !routeInfo.lanes.isEmpty,
      let laneImages = CarPlayLaneSymbol.imageSet(for: routeInfo.lanes, displayScale: displayScale) {
      let laneManeuver = CPManeuver()
      laneManeuver.userInfo = CPConstants.Maneuvers.lanes
      laneManeuver.instructionVariants = []
      laneManeuver.symbolSet = laneImages
      maneuvers.append(laneManeuver)
    }
    // Always provide the next upcoming turn, as you should provide as many meaneuvers as possible
    if let imageName = content.secondaryTurnImageName,
      let symbol = CarPlayManeuverSymbol.image(named: imageName, displayScale: displayScale) {
      let secondaryManeuver = CPManeuver()
      secondaryManeuver.userInfo = CPConstants.Maneuvers.secondary
      secondaryManeuver.instructionVariants = [L("then_turn")]
      secondaryManeuver.symbolImage = symbol
      maneuvers.append(secondaryManeuver)
    }
    return maneuvers
  }

  /// Instruction strings for the upcoming maneuver
  private func instructionVariants(for info: RouteInfo) -> [String] {
    return NavigationInstructionFormatter.instructionVariants(roadName: info.roadName,
                                                              roadRef: info.roadRef,
                                                              junctionRef: info.junctionRef,
                                                              destinationRef: info.destinationRef,
                                                              destination: info.destination,
                                                              isLink: info.isLink)
  }

  @available(iOS 18.0, *)
  private func laneGuidance(for routeInfo: RouteInfo) -> CPLaneGuidance {
    let guidance = CPLaneGuidance()
    guidance.lanes = routeInfo.lanes.map { lane in
      let angles = lane.laneWays.compactMap { LaneWay(rawValue: $0)?.angle }
      // CPLane requires at least one angle; fall back to "straight" for unmarked lanes.
      let safeAngles = angles.isEmpty ? [Measurement(value: 0, unit: UnitAngle.degrees)] : angles
      if let recommended = LaneWay(rawValue: lane.recommendedWay), recommended != .none {
        return CPLane(angles: safeAngles, highlightedAngle: recommended.angle, isPreferred: true)
      }
      return CPLane(angles: safeAngles)
    }
    let variants = instructionVariants(for: routeInfo)
    guidance.instructionVariants = variants.isEmpty ? [""] : variants
    return guidance
  }

  func createTrip(startPoint: MWMRoutePoint, endPoint: MWMRoutePoint, routeInfo: RouteInfo? = nil) -> CPTrip {
    let startPlacemark = MKPlacemark(coordinate: CLLocationCoordinate2D(latitude: startPoint.latitude,
                                                                        longitude: startPoint.longitude))
    let endPlacemark = MKPlacemark(coordinate: CLLocationCoordinate2D(latitude: endPoint.latitude,
                                                                      longitude: endPoint.longitude),
                                   addressDictionary: [CNPostalAddressStreetKey: endPoint.subtitle ?? ""])
    let startItem = MKMapItem(placemark: startPlacemark)
    let endItem = MKMapItem(placemark: endPlacemark)
    endItem.name = endPoint.title

    let routeChoice = CPRouteChoice(summaryVariants: [" "], additionalInformationVariants: [], selectionSummaryVariants: [])
    routeChoice.userInfo = routeInfo

    let trip = CPTrip(origin: startItem, destination: endItem, routeChoices: [routeChoice])
    trip.userInfo = [CPConstants.Trip.start: startPoint, CPConstants.Trip.end: endPoint]
    return trip
  }
}

// MARK: - RoutingManagerListener implementation
extension CarPlayRouter: RoutingManagerListener {
  func updateCameraInfo(isCameraOnRoute: Bool, speedLimitMps limit: Double) {
    CarPlayService.shared.updateCameraUI(isCameraOnRoute: isCameraOnRoute, speedLimitMps: limit < 0 ? nil : limit)
  }

  func processRouteBuilderEvent(with code: RouterResultCode, countries: [String]) {
    guard let trip = previewTrip else {
      return
    }
    switch code {
    case .noError, .hasWarnings:
      let manager = RoutingManager.routingManager
      if manager.isRouteFinished {
        listenerContainer.forEach({
          $0.routeDidFinish(trip)
        })
        return
      }
      if let info = manager.routeInfo {
        previewTrip?.routeChoices.first?.userInfo = info
        if routeSession == nil {
          listenerContainer.forEach({
            $0.didCreateRoute(routeInfo: info,
                              trip: trip)
          })
        } else {
          listenerContainer.forEach({
            $0.didUpdateRouteInfo(info, forTrip: trip)
          })
          updateUpcomingManeuvers(with: info)
        }
      }
    default:
      listenerContainer.forEach({
        $0.didFailureBuildRoute(forTrip: trip, code: code, countries: countries)
      })
    }
  }

  func didLocationUpdate(_ notifications: [String], routeInfo: RouteInfo?) {
    guard let trip = previewTrip else { return }

    let manager = RoutingManager.routingManager
    if manager.isRouteFinished {
      listenerContainer.forEach({
        $0.routeDidFinish(trip)
      })
      return
    }

    guard let routeInfo,
      manager.isRoutingActive else { return }
    let content = CarPlayManeuverContent(routeInfo: routeInfo)
    if maneuverRefreshState.needsRefresh(for: content) {
      updateUpcomingManeuvers(with: routeInfo)
    }
    listenerContainer.forEach({
      $0.didUpdateRouteInfo(routeInfo, forTrip: trip)
    })

    let tts = MWMTextToSpeech.tts()!
    if manager.isOnRoute && tts.active {
      tts.playTurnNotifications(notifications)
      tts.playWarningSound()
    }
  }
}
