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

@available(iOS 18.0, *)
enum CarPlayLaneMetadata {
  static func lane(for laneInfo: LaneInfo) -> CPLane {
    let angles = uniqueAngles(for: laneInfo.laneWays)
    if let recommended = LaneWay(rawValue: laneInfo.recommendedWay), recommended != .none {
      let highlightedAngle = recommended.angle
      let remainingAngles = angles.filter { !haveEqualDegrees($0, highlightedAngle) }
      return CPLane(angles: remainingAngles,
                    highlightedAngle: highlightedAngle,
                    isPreferred: true)
    }

    let safeAngles = angles.isEmpty ? [LaneWay.through.angle] : angles
    return CPLane(angles: safeAngles)
  }

  private static func uniqueAngles(for rawWays: [UInt8]) -> [Measurement<UnitAngle>] {
    var seenDegrees = Set<Double>()
    return rawWays.compactMap { rawWay in
      guard let way = LaneWay(rawValue: rawWay), way != .none else { return nil }
      let angle = way.angle
      let degrees = angle.converted(to: .degrees).value
      guard seenDegrees.insert(degrees).inserted else { return nil }
      return angle
    }
  }

  private static func haveEqualDegrees(_ lhs: Measurement<UnitAngle>,
                                       _ rhs: Measurement<UnitAngle>) -> Bool {
    return lhs.converted(to: .degrees).value == rhs.converted(to: .degrees).value
  }
}

struct CarPlayPrimaryManeuverIdentity: Equatable {
  let routeID: UInt64
  let turnIndex: UInt32
}

@available(iOS 17.4, *)
enum CarPlayInstrumentClusterMetadata {
  static func apply(to maneuver: CPManeuver, routeInfo: RouteInfo) {
    maneuver.maneuverType = routeInfo.carDirection.cpManeuverType
    maneuver.junctionType = routeInfo.carDirection.cpJunctionType
    maneuver.trafficSide = routeInfo.isLeftHandTraffic ? .left : .right

    let roadFollowingVariants = NavigationInstructionFormatter.carPlayRoadFollowingManeuverVariants(
      roadName: routeInfo.roadName,
      roadRef: routeInfo.roadRef,
      destinationRef: routeInfo.destinationRef,
      destination: routeInfo.destination,
      isLink: routeInfo.isLink)
    if !roadFollowingVariants.isEmpty {
      maneuver.roadFollowingManeuverVariants = roadFollowingVariants
    }
    if let exitLabel = NavigationInstructionFormatter.carPlayHighwayExitLabel(
      junctionRef: routeInfo.junctionRef) {
      maneuver.highwayExitLabel = exitLabel
    }
  }
}

struct CarPlayManeuverContent: Equatable {
  let primaryIdentity: CarPlayPrimaryManeuverIdentity
  let carDirection: CarDirection
  let secondaryTurnImageName: String?
  let lanes: [CarPlayLaneManeuverContent]

