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
  struct Images {
    let card: UIImage
    let adaptive: UIImage
  }

  static let canvasSize = CGSize(width: 50, height: 50)
  static let glyphInset: CGFloat = 2
  private static var glyphBoundsCache = [String: CGRect]()

  static func images(named name: String,
                     exitNumber: Int? = nil,
                     displayScale: CGFloat) -> Images? {
    guard let base = UIImage(named: name) else { return nil }
    let bounds = glyphBounds(of: base, named: name)

    let card = render(base, glyphBounds: bounds, tint: .white, exitNumber: exitNumber, displayScale: displayScale)
    let black = render(base, glyphBounds: bounds, tint: .black, exitNumber: exitNumber, displayScale: displayScale)
    let white = render(base, glyphBounds: bounds, tint: .white, exitNumber: exitNumber, displayScale: displayScale)

    let asset = UIImageAsset()
    asset.register(black, with: traits(for: .light, scale: displayScale))
    asset.register(white, with: traits(for: .dark, scale: displayScale))
    return Images(card: card, adaptive: asset.image(with: traits(for: .light, scale: displayScale)))
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

  private static func glyphBounds(of base: UIImage, named name: String) -> CGRect {
    if let cached = glyphBoundsCache[name] {
      return cached
    }
    let fullBounds = CGRect(origin: .zero, size: base.size)
    let scale: CGFloat = 4
    let width = Int(ceil(base.size.width * scale))
    let height = Int(ceil(base.size.height * scale))
    guard width > 0, height > 0,
      let context = CGContext(data: nil,
                              width: width,
                              height: height,
                              bitsPerComponent: 8,
                              bytesPerRow: width * 4,
                              space: CGColorSpaceCreateDeviceRGB(),
                              bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue) else {
      return fullBounds
    }
    context.translateBy(x: 0, y: CGFloat(height))
    context.scaleBy(x: scale, y: -scale)
    UIGraphicsPushContext(context)
    base.draw(in: fullBounds)
    UIGraphicsPopContext()

    guard let data = context.data else { return fullBounds }
    let pixels = data.bindMemory(to: UInt8.self, capacity: width * height * 4)
    var minX = width, minY = height, maxX = -1, maxY = -1
    for y in 0..<height {
      for x in 0..<width where pixels[(y * width + x) * 4 + 3] > 25 {
        minX = min(minX, x)
        maxX = max(maxX, x)
        minY = min(minY, y)
        maxY = max(maxY, y)
      }
    }
    guard maxX >= minX, maxY >= minY else { return fullBounds }
    let bounds = CGRect(x: CGFloat(minX) / scale,
                        y: CGFloat(minY) / scale,
                        width: CGFloat(maxX - minX + 1) / scale,
                        height: CGFloat(maxY - minY + 1) / scale)
    glyphBoundsCache[name] = bounds
    return bounds
  }

  private static func render(_ base: UIImage,
                             glyphBounds: CGRect,
                             tint: UIColor,
                             exitNumber: Int?,
                             displayScale: CGFloat) -> UIImage {
    let format = UIGraphicsImageRendererFormat()
    format.scale = displayScale
    format.opaque = false
    let target = CGRect(origin: .zero, size: canvasSize).insetBy(dx: glyphInset, dy: glyphInset)
    let fit = min(target.width / glyphBounds.width, target.height / glyphBounds.height)
    let renderer = UIGraphicsImageRenderer(size: canvasSize, format: format)
    let image = renderer.image { context in
      let cgContext = context.cgContext
      cgContext.translateBy(x: target.midX, y: target.midY)
      cgContext.scaleBy(x: fit, y: fit)
      cgContext.translateBy(x: -glyphBounds.midX, y: -glyphBounds.midY)

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

struct CarPlayLaneManeuverContent: Hashable {
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

struct CarPlayPrimaryManeuverIdentity: Hashable {
  let routeID: UInt64
  let turnIndex: UInt32
}

struct CarPlayLaneSymbolKey: Hashable {
  let identity: CarPlayPrimaryManeuverIdentity
  let lanes: [CarPlayLaneManeuverContent]
}

struct CarPlayManeuverDescription {
  let carDirection: CarDirection
  let exitNumber: Int
  let roadName: String
  let roadRef: String
  let junctionRef: String
  let destinationRef: String
  let destination: String
  let isLeftHandTraffic: Bool

  init(routeInfo: RouteInfo) {
    carDirection = routeInfo.carDirection
    exitNumber = routeInfo.roundExitNumber
    roadName = routeInfo.roadName
    roadRef = routeInfo.roadRef
    junctionRef = routeInfo.junctionRef
    destinationRef = routeInfo.destinationRef
    destination = routeInfo.destination
    isLeftHandTraffic = routeInfo.isLeftHandTraffic
  }

  init(step: CarPlayRouteStep, isLeftHandTraffic: Bool) {
    carDirection = step.carDirection
    exitNumber = step.carDirection.isRoundabout ? step.exitNumber : 0
    roadName = step.roadName
    roadRef = step.roadRef
    junctionRef = step.junctionRef
    destinationRef = step.destinationRef
    destination = step.destination
    self.isLeftHandTraffic = isLeftHandTraffic
  }

  func fallbackInstructionVariants(destinationName: String?) -> [String] {
    guard carDirection == .reachedYourDestination else { return [""] }
    let name = destinationName?.trimmingCharacters(in: .whitespacesAndNewlines) ?? ""
    return [name.isEmpty ? L("pick_destination") : name]
  }
}

@available(iOS 17.4, *)
enum CarPlayInstrumentClusterMetadata {
  static func apply(to maneuver: CPManeuver, description: CarPlayManeuverDescription) {
    maneuver.maneuverType = description.carDirection.cpManeuverType(exitNumber: description.exitNumber)
    maneuver.junctionType = description.carDirection.cpJunctionType
    maneuver.trafficSide = description.isLeftHandTraffic ? .left : .right

    let roadFollowingVariants = NavigationInstructionFormatter.carPlayRoadFollowingManeuverVariants(
      roadName: description.roadName,
      roadRef: description.roadRef,
      destinationRef: description.destinationRef,
      destination: description.destination)
    if !roadFollowingVariants.isEmpty {
      maneuver.roadFollowingManeuverVariants = roadFollowingVariants
    }
    if let exitLabel = NavigationInstructionFormatter.carPlayHighwayExitLabel(
      junctionRef: description.junctionRef) {
      maneuver.highwayExitLabel = exitLabel
    }
  }
}

struct CarPlayRouteStep {
  let turnIndex: UInt32
  let carDirection: CarDirection
  let exitNumber: Int
  let distanceFromStartMeters: Double
  let distanceFromPreviousMeters: Double
  let roadName: String
  let roadRef: String
  let junctionRef: String
  let destinationRef: String
  let destination: String
  let isLink: Bool
  let lanes: [LaneInfo]

  var laneContent: [CarPlayLaneManeuverContent] {
    return lanes.map(CarPlayLaneManeuverContent.init)
  }
}

struct CarPlayRoutePlan {
  static let secondaryManeuverThresholdMeters = 400.0

  let routeID: UInt64
  let steps: [CarPlayRouteStep]

  init(routeID: UInt64, steps: [CarPlayRouteStep]) {
    self.routeID = routeID
    self.steps = steps
  }

  init(routeID: UInt64, stepInfos: [MWMRouteStepInfo]) {
    var distanceFromStart = 0.0
    let steps: [CarPlayRouteStep] = stepInfos.compactMap { info in
      distanceFromStart += info.distMeters
      guard let direction = CarDirection(rawValue: UInt8(truncatingIfNeeded: info.carDirection)) else {
        return nil
      }
      return CarPlayRouteStep(turnIndex: info.turnIndex,
                              carDirection: direction,
                              exitNumber: Int(info.exitNum),
                              distanceFromStartMeters: distanceFromStart,
                              distanceFromPreviousMeters: info.distMeters,
                              roadName: info.toStreetName ?? "",
                              roadRef: info.toRef,
                              junctionRef: info.toJunctionRef,
                              destinationRef: info.toDestinationRef,
                              destination: info.toDestination,
                              isLink: info.toIsLink,
                              lanes: info.lanes.compactMap { $0 as? LaneInfo })
    }
    self.init(routeID: routeID, steps: steps)
  }

  func primaryStepIndex(forTurnIndex turnIndex: UInt32, direction: CarDirection) -> Int? {
    guard let index = steps.firstIndex(where: { $0.turnIndex >= turnIndex }) else { return nil }
    if steps[index].turnIndex == turnIndex {
      return index
    }
    return direction == .enterRoundAbout && steps[index].carDirection == .leaveRoundAbout ? index : nil
  }

  func secondaryStepIndex(after primaryIndex: Int) -> Int? {
    let nextIndex = primaryIndex + 1
    guard nextIndex < steps.count else { return nil }
    let gap = steps[nextIndex].distanceFromStartMeters - steps[primaryIndex].distanceFromStartMeters
    return gap <= Self.secondaryManeuverThresholdMeters ? nextIndex : nil
  }
}

struct CarPlayManeuverContent: Equatable {
  let primaryIdentity: CarPlayPrimaryManeuverIdentity
  let isPlannedPrimary: Bool
  let secondaryTurnIndex: UInt32?
  let lanes: [CarPlayLaneManeuverContent]

  init(routeInfo: RouteInfo, plan: CarPlayRoutePlan?) {
    lanes = routeInfo.lanes.map(CarPlayLaneManeuverContent.init)
    if let plan, plan.routeID == routeInfo.routeID,
       let primaryIndex = plan.primaryStepIndex(forTurnIndex: routeInfo.turnIndex,
                                                direction: routeInfo.carDirection) {
      primaryIdentity = CarPlayPrimaryManeuverIdentity(routeID: plan.routeID,
                                                       turnIndex: plan.steps[primaryIndex].turnIndex)
      isPlannedPrimary = true
      secondaryTurnIndex = plan.secondaryStepIndex(after: primaryIndex).map { plan.steps[$0].turnIndex }
    } else {
      primaryIdentity = CarPlayPrimaryManeuverIdentity(routeID: routeInfo.routeID,
                                                       turnIndex: routeInfo.turnIndex)
      isPlannedPrimary = false
      secondaryTurnIndex = nil
    }
  }
}

enum CarPlayManeuverRefreshReason: String, Equatable {
  case initial
  case reroute
  case routeChanged
  case primaryAdvanced
  case supplementaryChanged
}

enum CarPlayManeuverPhase: String, Equatable {
  case execute
  case prepare
  case initial
  case `continue`

  static let executeDistanceMeters = 30.0
  static let fallbackPrepareDistanceMeters = 150.0
  static let fallbackInitialDistanceMeters = 400.0

  static func phase(distanceMeters: Double,
                    firstNotificationDistanceMeters: Double,
                    secondNotificationDistanceMeters: Double,
                    holdingExecute: Bool) -> CarPlayManeuverPhase {
    if holdingExecute {
      return .execute
    }
    let prepareDistance = secondNotificationDistanceMeters > 0 ? secondNotificationDistanceMeters
                                                               : fallbackPrepareDistanceMeters
    let initialDistance = firstNotificationDistanceMeters > 0 ? max(firstNotificationDistanceMeters, prepareDistance)
                                                              : fallbackInitialDistanceMeters
    let executeDistance = min(executeDistanceMeters, prepareDistance)
    switch distanceMeters {
    case ..<executeDistance: return .execute
    case ..<prepareDistance: return .prepare
    case ..<initialDistance: return .initial
    default: return .continue
    }
  }
}

struct CarPlayEstimatesSnapshot: Equatable {
  private struct WeakTarget {
    weak var object: AnyObject?
  }

  static let timeBucketSeconds = 30.0

  private let targets: [WeakTarget]
  let distance: Double
  let unit: UnitLength
  let timeBucket: Int

  init(targets: [AnyObject], estimates: CPTravelEstimates) {
    self.targets = targets.map { WeakTarget(object: $0) }
    distance = estimates.distanceRemaining.value
    unit = estimates.distanceRemaining.unit
    timeBucket = estimates.timeRemaining < 0 ? -1 : Int(estimates.timeRemaining / Self.timeBucketSeconds)
  }

  static func == (lhs: CarPlayEstimatesSnapshot, rhs: CarPlayEstimatesSnapshot) -> Bool {
    guard lhs.distance == rhs.distance, lhs.unit == rhs.unit, lhs.timeBucket == rhs.timeBucket,
          lhs.targets.count == rhs.targets.count else {
      return false
    }
    return zip(lhs.targets, rhs.targets).allSatisfy { lhsTarget, rhsTarget in
      guard let object = lhsTarget.object else { return false }
      return object === rhsTarget.object
    }
  }
}

struct CarPlayManeuverRefreshState {
  private(set) var displayedContent: CarPlayManeuverContent?

  func reason(for content: CarPlayManeuverContent,
              forcing forcedReason: CarPlayManeuverRefreshReason? = nil) -> CarPlayManeuverRefreshReason? {
    if let forcedReason {
      return forcedReason
    }
    guard let displayedContent else {
      return .initial
    }
    guard displayedContent != content else { return nil }

    if displayedContent.primaryIdentity == content.primaryIdentity {
      return .supplementaryChanged
    }
    let sameRoute = displayedContent.primaryIdentity.routeID == content.primaryIdentity.routeID
    return sameRoute ? .primaryAdvanced : .routeChanged
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
  private var diagnosticNavigationOrigin = "none"
  var diagnosticNavigationContext: String {
    "session=\(CarPlayLogging.diagnosticIdentity(routeSession)) origin={\(diagnosticNavigationOrigin)}"
  }
  private var initialSpeedCamSettings: SpeedCameraManagerMode
  private var maneuverRefreshState = CarPlayManeuverRefreshState()
  private var publishedPrimaryIdentity: CarPlayPrimaryManeuverIdentity?
  private var routePlan: CarPlayRoutePlan?
  private var plannedManeuvers: [UInt32: CPManeuver] = [:]
  private var decoratedPrimaryIdentities = Set<CarPlayPrimaryManeuverIdentity>()
  private var unplannedPrimaryManeuvers: [CarPlayPrimaryManeuverIdentity: CPManeuver] = [:]
  private var laneSymbolManeuvers: [CarPlayLaneSymbolKey: CPManeuver] = [:]
  /// Typed `AnyObject` until we target iOS 18
  private var plannedLaneGuidances: [UInt32: AnyObject] = [:]
  private var liveLaneGuidances: [CarPlayLaneSymbolKey: AnyObject] = [:]
  private var lastObservedIdentity: CarPlayPrimaryManeuverIdentity?
  private var lastObservedDirection: CarDirection?
  private var lastObservedDistanceMeters: Double?
  private var lastManeuverPhase: CarPlayManeuverPhase?
  private var lastRoadNameVariants: [String]?
  private var isMissingPrimaryWarningActive = false
  private var lastManeuverEstimates: CarPlayEstimatesSnapshot?
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
  func startNavigationSession(forTrip trip: CPTrip,
                              template: CPMapTemplate,
                              initialRouteInfo routeInfo: RouteInfo) {
    guard routeSession == nil else {
      let errorMessage = "Route session is already running."
      LOG(.error, "\(CarPlayLogging.carPlay) \(errorMessage)")
      Toast.show(withText: errorMessage, alignment: .top)
      return
    }
    diagnosticNavigationOrigin = "\(CarPlayService.shared.diagnosticConnectionContext) template=\(CarPlayLogging.diagnosticIdentity(template))"
    LOG(.info, "\(CarPlayLogging.carPlay) navigationSession begin \(diagnosticNavigationContext)")
    resetGuidanceState()
    LOG(.info,
        "[CarPlayGuidance] session_started initial=\(identityDescription(routeIdentity(routeInfo))) direction=\(routeInfo.carDirection.diagnosticName) distanceM=\(formattedDistanceMeters(routeInfo))")
    routeSession = template.startNavigationSession(for: trip)
    LOG(.info, "\(CarPlayLogging.carPlay) navigationSession completed \(diagnosticNavigationContext)")
    observeGuidance(routeInfo)
    refreshUpcomingManeuvers(with: routeInfo)
    updateDynamicNavigationState(with: routeInfo)
    LOG(.info,
        "[CarPlayGuidance] session_ready primary=\(identityDescription(publishedPrimaryIdentity)) direction=\(routeInfo.carDirection.diagnosticName) distanceM=\(formattedDistanceMeters(routeInfo)) maneuvers=\(routeSession?.upcomingManeuvers.count ?? 0)")
  }

  func cancelNavigationSession() {
    LOG(.info, "\(CarPlayLogging.carPlay) navigationSession cancel begin \(diagnosticNavigationContext)")
    LOG(.info, "[CarPlayGuidance] session_cancelled last=\(identityDescription(publishedPrimaryIdentity))")
    routeSession?.cancelTrip()
    routeSession = nil
    LOG(.info, "\(CarPlayLogging.carPlay) navigationSession cancelled session=nil origin={\(diagnosticNavigationOrigin)}")
    diagnosticNavigationOrigin = "none"
    resetGuidanceState()
  }

  func cancelTrip() {
    LOG(.info, "\(CarPlayLogging.carPlay) cancelTrip begin")
    cancelNavigationSession()
    completeRouteAndRemovePoints()
    LOG(.info, "\(CarPlayLogging.carPlay) cancelTrip completed")
  }

  func finishTrip() {
    LOG(.info, "\(CarPlayLogging.carPlay) navigationSession finish begin \(diagnosticNavigationContext)")
    LOG(.info, "[CarPlayGuidance] session_finished last=\(identityDescription(publishedPrimaryIdentity))")
    routeSession?.finishTrip()
    routeSession = nil
    LOG(.info, "\(CarPlayLogging.carPlay) navigationSession finish completed session=nil origin={\(diagnosticNavigationOrigin)}")
    diagnosticNavigationOrigin = "none"
    resetGuidanceState()
    completeRouteAndRemovePoints()
  }

  private func resetGuidanceState() {
    maneuverRefreshState.reset()
    publishedPrimaryIdentity = nil
    resetRoutePlan()
    lastObservedIdentity = nil
    lastObservedDirection = nil
    lastObservedDistanceMeters = nil
    lastManeuverPhase = nil
    lastRoadNameVariants = nil
    isMissingPrimaryWarningActive = false
    lastManeuverEstimates = nil
  }

  private func resetRoutePlan() {
    routePlan = nil
    plannedManeuvers = [:]
    decoratedPrimaryIdentities = []
    unplannedPrimaryManeuvers = [:]
    laneSymbolManeuvers = [:]
    plannedLaneGuidances = [:]
    liveLaneGuidances = [:]
  }

  private func installRoutePlanIfNeeded(for routeInfo: RouteInfo) {
    guard let routeSession, routePlan?.routeID != routeInfo.routeID else { return }
    let isReroute = routePlan != nil
    resetRoutePlan()
    let locale = NSLocale.preferredLanguages.first ?? "en"
    let plan = CarPlayRoutePlan(routeID: routeInfo.routeID,
                                stepInfos: MWMRouter.routeSteps(forLocale: locale))
    routePlan = plan

    let metricUnits = routeInfo.turnUnits == .meters || routeInfo.turnUnits == .kilometers
    var maneuvers = [CPManeuver]()
    var laneGuidances = [AnyObject]()
    for step in plan.steps {
      let maneuver = createManeuver(
        description: CarPlayManeuverDescription(step: step, isLeftHandTraffic: routeInfo.isLeftHandTraffic),
        turnImageName: RoutingManager.turnImageName(carDirection: step.carDirection.rawValue, isPrimary: true),
        shields: nil,
        estimates: CPTravelEstimates(distanceRemaining: displayDistance(meters: step.distanceFromPreviousMeters,
                                                                        metric: metricUnits),
                                     timeRemaining: 0.0))
      if #available(iOS 18.0, *), !step.lanes.isEmpty {
        let guidance = laneGuidance(lanes: step.lanes,
                                    description: CarPlayManeuverDescription(step: step,
                                                                            isLeftHandTraffic: routeInfo.isLeftHandTraffic))
        maneuver.linkedLaneGuidance = guidance
        plannedLaneGuidances[step.turnIndex] = guidance
        laneGuidances.append(guidance)
      }
      plannedManeuvers[step.turnIndex] = maneuver
      maneuvers.append(maneuver)
    }
    if #available(iOS 17.4, *) {
      let guidances = laneGuidances as? [CPLaneGuidance] ?? []
      if isReroute, !maneuvers.isEmpty {
        resumeRerouted(routeSession, plan: plan, maneuvers: maneuvers, laneGuidances: guidances, routeInfo: routeInfo)
      } else {
        if #available(iOS 18.0, *), !guidances.isEmpty {
          routeSession.add(guidances)
        }
        if !maneuvers.isEmpty {
          routeSession.add(maneuvers)
        }
      }
    }
    let stepList = plan.steps.map { "\($0.turnIndex):\($0.carDirection.diagnosticName)" }.joined(separator: ",")
    LOG(.info,
        "[CarPlayGuidance] steps_installed route=\(plan.routeID) count=\(plan.steps.count) laneGuidances=\(laneGuidances.count) steps=[\(stepList)]")
    if plan.steps.isEmpty {
      LOG(.warning, "[CarPlayGuidance] invariant_failed no_steps route=\(plan.routeID)")
    }
  }

  @available(iOS 17.4, *)
  private func resumeRerouted(_ routeSession: CPNavigationSession,
                              plan: CarPlayRoutePlan,
                              maneuvers: [CPManeuver],
                              laneGuidances: [CPLaneGuidance],
                              routeInfo: RouteInfo) {
    let content = CarPlayManeuverContent(routeInfo: routeInfo, plan: plan)
    let currentManeuver = (content.isPlannedPrimary ? plannedManeuvers[content.primaryIdentity.turnIndex] : nil)
      ?? maneuvers[0]
    let currentLaneGuidance: CPLaneGuidance
    if content.isPlannedPrimary,
       let planned = plannedLaneGuidances[content.primaryIdentity.turnIndex] as? CPLaneGuidance {
      currentLaneGuidance = planned
    } else {
      currentLaneGuidance = CPLaneGuidance()
      currentLaneGuidance.lanes = []
      currentLaneGuidance.instructionVariants = currentManeuver.instructionVariants
    }
    let tripEstimates = CPTravelEstimates(
      distanceRemaining: Measurement(value: routeInfo.targetDistance, unit: routeInfo.targetUnits),
      timeRemaining: routeInfo.timeToTarget)
    let routeInformation = CPRouteInformation(maneuvers: maneuvers,
                                              laneGuidances: laneGuidances,
                                              currentManeuvers: [currentManeuver],
                                              currentLaneGuidance: currentLaneGuidance,
                                              trip: tripEstimates,
                                              maneuverTravelEstimates: CPTravelEstimates(
                                                distanceRemaining: Measurement(value: routeInfo.distanceToTurn,
                                                                               unit: routeInfo.turnUnits),
                                                timeRemaining: 0.0))
    routeSession.pauseTrip(for: .rerouting, description: nil)
    routeSession.resumeTrip(updatedRouteInformation: routeInformation)
    LOG(.info,
        "[CarPlayGuidance] route_resumed route=\(plan.routeID) maneuvers=\(maneuvers.count) laneGuidances=\(laneGuidances.count) current=\(identityDescription(content.primaryIdentity)) planned=\(content.isPlannedPrimary)")
  }

  private func refreshUpcomingManeuvers(
    with routeInfo: RouteInfo,
    forcing forcedReason: CarPlayManeuverRefreshReason? = nil
  ) {
    guard routeSession != nil else { return }
    installRoutePlanIfNeeded(for: routeInfo)
    let content = CarPlayManeuverContent(routeInfo: routeInfo, plan: routePlan)
    guard let reason = maneuverRefreshState.reason(for: content, forcing: forcedReason) else { return }
    updateUpcomingManeuvers(with: routeInfo, content: content, reason: reason)
  }

  private func updateUpcomingManeuvers(with routeInfo: RouteInfo,
                                       content: CarPlayManeuverContent,
                                       reason: CarPlayManeuverRefreshReason) {
    guard let routeSession else { return }
    let previousContent = maneuverRefreshState.displayedContent

    let primaryManeuver: CPManeuver
    if content.isPlannedPrimary, let planned = plannedManeuvers[content.primaryIdentity.turnIndex] {
      decoratePlannedPrimaryIfNeeded(planned, identity: content.primaryIdentity, routeInfo: routeInfo)
      primaryManeuver = planned
    } else {
      LOG(.warning,
          "[CarPlayGuidance] invariant_failed turn_without_step route=\(identityDescription(routeIdentity(routeInfo))) plan=\(routePlan.map { String($0.routeID) } ?? "none") steps=\(routePlan?.steps.count ?? 0)")
      primaryManeuver = unplannedPrimaryManeuver(for: routeInfo, identity: content.primaryIdentity)
    }

    var maneuvers = [primaryManeuver]
    if let laneManeuver = laneSymbolManeuver(for: routeInfo, identity: content.primaryIdentity) {
      maneuvers.append(laneManeuver)
    }
    if let secondaryTurnIndex = content.secondaryTurnIndex,
       let secondaryManeuver = plannedManeuvers[secondaryTurnIndex] {
      maneuvers.append(secondaryManeuver)
    }
    routeSession.upcomingManeuvers = maneuvers
    if #available(iOS 17.4, *) {
      routeSession.currentLaneGuidance = currentLaneGuidance(for: routeInfo, content: content) as? CPLaneGuidance
    }
    maneuverRefreshState.didDisplay(content)
    if previousContent?.primaryIdentity != content.primaryIdentity {
      lastManeuverPhase = nil
    }
    publishedPrimaryIdentity = content.primaryIdentity

    LOG(.info,
        "[CarPlayGuidance] maneuvers_published reason=\(reason.rawValue) previous=\(identityDescription(previousContent?.primaryIdentity)) route=\(identityDescription(routeIdentity(routeInfo)))->step=\(identityDescription(content.primaryIdentity)) planned=\(content.isPlannedPrimary) direction=\(routeInfo.carDirection.diagnosticName) lanes=\(previousContent?.lanes.count ?? 0)->\(content.lanes.count) secondary=\(previousContent?.secondaryTurnIndex.map { String($0) } ?? "none")->\(content.secondaryTurnIndex.map { String($0) } ?? "none") \(secondaryDescription(content)) count=\(maneuvers.count) \(roadDescription(routeInfo))")
    if #available(iOS 18.0, *) {
      logLaneGuidanceTransition(from: previousContent, to: content)
    }
    if routeSession.upcomingManeuvers.first == nil {
      LOG(.warning, "[CarPlayGuidance] invariant_failed missing_primary snapshot=\(identityDescription(content.primaryIdentity))")
    }
  }

  private func decoratePlannedPrimaryIfNeeded(_ maneuver: CPManeuver,
                                              identity: CarPlayPrimaryManeuverIdentity,
                                              routeInfo: RouteInfo) {
    guard routeInfo.roadShields != nil, decoratedPrimaryIdentities.insert(identity).inserted else { return }
    let attributed = instructionVariants(description: CarPlayManeuverDescription(routeInfo: routeInfo),
                                         shields: routeInfo.roadShields).attributed
    if !attributed.isEmpty {
      maneuver.attributedInstructionVariants = attributed
    }
  }

  private func unplannedPrimaryManeuver(for routeInfo: RouteInfo,
                                        identity: CarPlayPrimaryManeuverIdentity) -> CPManeuver {
    if let cached = unplannedPrimaryManeuvers[identity] {
      return cached
    }
    let maneuver = createManeuver(description: CarPlayManeuverDescription(routeInfo: routeInfo),
                                  turnImageName: routeInfo.turnImageName,
                                  shields: routeInfo.roadShields,
                                  estimates: createEstimates(routeInfo))
    unplannedPrimaryManeuvers[identity] = maneuver
    if #available(iOS 17.4, *) {
      routeSession?.add([maneuver])
    }
    return maneuver
  }

  private func laneSymbolManeuver(for routeInfo: RouteInfo,
                                  identity: CarPlayPrimaryManeuverIdentity) -> CPManeuver? {
    guard !routeInfo.lanes.isEmpty else { return nil }
    let key = CarPlayLaneSymbolKey(identity: identity, lanes: routeInfo.lanes.map(CarPlayLaneManeuverContent.init))
    if let cached = laneSymbolManeuvers[key] {
      return cached
    }
    guard let laneImages = CarPlayLaneSymbol.imageSet(for: routeInfo.lanes, displayScale: displayScale) else {
      return nil
    }
    let laneManeuver = CPManeuver()
    laneManeuver.userInfo = CPConstants.Maneuvers.lanes
    laneManeuver.instructionVariants = []
    laneManeuver.symbolSet = laneImages
    laneSymbolManeuvers[key] = laneManeuver
    if #available(iOS 17.4, *) {
      routeSession?.add([laneManeuver])
    }
    return laneManeuver
  }

  private func currentLaneGuidance(for routeInfo: RouteInfo, content: CarPlayManeuverContent) -> AnyObject? {
    guard #available(iOS 18.0, *), !content.lanes.isEmpty else { return nil }
    if content.isPlannedPrimary,
       let step = routePlan?.steps.first(where: { $0.turnIndex == content.primaryIdentity.turnIndex }),
       step.laneContent == content.lanes,
       let planned = plannedLaneGuidances[step.turnIndex] {
      return planned
    }
    let key = CarPlayLaneSymbolKey(identity: content.primaryIdentity, lanes: content.lanes)
    if let cached = liveLaneGuidances[key] {
      return cached
    }
    let guidance = laneGuidance(lanes: routeInfo.lanes, description: CarPlayManeuverDescription(routeInfo: routeInfo))
    routeSession?.add([guidance])
    liveLaneGuidances[key] = guidance
    return guidance
  }

  private func maneuverPhase(for routeInfo: RouteInfo) -> CarPlayManeuverPhase {
    let holdingExecute = routeInfo.carDirection == .leaveRoundAbout && lastManeuverPhase == .execute
    return CarPlayManeuverPhase.phase(
      distanceMeters: distanceInMeters(routeInfo.distanceToTurn, units: routeInfo.turnUnits),
      firstNotificationDistanceMeters: routeInfo.firstNotificationDistanceMeters,
      secondNotificationDistanceMeters: routeInfo.secondNotificationDistanceMeters,
      holdingExecute: holdingExecute)
  }

  private func updateDynamicNavigationState(with routeInfo: RouteInfo) {
    guard let routeSession else { return }
    guard let primaryManeuver = routeSession.upcomingManeuvers.first,
          let estimates = createEstimates(routeInfo) else {
      if !isMissingPrimaryWarningActive {
        LOG(.warning,
            "[CarPlayGuidance] invariant_failed dynamic_update_without_primary snapshot=\(identityDescription(routeIdentity(routeInfo)))")
        isMissingPrimaryWarningActive = true
      }
      return
    }
    isMissingPrimaryWarningActive = false
    let snapshot = CarPlayEstimatesSnapshot(targets: [routeSession, primaryManeuver], estimates: estimates)
    if snapshot != lastManeuverEstimates {
      routeSession.updateEstimates(estimates, for: primaryManeuver)
      lastManeuverEstimates = snapshot
    }

    if #available(iOS 17.4, *) {
      let phase = maneuverPhase(for: routeInfo)
      if phase != lastManeuverPhase {
        switch phase {
        case .execute: routeSession.maneuverState = .execute
        case .prepare: routeSession.maneuverState = .prepare
        case .initial: routeSession.maneuverState = .initial
        case .continue: routeSession.maneuverState = .continue
        }
        LOG(.info,
            "[CarPlayGuidance] maneuver_state_changed from=\(lastManeuverPhase?.rawValue ?? "none") to=\(phase.rawValue) snapshot=\(identityDescription(routeIdentity(routeInfo))) direction=\(routeInfo.carDirection.diagnosticName) distanceM=\(formattedDistanceMeters(routeInfo)) \(roadDescription(routeInfo))")
        lastManeuverPhase = phase
      }
      let roadName = routeInfo.currentRoadName.trimmingCharacters(in: .whitespacesAndNewlines)
      let roadNameVariants = roadName.isEmpty ? [] : [roadName]
      if roadNameVariants != lastRoadNameVariants {
        routeSession.currentRoadNameVariants = roadNameVariants
        lastRoadNameVariants = roadNameVariants
      }
    }
  }

  private func observeGuidance(_ routeInfo: RouteInfo) {
    let identity = routeIdentity(routeInfo)
    let distanceMeters = distanceInMeters(routeInfo.distanceToTurn, units: routeInfo.turnUnits)
    if let previousIdentity = lastObservedIdentity, previousIdentity != identity {
      let sameRoute = previousIdentity.routeID == identity.routeID
      let indexDelta = Int64(identity.turnIndex) - Int64(previousIdentity.turnIndex)
      if sameRoute && indexDelta < 0 {
        LOG(.warning,
            "[CarPlayGuidance] invariant_failed turn_index_moved_backwards previous=\(identityDescription(previousIdentity)) current=\(identityDescription(identity))")
      }
      LOG(.info,
          "[CarPlayGuidance] \(sameRoute ? "turn_advanced" : "route_changed") previous=\(identityDescription(previousIdentity)) current=\(identityDescription(identity)) previousDirection=\(lastObservedDirection?.diagnosticName ?? "none") currentDirection=\(routeInfo.carDirection.diagnosticName) indexDelta=\(indexDelta) previousLastDistanceM=\(formatMeters(lastObservedDistanceMeters)) currentDistanceM=\(formatMeters(distanceMeters)) \(roadDescription(routeInfo))")
    } else if lastObservedIdentity == nil {
      LOG(.info,
          "[CarPlayGuidance] route_observed current=\(identityDescription(identity)) direction=\(routeInfo.carDirection.diagnosticName) distanceM=\(formatMeters(distanceMeters)) \(roadDescription(routeInfo))")
    }
    lastObservedIdentity = identity
    lastObservedDirection = routeInfo.carDirection
    lastObservedDistanceMeters = distanceMeters
  }

  private func routeIdentity(_ routeInfo: RouteInfo) -> CarPlayPrimaryManeuverIdentity {
    return CarPlayPrimaryManeuverIdentity(routeID: routeInfo.routeID, turnIndex: routeInfo.turnIndex)
  }

  private func distanceInMeters(_ distance: Double, units: UnitLength) -> Double {
    return Measurement(value: distance, unit: units).converted(to: .meters).value
  }

  private func displayDistance(meters: Double, metric: Bool) -> Measurement<UnitLength> {
    let distance = Measurement(value: meters, unit: UnitLength.meters)
    if metric {
      return meters < 1000 ? distance : distance.converted(to: .kilometers)
    }
    let miles = distance.converted(to: .miles)
    return miles.value < 0.1 ? distance.converted(to: .feet) : miles
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

  private func secondaryDescription(_ content: CarPlayManeuverContent) -> String {
    guard let plan = routePlan, content.isPlannedPrimary,
          let primary = plan.steps.first(where: { $0.turnIndex == content.primaryIdentity.turnIndex }) else {
      return "secondaryGapM=none"
    }
    guard let secondaryTurnIndex = content.secondaryTurnIndex,
          let secondary = plan.steps.first(where: { $0.turnIndex == secondaryTurnIndex }) else {
      let next = plan.steps.first { $0.turnIndex > primary.turnIndex }
      let gap = next.map { formatMeters($0.distanceFromStartMeters - primary.distanceFromStartMeters) } ?? "none"
      return "secondaryGapM=\(gap) secondaryDirection=none"
    }
    return "secondaryGapM=\(formatMeters(secondary.distanceFromStartMeters - primary.distanceFromStartMeters)) secondaryDirection=\(secondary.carDirection.diagnosticName)"
  }

  private func logLaneGuidanceTransition(from previousContent: CarPlayManeuverContent?,
                                         to content: CarPlayManeuverContent) {
    let action: String
    if content.lanes.isEmpty {
      guard previousContent?.lanes.isEmpty == false else { return }
      action = "cleared"
    } else if previousContent?.lanes.isEmpty ?? true {
      action = "created"
    } else if previousContent?.lanes != content.lanes {
      action = "replaced"
    } else {
      return
    }
    LOG(.info,
        "[CarPlayGuidance] lane_guidance_\(action) snapshot=\(identityDescription(content.primaryIdentity)) lanes=\(previousContent?.lanes.count ?? 0)->\(content.lanes.count)")
  }

  private func createEstimates(_ routeInfo: RouteInfo) -> CPTravelEstimates? {
    let measurement = Measurement(value: routeInfo.distanceToTurn, unit: routeInfo.turnUnits)
    return CPTravelEstimates(distanceRemaining: measurement, timeRemaining: 0.0)
  }

  private func instructionVariants(description: CarPlayManeuverDescription,
                                   shields: RoadShieldInfo?) -> NavigationInstructionFormatter.CarPlayVariants {
    let formattedVariants = NavigationInstructionFormatter.carPlayInstructionVariants(
      roadName: description.roadName,
      roadRef: description.roadRef,
      junctionRef: description.junctionRef,
      destinationRef: description.destinationRef,
      destination: description.destination,
      isLeftHandTraffic: description.isLeftHandTraffic,
      shields: shields)
    // On a roundabout, prefix each variant with the exit to take, e.g. "3rd exit, Main Street"
    // (or "3rd exit" alone when there's no road name).
    guard description.exitNumber != 0 else { return formattedVariants }
    let ordinalExitNumber = NumberFormatter.localizedString(from: NSNumber(value: description.exitNumber),
                                                            number: .ordinal)
    let exitNumber = String(format: L("carplay_roundabout_exit"), arguments: [ordinalExitNumber])
    return NavigationInstructionFormatter.prefixCarPlayInstructionVariants(formattedVariants, with: exitNumber)
  }

  private var destinationName: String? {
    return routeSession?.trip.destination.name
  }

  private func createManeuver(description: CarPlayManeuverDescription,
                              turnImageName: String?,
                              shields: RoadShieldInfo?,
                              estimates: CPTravelEstimates?) -> CPManeuver {
    let maneuver = CPManeuver()
    maneuver.userInfo = CPConstants.Maneuvers.primary
    let variants = instructionVariants(description: description, shields: shields)
    // CarPlay requires at least one variant; use "" when the turn has no road name.
    maneuver.instructionVariants = variants.text.isEmpty
      ? description.fallbackInstructionVariants(destinationName: destinationName)
      : variants.text
    if !variants.attributed.isEmpty {
      maneuver.attributedInstructionVariants = variants.attributed
    }
    if let turnImageName,
      let symbols = CarPlayManeuverSymbol.images(
        named: turnImageName,
        exitNumber: description.exitNumber == 0 ? nil : description.exitNumber,
        displayScale: displayScale) {
      maneuver.symbolImage = symbols.card
      maneuver.dashboardSymbolImage = symbols.adaptive
      maneuver.notificationSymbolImage = symbols.adaptive
    }
    if let estimates {
      maneuver.initialTravelEstimates = estimates
    }
    // Structured metadata for the instrument cluster / HUD on supported vehicles.
    if #available(iOS 17.4, *) {
      CarPlayInstrumentClusterMetadata.apply(to: maneuver, description: description)
    }
    return maneuver
  }

  @available(iOS 18.0, *)
  private func laneGuidance(lanes: [LaneInfo], description: CarPlayManeuverDescription) -> CPLaneGuidance {
    let guidance = CPLaneGuidance()
    guidance.lanes = lanes.map { CarPlayLaneMetadata.lane(for: $0) }
    let variants = NavigationInstructionFormatter.instructionVariants(roadName: description.roadName,
                                                                      roadRef: description.roadRef,
                                                                      junctionRef: description.junctionRef,
                                                                      destinationRef: description.destinationRef,
                                                                      destination: description.destination)
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
            "[CarPlayGuidance] route_installed identity=\(identityDescription(routeIdentity(info))) direction=\(info.carDirection.diagnosticName) distanceM=\(formattedDistanceMeters(info)) \(roadDescription(info))")
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