  init(routeInfo: RouteInfo) {
    primaryIdentity = CarPlayPrimaryManeuverIdentity(routeID: routeInfo.routeID,
                                                     turnIndex: routeInfo.turnIndex)
    carDirection = routeInfo.carDirection
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

enum CarPlayManeuverRefreshReason: String, Equatable {
  case initial
  case reroute
  case routeChanged
  case primaryAdvanced
  case primaryContentChanged
  case supplementaryChanged
  case roundaboutPrimaryRetained
}

enum CarPlayManeuverRefreshDecision: Equatable {
  case none
  case replacePrimary(CarPlayManeuverRefreshReason)
  case retainPrimary(CarPlayManeuverRefreshReason)
}

enum CarPlayManeuverPhase: String, Equatable {
  case execute
  case prepare
  case initial
  case `continue`
}

struct CarPlayManeuverRefreshState {
  private(set) var displayedContent: CarPlayManeuverContent?

  func decision(for content: CarPlayManeuverContent,
                forcing reason: CarPlayManeuverRefreshReason? = nil) -> CarPlayManeuverRefreshDecision {
    if let reason {
      return .replacePrimary(reason)
    }
    guard let displayedContent else {
      return .replacePrimary(.initial)
    }
    guard displayedContent != content else { return .none }

    if displayedContent.primaryIdentity == content.primaryIdentity {
      if displayedContent.carDirection != content.carDirection {
        return .replacePrimary(.primaryContentChanged)
      }
      return .retainPrimary(.supplementaryChanged)
    }

    let sameRoute = displayedContent.primaryIdentity.routeID == content.primaryIdentity.routeID
    // The Apple bridge selects the exit road before entry; after entry that same road becomes
    // the normal next road, so only supplementary content needs to advance here.
    if sameRoute,
       (displayedContent.carDirection == .enterRoundAbout ||
        displayedContent.carDirection == .stayOnRoundAbout),
       content.carDirection == .leaveRoundAbout {
      return .retainPrimary(.roundaboutPrimaryRetained)
    }
    return .replacePrimary(sameRoute ? .primaryAdvanced : .routeChanged)
  }

  mutating func didDisplay(_ content: CarPlayManeuverContent) {
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
  private var publishedPrimaryIdentity: CarPlayPrimaryManeuverIdentity?
  private var isRetainingCombinedRoundaboutPrimary = false
  private var lastObservedContent: CarPlayManeuverContent?
  private var lastObservedDistanceMeters: Double?
  private var lastManeuverPhase: CarPlayManeuverPhase?
  private var isMissingPrimaryWarningActive = false
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
    LOG(.info, "[CarPlayGuidance] session_started")
    routeSession = template.startNavigationSession(for: trip)
    routeSession?.pauseTrip(for: .loading, description: nil)
    resetGuidanceState()
    if let routeInfo = RoutingManager.routingManager.routeInfo {
      observeGuidance(routeInfo)
      refreshUpcomingManeuvers(with: routeInfo)
      updateDynamicNavigationState(with: routeInfo)
    }
  }

  func cancelNavigationSession() {
    LOG(.info, "[CarPlayGuidance] session_cancelled last=\(identityDescription(publishedPrimaryIdentity))")
    routeSession?.cancelTrip()
    routeSession = nil
    activeLaneGuidance = nil
    resetGuidanceState()
  }

  func cancelTrip() {
    LOG(.info, "Cancelling trip")
    cancelNavigationSession()
    completeRouteAndRemovePoints()
  }

  func finishTrip() {
    LOG(.info, "[CarPlayGuidance] session_finished last=\(identityDescription(publishedPrimaryIdentity))")
    routeSession?.finishTrip()
    routeSession = nil
    activeLaneGuidance = nil
    resetGuidanceState()
    completeRouteAndRemovePoints()
  }

  private func resetGuidanceState() {
    maneuverRefreshState.reset()
    publishedPrimaryIdentity = nil
    isRetainingCombinedRoundaboutPrimary = false
    lastObservedContent = nil
    lastObservedDistanceMeters = nil
    lastManeuverPhase = nil
    isMissingPrimaryWarningActive = false
  }

  private func refreshUpcomingManeuvers(
    with routeInfo: RouteInfo,
    forcing reason: CarPlayManeuverRefreshReason? = nil
  ) {
    let content = CarPlayManeuverContent(routeInfo: routeInfo)
    let decision = maneuverRefreshState.decision(for: content, forcing: reason)
    guard decision != .none else { return }
    updateUpcomingManeuvers(with: routeInfo, content: content, decision: decision)
  }

  private func updateUpcomingManeuvers(with routeInfo: RouteInfo,
                                       content: CarPlayManeuverContent,
                                       decision: CarPlayManeuverRefreshDecision) {
    guard let routeSession else { return }
    let previousContent = maneuverRefreshState.displayedContent
    let shouldRetainPrimary: Bool
    let reason: CarPlayManeuverRefreshReason
    switch decision {
    case .none:
      return
    case .replacePrimary(let refreshReason):
      shouldRetainPrimary = false
      reason = refreshReason
    case .retainPrimary(let refreshReason):
      shouldRetainPrimary = true
      reason = refreshReason
    }
    let retainedPrimary = shouldRetainPrimary ? routeSession.upcomingManeuvers.first : nil
    let didRetainPrimary = retainedPrimary != nil
    let maneuvers = createUpcomingManeuvers(with: routeInfo,
                                            content: content,
                                            retainedPrimary: retainedPrimary)
    if #available(iOS 17.4, *) {
      if let guidance = activeLaneGuidance as? CPLaneGuidance {
        routeSession.add([guidance])
      }
      let newManeuvers = retainedPrimary == nil ? maneuvers : Array(maneuvers.dropFirst())
      if !newManeuvers.isEmpty {
        routeSession.add(newManeuvers)
      }
    }
    routeSession.upcomingManeuvers = maneuvers
    if #available(iOS 17.4, *) {
      // Apple requires lane guidance to be added to the session before it becomes current.
      // The add happens above, so publish the new current value (including nil) only now.
      routeSession.currentLaneGuidance = activeLaneGuidance as? CPLaneGuidance
      if #available(iOS 18.0, *) {
        logLaneGuidanceTransition(from: previousContent, to: content)
      }
    }
    maneuverRefreshState.didDisplay(content)
    if !didRetainPrimary {
      publishedPrimaryIdentity = content.primaryIdentity
      isRetainingCombinedRoundaboutPrimary = false
    } else if reason == .roundaboutPrimaryRetained {
      isRetainingCombinedRoundaboutPrimary = true
    }

    LOG(.info,
        "[CarPlayGuidance] maneuvers_published reason=\(reason.rawValue) retainedPrimary=\(didRetainPrimary) previous=\(identityDescription(previousContent?.primaryIdentity)) snapshot=\(identityDescription(content.primaryIdentity)) suppliedPrimary=\(identityDescription(publishedPrimaryIdentity)) direction=\(routeInfo.carDirection.diagnosticName) lanes=\(previousContent?.lanes.count ?? 0)->\(routeInfo.lanes.count) secondary=\(logValue(previousContent?.secondaryTurnImageName ?? "none"))->\(logValue(content.secondaryTurnImageName ?? "none")) \(roadDescription(routeInfo))")

    if routeSession.upcomingManeuvers.first == nil {
      LOG(.warning, "[CarPlayGuidance] invariant_failed missing_primary snapshot=\(identityDescription(content.primaryIdentity))")
    }
    if publishedPrimaryIdentity != content.primaryIdentity && !isRetainingCombinedRoundaboutPrimary {
      LOG(.warning,
          "[CarPlayGuidance] invariant_failed identity_mismatch supplied=\(identityDescription(publishedPrimaryIdentity)) snapshot=\(identityDescription(content.primaryIdentity))")
    }
  }

  private func maneuverPhase(forDistanceToTurn distance: Double,
                             units: UnitLength) -> CarPlayManeuverPhase {
    let meters = distanceInMeters(distance, units: units)
    switch meters {
    case ..<30: return .execute
    case ..<150: return .prepare
    case ..<400: return .initial
    default: return .continue
    }
  }

  private func updateDynamicNavigationState(with routeInfo: RouteInfo) {
    guard let routeSession else { return }
    guard let primaryManeuver = routeSession.upcomingManeuvers.first,
          let estimates = createEstimates(routeInfo) else {
      if !isMissingPrimaryWarningActive {
        LOG(.warning,
            "[CarPlayGuidance] invariant_failed dynamic_update_without_primary snapshot=\(identityDescription(CarPlayManeuverContent(routeInfo: routeInfo).primaryIdentity))")
        isMissingPrimaryWarningActive = true
      }
      return
    }
    isMissingPrimaryWarningActive = false
    routeSession.updateEstimates(estimates, for: primaryManeuver)

    if #available(iOS 17.4, *) {
      let phase = maneuverPhase(forDistanceToTurn: routeInfo.distanceToTurn, units: routeInfo.turnUnits)
      switch phase {
      case .execute: routeSession.maneuverState = .execute
      case .prepare: routeSession.maneuverState = .prepare
      case .initial: routeSession.maneuverState = .initial
      case .continue: routeSession.maneuverState = .continue
      }
      let roadName = routeInfo.currentRoadName.trimmingCharacters(in: .whitespacesAndNewlines)
      routeSession.currentRoadNameVariants = roadName.isEmpty ? [] : [roadName]

      if phase != lastManeuverPhase {
        LOG(.info,
            "[CarPlayGuidance] maneuver_state_changed from=\(lastManeuverPhase?.rawValue ?? "none") to=\(phase.rawValue) snapshot=\(identityDescription(CarPlayManeuverContent(routeInfo: routeInfo).primaryIdentity)) direction=\(routeInfo.carDirection.diagnosticName) distanceM=\(formattedDistanceMeters(routeInfo)) \(roadDescription(routeInfo))")
        lastManeuverPhase = phase
      }
    }
  }

  private func observeGuidance(_ routeInfo: RouteInfo) {
    let content = CarPlayManeuverContent(routeInfo: routeInfo)
    let distanceMeters = distanceInMeters(routeInfo.distanceToTurn, units: routeInfo.turnUnits)
    if let previousContent = lastObservedContent,
       previousContent.primaryIdentity != content.primaryIdentity {
      let sameRoute = previousContent.primaryIdentity.routeID == content.primaryIdentity.routeID
      let indexDelta = Int64(content.primaryIdentity.turnIndex) - Int64(previousContent.primaryIdentity.turnIndex)
      if sameRoute && indexDelta < 0 {
        LOG(.warning,
            "[CarPlayGuidance] invariant_failed turn_index_moved_backwards previous=\(identityDescription(previousContent.primaryIdentity)) current=\(identityDescription(content.primaryIdentity))")
      }
      LOG(.info,
          "[CarPlayGuidance] \(sameRoute ? "turn_advanced" : "route_changed") previous=\(identityDescription(previousContent.primaryIdentity)) current=\(identityDescription(content.primaryIdentity)) previousDirection=\(previousContent.carDirection.diagnosticName) currentDirection=\(content.carDirection.diagnosticName) indexDelta=\(indexDelta) previousLastDistanceM=\(formatMeters(lastObservedDistanceMeters)) currentDistanceM=\(formatMeters(distanceMeters)) \(roadDescription(routeInfo))")
      lastManeuverPhase = nil
    } else if lastObservedContent == nil {
      LOG(.info,
          "[CarPlayGuidance] route_observed current=\(identityDescription(content.primaryIdentity)) direction=\(content.carDirection.diagnosticName) distanceM=\(formatMeters(distanceMeters)) \(roadDescription(routeInfo))")
    }
    lastObservedContent = content
    lastObservedDistanceMeters = distanceMeters
  }

  private func distanceInMeters(_ distance: Double, units: UnitLength) -> Double {
    return Measurement(value: distance, unit: units).converted(to: .meters).value
  }

  private func formattedDistanceMeters(_ routeInfo: RouteInfo) -> String {
    return formatMeters(distanceInMeters(routeInfo.distanceToTurn, units: routeInfo.turnUnits))
  }

  private func formatMeters(_ meters: Double?) -> String {
    guard let meters else { return "none" }
    return String(format: "%.1f", meters)
  }

  private func identityDescription(_ identity: CarPlayPrimaryManeuverIdentity?) -> String {
    guard let identity else { return "none" }
    return "\(identity.routeID):\(identity.turnIndex)"
  }

  private func logValue(_ value: String) -> String {
    let singleLine = value.replacingOccurrences(of: "\n", with: " ")
      .replacingOccurrences(of: "\r", with: " ")
    return "\"\(singleLine)\""
  }

  private func roadDescription(_ routeInfo: RouteInfo) -> String {
    return "currentRoad=\(logValue(routeInfo.currentRoadName)) nextRoad=\(logValue(routeInfo.roadName)) roadRef=\(logValue(routeInfo.roadRef)) junction=\(logValue(routeInfo.junctionRef)) destinationRef=\(logValue(routeInfo.destinationRef)) destination=\(logValue(routeInfo.destination))"
  }

  private func logLaneGuidanceTransition(from previousContent: CarPlayManeuverContent?,
                                         to content: CarPlayManeuverContent) {
    let action: String
    if content.lanes.isEmpty {
      guard previousContent?.lanes.isEmpty == false else { return }
      action = "cleared"
    } else if previousContent?.lanes.isEmpty ?? true {
      action = "created"
    } else {
      action = "replaced"
    }
    LOG(.info,
        "[CarPlayGuidance] lane_guidance_\(action) snapshot=\(identityDescription(content.primaryIdentity)) lanes=\(previousContent?.lanes.count ?? 0)->\(content.lanes.count)")
  }

  private func createEstimates(_ routeInfo: RouteInfo) -> CPTravelEstimates? {
    let measurement = Measurement(value: routeInfo.distanceToTurn, unit: routeInfo.turnUnits)
    return CPTravelEstimates(distanceRemaining: measurement, timeRemaining: 0.0)
  }

  private func createPrimaryManeuver(with routeInfo: RouteInfo) -> CPManeuver {
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
    // Structured metadata for the instrument cluster / HUD on supported vehicles.
    if #available(iOS 17.4, *) {
      CarPlayInstrumentClusterMetadata.apply(to: primaryManeuver, routeInfo: routeInfo)
    }
    return primaryManeuver
  }

  private func updateLaneGuidance(on primaryManeuver: CPManeuver, with routeInfo: RouteInfo) {
    // Lane guidance for the instrument cluster / any surface that consumes it (not the app screen).
    if #available(iOS 18.0, *) {
      if routeInfo.lanes.isEmpty {
        activeLaneGuidance = nil
        primaryManeuver.setValue(nil, forKey: #keyPath(CPManeuver.linkedLaneGuidance))
      } else {
        let guidance = laneGuidance(for: routeInfo)
        activeLaneGuidance = guidance
        primaryManeuver.linkedLaneGuidance = guidance
      }
    }
  }

  private func createUpcomingManeuvers(with routeInfo: RouteInfo,
                                       content: CarPlayManeuverContent,
                                       retainedPrimary: CPManeuver?) -> [CPManeuver] {
    let primaryManeuver = retainedPrimary ?? createPrimaryManeuver(with: routeInfo)
    updateLaneGuidance(on: primaryManeuver, with: routeInfo)
    var maneuvers = [primaryManeuver]
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
    guidance.lanes = routeInfo.lanes.map { CarPlayLaneMetadata.lane(for: $0) }
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
        LOG(.info,
            "[CarPlayGuidance] route_installed identity=\(identityDescription(CarPlayManeuverContent(routeInfo: info).primaryIdentity)) direction=\(info.carDirection.diagnosticName) distanceM=\(formattedDistanceMeters(info)) \(roadDescription(info))")
        if routeSession == nil {
          listenerContainer.forEach({
            $0.didCreateRoute(routeInfo: info,
                              trip: trip)
          })
        } else {
          observeGuidance(info)
          refreshUpcomingManeuvers(with: info, forcing: .reroute)
          updateDynamicNavigationState(with: info)
          listenerContainer.forEach({
            $0.didUpdateRouteInfo(info, forTrip: trip)
          })
        }
      }
    default:
      listenerContainer.forEach({
        $0.didFailureBuildRoute(forTrip: trip, code: code, countries: countries)
      })
    }
  }

  func didLocationUpdate(_ routeNotifications: [String], routeInfo: RouteInfo?) {
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
    observeGuidance(routeInfo)
    refreshUpcomingManeuvers(with: routeInfo)
    updateDynamicNavigationState(with: routeInfo)
    listenerContainer.forEach({
      $0.didUpdateRouteInfo(routeInfo, forTrip: trip)
    })

    let tts = MWMTextToSpeech.tts()!
    if manager.isOnRoute {
      tts.playRouteNotifications(routeNotifications)
      tts.playWarningSound()
    }
  }
}
