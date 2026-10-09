import CarPlay
import XCTest
import UIKit
@testable import CoMaps__Debug_

final class CarPlayServiceTests: XCTestCase {

  var carPlayService: CarPlayService!

  override func setUp() {
    super.setUp()
    carPlayService = .shared
  }

  override func tearDown() {
    carPlayService = nil
    super.tearDown()
  }

  func testPanningInterfaceStateIsInitiallyHidden() {
    let state = CarPlayPanningInterfaceState()

    XCTAssertFalse(state.isPresented)
  }

  func testPanningInterfaceStateTracksShowAndDismiss() {
    var state = CarPlayPanningInterfaceState()
    let template = CPMapTemplate()

    state.didShow(template)
    XCTAssertTrue(state.isPresented)
    XCTAssertTrue(state.didDismiss(template))
    XCTAssertFalse(state.isPresented)
  }

  func testPanningInterfaceStateResetClearsPresentedTemplate() {
    var state = CarPlayPanningInterfaceState()
    state.didShow(CPMapTemplate())

    state.reset()

    XCTAssertFalse(state.isPresented)
  }

  func testPanningInterfaceStateIgnoresLateDismissFromReplacedTemplate() {
    var state = CarPlayPanningInterfaceState()
    let replacedTemplate = CPMapTemplate()
    let currentTemplate = CPMapTemplate()
    state.didShow(replacedTemplate)
    state.reset()
    state.didShow(currentTemplate)

    XCTAssertFalse(state.didDismiss(replacedTemplate))
    XCTAssertTrue(state.isPresented)
    XCTAssertTrue(state.didDismiss(currentTemplate))
    XCTAssertFalse(state.isPresented)
  }

  func testSearchContextStateStartsOwnedByPhone() {
    let state = CarPlaySearchContextState()

    XCTAssertEqual(state.owner, .phone)
  }

  func testSearchContextStateResetsOnlyWhenOwnerChanges() {
    var state = CarPlaySearchContextState()

    XCTAssertTrue(state.transition(to: .car))
    XCTAssertEqual(state.owner, .car)
    XCTAssertFalse(state.transition(to: .car))
    XCTAssertTrue(state.transition(to: .phone))
    XCTAssertEqual(state.owner, .phone)
    XCTAssertFalse(state.transition(to: .phone))
  }

  func testSearchContextStateIgnoresTransientHostlessRebind() {
    var state = CarPlaySearchContextState()
    XCTAssertTrue(state.transition(to: .car))

    XCTAssertFalse(state.transition(to: nil))
    XCTAssertEqual(state.owner, .car)
    XCTAssertFalse(state.transition(to: .car))
  }

  func testSearchResultOrderingMovesSelectedResultFirst() {
    let results = [10, 20, 30, 40]

    XCTAssertEqual(CarPlaySearchResultOrdering.selectedFirst(results, selectedIndex: 0),
                   [10, 20, 30, 40])
    XCTAssertEqual(CarPlaySearchResultOrdering.selectedFirst(results, selectedIndex: 2),
                   [30, 10, 20, 40])
    XCTAssertEqual(CarPlaySearchResultOrdering.selectedFirst(results, selectedIndex: 3),
                   [40, 10, 20, 30])
  }

  func testSearchResultOrderingRejectsInvalidIndex() {
    XCTAssertNil(CarPlaySearchResultOrdering.selectedFirst([10, 20], selectedIndex: -1))
    XCTAssertNil(CarPlaySearchResultOrdering.selectedFirst([10, 20], selectedIndex: 2))
    XCTAssertNil(CarPlaySearchResultOrdering.selectedFirst([Int](), selectedIndex: 0))
  }

  func testListLimiterReservesLastRowForOverflowWarning() {
    let items = Array(1 ... 21)

    XCTAssertEqual(CarPlayListLimiter.limit(items,
                                            to: 12,
                                            withOverflowItem: -1),
                   Array(1 ... 11) + [-1])
  }

  func testListLimiterHandlesRestrictionsWithUnchangedReportedMaximum() {
    let items = Array(1 ... 21)
    let reportedMaximum = 24

    for isListLimited in [false, true, false] {
      let effectiveMaximum = CarPlayListLimiter.effectiveMaximumItemCount(reportedMaximum,
                                                                          isListLimited: isListLimited)
      let result = CarPlayListLimiter.limit(items,
                                            to: effectiveMaximum,
                                            withOverflowItem: -1)

      XCTAssertEqual(result, isListLimited ? Array(1 ... 11) + [-1] : items)
    }
  }

  func testListLimiterRespectsReportedMaximumBelowRestrictionLimit() {
    let effectiveMaximum = CarPlayListLimiter.effectiveMaximumItemCount(8, isListLimited: true)

    XCTAssertEqual(effectiveMaximum, 8)
    XCTAssertEqual(CarPlayListLimiter.limit(Array(1 ... 21),
                                            to: effectiveMaximum,
                                            withOverflowItem: -1),
                   Array(1 ... 7) + [-1])
  }

  func testListLimiterReturnsNoItemsAtZeroCapacity() {
    for isListLimited in [false, true] {
      let effectiveMaximum = CarPlayListLimiter.effectiveMaximumItemCount(0,
                                                                          isListLimited: isListLimited)

      XCTAssertEqual(CarPlayListLimiter.limit([1],
                                              to: effectiveMaximum,
                                              withOverflowItem: -1),
                     [])
    }
  }

  func testListLimiterDoesNotTruncateItemsAtOrBelowLimit() {
    let effectiveMaximum = CarPlayListLimiter.effectiveMaximumItemCount(24, isListLimited: true)

    XCTAssertEqual(CarPlayListLimiter.limit(Array(1 ... 12),
                                            to: effectiveMaximum,
                                            withOverflowItem: -1),
                   Array(1 ... 12))
    XCTAssertEqual(CarPlayListLimiter.limit(Array(1 ... 11),
                                            to: effectiveMaximum,
                                            withOverflowItem: -1),
                   Array(1 ... 11))
  }

  func testListItemHandlerCompletesUnknownSelectionExactlyOnce() {
    let item = CPListItem(text: "Unknown", detailText: nil)
    var completionCount = 0
    ListTemplateBuilder.configureSelectionHandler(for: item)

    item.handler?(item, {
      completionCount += 1
    })

    XCTAssertEqual(completionCount, 1)
  }

  func testCreateEstimates() {
    let routeInfo = RouteInfo(routeID: 1,
                              turnIndex: 1,
                              timeToTarget: 100,
                              targetDistance: 25.2,
                              targetUnitsIndex: 1, // km
                              distanceToTurn: 0.5,
                              turnUnitsIndex: 0, // m
                              turnImageName: nil,
                              firstNotificationDistanceMeters: 0,
                              secondNotificationDistanceMeters: 0,
                              speedMps: 40.5,
                              speedLimitMps: 60,
                              roundExitNumber: 0,
                              lanes: [],
                              roadName: "Niamiha",
                              roadRef: "",
                              junctionRef: "",
                              destinationRef: "",
                              destination: "",
                              isLink: false,
                              roadShields: nil,
                              currentRoadName: "Niamiha",
                              carDirectionIndex: 0,
                              isLeftHandTraffic: false)
    let estimates = carPlayService.createEstimates(routeInfo: routeInfo)

    guard let estimates else {
      XCTFail("Estimates should not be nil.")
      return
    }

    XCTAssertEqual(estimates.distanceRemaining, Measurement<UnitLength>(value: 25.2, unit: .kilometers))
    XCTAssertEqual(estimates.timeRemaining, 100)
  }

  func testManeuverRefreshStateUsesPrimaryIdentity() {
    let plan = makePlan([(2, .turnLeft, 0), (3, .turnLeft, 1000)])
    let content = CarPlayManeuverContent(routeInfo: makeRouteInfo(turnIndex: 2), plan: plan)
    let nextIdenticalTurn = CarPlayManeuverContent(routeInfo: makeRouteInfo(turnIndex: 3), plan: plan)
    var state = CarPlayManeuverRefreshState()

    XCTAssertEqual(state.reason(for: content), .initial)
    state.didDisplay(content)
    XCTAssertNil(state.reason(for: content))
    XCTAssertEqual(state.reason(for: nextIdenticalTurn), .primaryAdvanced,
                   "Consecutive visually identical turns must still replace the primary")
  }

  func testManeuverRefreshStateIgnoresDistanceOnlyChanges() {
    let plan = makePlan([(2, .turnLeft, 0)])
    let far = CarPlayManeuverContent(routeInfo: makeRouteInfo(turnIndex: 2, distanceToTurn: 500), plan: plan)
    let near = CarPlayManeuverContent(routeInfo: makeRouteInfo(turnIndex: 2, distanceToTurn: 20), plan: plan)
    var state = CarPlayManeuverRefreshState()
    state.didDisplay(far)

    XCTAssertNil(state.reason(for: near),
                 "Distance updates must update estimates without republishing maneuvers")
  }

  func testManeuverRefreshStateReplacesPrimaryForNewRouteWithSameTurnIndex() {
    let firstRoute = CarPlayManeuverContent(routeInfo: makeRouteInfo(routeID: 1, turnIndex: 4),
                                            plan: makePlan(routeID: 1, [(4, .turnLeft, 0)]))
    let reroute = CarPlayManeuverContent(routeInfo: makeRouteInfo(routeID: 2, turnIndex: 4),
                                         plan: makePlan(routeID: 2, [(4, .turnLeft, 0)]))
    var state = CarPlayManeuverRefreshState()
    state.didDisplay(firstRoute)

    XCTAssertEqual(state.reason(for: reroute), .routeChanged)
  }

  func testManeuverRefreshStateRepublishesForLaneGuidanceChanges() {
    let plan = makePlan([(2, .turnLeft, 0)])
    let withoutLanes = CarPlayManeuverContent(routeInfo: makeRouteInfo(turnIndex: 2), plan: plan)
    let withLeftLane = CarPlayManeuverContent(
      routeInfo: makeRouteInfo(turnIndex: 2, lanes: [makeLane(ways: [.left], recommended: .left)]), plan: plan)
    let withRightLane = CarPlayManeuverContent(
      routeInfo: makeRouteInfo(turnIndex: 2, lanes: [makeLane(ways: [.right], recommended: .right)]), plan: plan)
    var state = CarPlayManeuverRefreshState()

    state.didDisplay(withoutLanes)
    XCTAssertEqual(state.reason(for: withLeftLane), .supplementaryChanged)
    state.didDisplay(withLeftLane)
    XCTAssertEqual(state.reason(for: withRightLane), .supplementaryChanged)
    XCTAssertEqual(state.reason(for: withoutLanes), .supplementaryChanged,
                   "Removing lane guidance must refresh the maneuvers")
  }

  func testManeuverRefreshStateResetForcesInitialPublish() {
    let content = CarPlayManeuverContent(routeInfo: makeRouteInfo(turnIndex: 2), plan: makePlan([(2, .turnLeft, 0)]))
    var state = CarPlayManeuverRefreshState()
    state.didDisplay(content)
    state.reset()

    XCTAssertEqual(state.reason(for: content), .initial)
  }

  func testManeuverRefreshStateCanForceReroute() {
    let content = CarPlayManeuverContent(routeInfo: makeRouteInfo(turnIndex: 2), plan: makePlan([(2, .turnLeft, 0)]))
    var state = CarPlayManeuverRefreshState()
    state.didDisplay(content)

    XCTAssertEqual(state.reason(for: content, forcing: .reroute), .reroute)
  }

  func testRoundaboutEntranceAndExitMapToTheSamePlannedManeuver() {
    let plan = makePlan([(2, .turnLeft, 0), (5, .leaveRoundAbout, 1000), (9, .reachedYourDestination, 3000)])
    let entrance = CarPlayManeuverContent(
      routeInfo: makeRouteInfo(turnIndex: 3, carDirection: .enterRoundAbout), plan: plan)
    let exit = CarPlayManeuverContent(
      routeInfo: makeRouteInfo(turnIndex: 5, carDirection: .leaveRoundAbout), plan: plan)
    var state = CarPlayManeuverRefreshState()
    state.didDisplay(entrance)

    XCTAssertTrue(entrance.isPlannedPrimary)
    XCTAssertEqual(entrance.primaryIdentity.turnIndex, 5)
    XCTAssertEqual(exit.primaryIdentity.turnIndex, 5)
    XCTAssertNil(state.reason(for: exit),
                 "Entering the roundabout must not replace the combined roundabout maneuver")
  }

  func testRoundaboutEntranceOnRouteEndingOnRingIsItsOwnPlannedManeuver() {
    let plan = makePlan([(2, .turnLeft, 0), (3, .enterRoundAbout, 1000), (6, .reachedYourDestination, 1100)])
    let entrance = CarPlayManeuverContent(
      routeInfo: makeRouteInfo(turnIndex: 3, carDirection: .enterRoundAbout), plan: plan)

    XCTAssertTrue(entrance.isPlannedPrimary)
    XCTAssertEqual(entrance.primaryIdentity.turnIndex, 3)
  }

  func testManeuverPhaseFollowsVoicePromptDistances() {
    func phase(_ distance: Double) -> CarPlayManeuverPhase {
      return CarPlayManeuverPhase.phase(distanceMeters: distance,
                                        firstNotificationDistanceMeters: 1000,
                                        secondNotificationDistanceMeters: 120,
                                        holdingExecute: false)
    }

    XCTAssertEqual(phase(1500), .continue)
    XCTAssertEqual(phase(999), .initial)
    XCTAssertEqual(phase(119), .prepare)
    XCTAssertEqual(phase(29), .execute)
  }

  func testManeuverPhaseFallsBackWithoutVoicePromptDistances() {
    func phase(_ distance: Double) -> CarPlayManeuverPhase {
      return CarPlayManeuverPhase.phase(distanceMeters: distance,
                                        firstNotificationDistanceMeters: 0,
                                        secondNotificationDistanceMeters: 0,
                                        holdingExecute: false)
    }

    XCTAssertEqual(phase(400), .continue)
    XCTAssertEqual(phase(399), .initial)
    XCTAssertEqual(phase(149), .prepare)
    XCTAssertEqual(phase(29), .execute)
  }

  func testManeuverPhaseKeepsExecuteWithinShortPrepareDistance() {
    XCTAssertEqual(CarPlayManeuverPhase.phase(distanceMeters: 25,
                                              firstNotificationDistanceMeters: 235,
                                              secondNotificationDistanceMeters: 20,
                                              holdingExecute: false), .initial)
    XCTAssertEqual(CarPlayManeuverPhase.phase(distanceMeters: 19,
                                              firstNotificationDistanceMeters: 235,
                                              secondNotificationDistanceMeters: 20,
                                              holdingExecute: false), .execute)
  }

  func testEstimatesSnapshotIgnoresInvisibleChanges() {
    let maneuver = CPManeuver()
    func snapshot(_ target: AnyObject, meters: Double, seconds: TimeInterval) -> CarPlayEstimatesSnapshot {
      let estimates = CPTravelEstimates(distanceRemaining: Measurement(value: meters, unit: UnitLength.meters),
                                        timeRemaining: seconds)
      return CarPlayEstimatesSnapshot(targets: [target], estimates: estimates)
    }

    XCTAssertEqual(snapshot(maneuver, meters: 380, seconds: 61), snapshot(maneuver, meters: 380, seconds: 89))
    XCTAssertNotEqual(snapshot(maneuver, meters: 380, seconds: 89), snapshot(maneuver, meters: 380, seconds: 90))
    XCTAssertNotEqual(snapshot(maneuver, meters: 380, seconds: 61), snapshot(maneuver, meters: 370, seconds: 61))
    XCTAssertNotEqual(snapshot(maneuver, meters: 380, seconds: 61), snapshot(CPManeuver(), meters: 380, seconds: 61))
    XCTAssertNotEqual(snapshot(maneuver, meters: -1, seconds: -1), snapshot(maneuver, meters: -1, seconds: 0))
  }

  func testEstimatesSnapshotNeverMatchesAReleasedTarget() {
    var maneuver: CPManeuver? = CPManeuver()
    let estimates = CPTravelEstimates(distanceRemaining: Measurement(value: 380, unit: UnitLength.meters),
                                      timeRemaining: 0)
    let previous = CarPlayEstimatesSnapshot(targets: [maneuver!], estimates: estimates)
    maneuver = nil
    let replacement = CPManeuver()

    XCTAssertNotEqual(previous, CarPlayEstimatesSnapshot(targets: [replacement], estimates: estimates))
  }

  func testManeuverPhaseHoldsExecuteInsideRoundabout() {
    XCTAssertEqual(CarPlayManeuverPhase.phase(distanceMeters: 300,
                                              firstNotificationDistanceMeters: 1000,
                                              secondNotificationDistanceMeters: 120,
                                              holdingExecute: true), .execute)
  }

  func testTurnMissingFromPlanDoesNotMapToALaterStep() {
    let plan = makePlan([(2, .turnLeft, 0), (6, .reachedYourDestination, 1100)])
    let entrance = CarPlayManeuverContent(
      routeInfo: makeRouteInfo(turnIndex: 3, carDirection: .enterRoundAbout), plan: plan)
    let turn = CarPlayManeuverContent(
      routeInfo: makeRouteInfo(turnIndex: 4, carDirection: .turnRight), plan: plan)

    XCTAssertFalse(entrance.isPlannedPrimary)
    XCTAssertEqual(entrance.primaryIdentity, CarPlayPrimaryManeuverIdentity(routeID: 1, turnIndex: 3))
    XCTAssertFalse(turn.isPlannedPrimary)
    XCTAssertEqual(turn.primaryIdentity, CarPlayPrimaryManeuverIdentity(routeID: 1, turnIndex: 4))
  }

  func testSecondaryManeuverIsTheNextStepWithinThreshold() {
    let plan = makePlan([(2, .turnLeft, 0), (4, .turnRight, 1000), (6, .turnLeft, 1400),
                         (8, .reachedYourDestination, 1801)])

    XCTAssertNil(CarPlayManeuverContent(routeInfo: makeRouteInfo(turnIndex: 2), plan: plan).secondaryTurnIndex)
    XCTAssertEqual(CarPlayManeuverContent(routeInfo: makeRouteInfo(turnIndex: 4), plan: plan).secondaryTurnIndex, 6)
    XCTAssertNil(CarPlayManeuverContent(routeInfo: makeRouteInfo(turnIndex: 6), plan: plan).secondaryTurnIndex)
    XCTAssertNil(CarPlayManeuverContent(routeInfo: makeRouteInfo(turnIndex: 8), plan: plan).secondaryTurnIndex)
  }

  func testManeuverContentFallsBackToRouteInfoWithoutMatchingPlan() {
    let otherRoutePlan = makePlan(routeID: 7, [(2, .turnLeft, 0)])
    let content = CarPlayManeuverContent(routeInfo: makeRouteInfo(routeID: 1, turnIndex: 2), plan: otherRoutePlan)

    XCTAssertFalse(content.isPlannedPrimary)
    XCTAssertEqual(content.primaryIdentity, CarPlayPrimaryManeuverIdentity(routeID: 1, turnIndex: 2))
    XCTAssertNil(content.secondaryTurnIndex)
  }

  func testRoundaboutManeuverTypeCarriesExitNumber() throws {
    guard #available(iOS 17.4, *) else { throw XCTSkip("Maneuver metadata requires iOS 17.4") }
    XCTAssertEqual(CarDirection.leaveRoundAbout.cpManeuverType(exitNumber: 3), .roundaboutExit3)
    XCTAssertEqual(CarDirection.enterRoundAbout.cpManeuverType(exitNumber: 1), .roundaboutExit1)
    XCTAssertEqual(CarDirection.leaveRoundAbout.cpManeuverType(exitNumber: 19), .roundaboutExit19)
    XCTAssertEqual(CarDirection.leaveRoundAbout.cpManeuverType(exitNumber: 0), .exitRoundabout)
    XCTAssertEqual(CarDirection.leaveRoundAbout.cpManeuverType(exitNumber: 20), .exitRoundabout)
    XCTAssertEqual(CarDirection.turnLeft.cpManeuverType(exitNumber: 3), .leftTurn)
  }

  func testLaneWayTurnImageNames() {
    XCTAssertEqual(LaneWay.through.turnImageName, "straight")
    XCTAssertEqual(LaneWay.none.turnImageName, "straight")
    XCTAssertEqual(LaneWay.left.turnImageName, "simple_left")
    XCTAssertEqual(LaneWay.sharpLeft.turnImageName, "sharp_left")
    XCTAssertEqual(LaneWay.slightLeft.turnImageName, "slight_left")
    XCTAssertEqual(LaneWay.mergeToLeft.turnImageName, "slight_left")
    XCTAssertEqual(LaneWay.reverseLeft.turnImageName, "uturn_left")
    XCTAssertEqual(LaneWay.right.turnImageName, "simple_right")
    XCTAssertEqual(LaneWay.sharpRight.turnImageName, "sharp_right")
    XCTAssertEqual(LaneWay.slightRight.turnImageName, "slight_right")
    XCTAssertEqual(LaneWay.mergeToRight.turnImageName, "slight_right")
    XCTAssertEqual(LaneWay.reverseRight.turnImageName, "uturn_right")
  }

  func testCarPlayManeuverSymbolUsesBlackAndWhiteVariants() throws {
    let displayScale: CGFloat = 2
    let symbol = try XCTUnwrap(
      CarPlayManeuverSymbol.images(named: "ic_cp_simple_left", displayScale: displayScale)).adaptive
    XCTAssertNotNil(symbol.imageAsset)
    XCTAssertEqual(symbol.scale, displayScale)

    let light = CarPlayManeuverSymbol.resolvedVariant(of: symbol, style: .light)
    let dark = CarPlayManeuverSymbol.resolvedVariant(of: symbol, style: .dark)
    XCTAssertLessThan(try averageVisibleLuminance(of: light), 0.1)
    XCTAssertGreaterThan(try averageVisibleLuminance(of: dark), 0.9)
  }

  func testNumberedRoundaboutPreservesBlackAndWhiteVariants() throws {
    let displayScale: CGFloat = 2
    let plain = try XCTUnwrap(
      CarPlayManeuverSymbol.images(named: "ic_cp_round", displayScale: displayScale)).adaptive
    let numbered = try XCTUnwrap(
      CarPlayManeuverSymbol.images(named: "ic_cp_round", exitNumber: 3, displayScale: displayScale)).adaptive
    XCTAssertEqual(numbered.scale, displayScale)

    let light = CarPlayManeuverSymbol.resolvedVariant(of: numbered, style: .light)
    let dark = CarPlayManeuverSymbol.resolvedVariant(of: numbered, style: .dark)
    XCTAssertLessThan(try averageVisibleLuminance(of: light), 0.1)
    XCTAssertGreaterThan(try averageVisibleLuminance(of: dark), 0.9)

    let plainLight = CarPlayManeuverSymbol.resolvedVariant(of: plain, style: .light)
    XCTAssertGreaterThan(try visiblePixelCount(of: light),
                         try visiblePixelCount(of: plainLight),
                         "The exit number should add visible pixels to the roundabout symbol")
  }

  func testCarPlayManeuverCardSymbolIsAlwaysWhite() throws {
    let displayScale: CGFloat = 2
    let card = try XCTUnwrap(
      CarPlayManeuverSymbol.images(named: "ic_cp_simple_left", displayScale: displayScale)).card

    XCTAssertEqual(card.scale, displayScale)
    XCTAssertEqual(card.renderingMode, .alwaysOriginal)
    XCTAssertGreaterThan(try averageVisibleLuminance(of: card), 0.9)
    let resolvedLight = CarPlayManeuverSymbol.resolvedVariant(of: card, style: .light)
    XCTAssertGreaterThan(try averageVisibleLuminance(of: resolvedLight), 0.9)
  }

  func testCarPlayManeuverSymbolIsTrimmedToFillTheCanvas() throws {
    let card = try XCTUnwrap(
      CarPlayManeuverSymbol.images(named: "ic_cp_simple_left", displayScale: 2)).card
    let bounds = try visibleBounds(of: card)
    let cgImage = try XCTUnwrap(card.cgImage)

    XCTAssertEqual(card.size, CarPlayManeuverSymbol.canvasSize)
    XCTAssertGreaterThanOrEqual(bounds.height / CGFloat(cgImage.height), 0.85)
  }

  func testDestinationStepWithoutRoadNameUsesDestinationName() {
    let destination = CarPlayManeuverDescription(
      step: makePlan([(6, .reachedYourDestination, 1100)]).steps[0], isLeftHandTraffic: false)
    let turn = CarPlayManeuverDescription(
      step: makePlan([(4, .turnLeft, 1000)]).steps[0], isLeftHandTraffic: false)

    XCTAssertEqual(destination.fallbackInstructionVariants(destinationName: " Tante "), ["Tante"])
    XCTAssertEqual(destination.fallbackInstructionVariants(destinationName: nil), [L("pick_destination")])
    XCTAssertEqual(destination.fallbackInstructionVariants(destinationName: ""), [L("pick_destination")])
    XCTAssertEqual(turn.fallbackInstructionVariants(destinationName: "Tante"), [""])
  }

  func testMultipleRoadRefsAreSeparatedAndDeduplicated() {
    XCTAssertEqual(NavigationInstructionFormatter.instructionVariants(roadName: "Storoveien",
                                                                      roadRef: "150;Ring 3;Ring 3",
                                                                      junctionRef: "",
                                                                      destinationRef: "",
                                                                      destination: ""),
                   ["150 / Ring 3 Storoveien", "150 / Ring 3", "Storoveien"])
    XCTAssertEqual(NavigationInstructionFormatter.instructionVariants(roadName: "",
                                                                      roadRef: "",
                                                                      junctionRef: "",
                                                                      destinationRef: "E6; E18",
                                                                      destination: "Smestad"),
                   ["E6 / E18 → Smestad", "E6 / E18"])
    XCTAssertEqual(NavigationInstructionFormatter.displayRefs(" 150 ;; Ring 3;150 "), "150 / Ring 3")
  }

  func testCarPlayLaneImageSetUsesWhiteLightContentAndBlackDarkContent() throws {
    let lanes = [
      LaneInfo(laneWays: [NSNumber(value: LaneWay.left.rawValue)],
               recommendedWay: LaneWay.left.rawValue),
      LaneInfo(laneWays: [NSNumber(value: LaneWay.through.rawValue)],
               recommendedWay: LaneWay.none.rawValue),
    ]
    let displayScale: CGFloat = 2
    let imageSet = try XCTUnwrap(
      CarPlayLaneSymbol.imageSet(for: lanes, displayScale: displayScale))

    XCTAssertEqual(imageSet.lightContentImage.size, CGSize(width: 120, height: 18))
    XCTAssertEqual(imageSet.darkContentImage.size, CGSize(width: 120, height: 18))
    XCTAssertEqual(imageSet.lightContentImage.scale, displayScale)
    XCTAssertEqual(imageSet.darkContentImage.scale, displayScale)
    XCTAssertGreaterThan(try averageVisibleLuminance(of: imageSet.lightContentImage), 0.9)
    XCTAssertLessThan(try averageVisibleLuminance(of: imageSet.darkContentImage), 0.1)
  }

  func testPreferredCarPlayLaneSeparatesHighlightedAngleFromRemainingAngles() throws {
    guard #available(iOS 18.0, *) else { throw XCTSkip("Structured lanes require iOS 18") }
    let metadata = CarPlayLaneMetadata.lane(
      for: makeLane(ways: [.through, .right], recommended: .right))
    let highlightedAngle = try XCTUnwrap(metadata.highlightedAngle)

    XCTAssertEqual(degrees(highlightedAngle), 90)
    XCTAssertEqual(metadata.angles.map(degrees), [0])
    XCTAssertFalse(metadata.angles.contains { degrees($0) == degrees(highlightedAngle) })
  }

  func testPreferredCarPlayLaneRemovesEquivalentHighlightedAngles() throws {
    guard #available(iOS 18.0, *) else { throw XCTSkip("Structured lanes require iOS 18") }
    let metadata = CarPlayLaneMetadata.lane(
      for: makeLane(ways: [.slightLeft, .mergeToLeft, .through], recommended: .mergeToLeft))
    let highlightedAngle = try XCTUnwrap(metadata.highlightedAngle)

    XCTAssertEqual(degrees(highlightedAngle), -45)
    XCTAssertEqual(metadata.angles.map(degrees), [0])
  }

  func testUnrecommendedCarPlayLaneDeduplicatesAnglesPreservingOrder() throws {
    guard #available(iOS 18.0, *) else { throw XCTSkip("Structured lanes require iOS 18") }
    let metadata = CarPlayLaneMetadata.lane(
      for: makeLane(ways: [.slightLeft, .mergeToLeft, .through], recommended: .none))

    XCTAssertNil(metadata.highlightedAngle)
    XCTAssertEqual(metadata.angles.map(degrees), [-45, 0])
  }

  func testDirectionlessCarPlayLaneFallsBackToStraightAhead() throws {
    guard #available(iOS 18.0, *) else { throw XCTSkip("Structured lanes require iOS 18") }
    let metadata = CarPlayLaneMetadata.lane(for: makeLane(ways: [], recommended: .none))

    XCTAssertNil(metadata.highlightedAngle)
    XCTAssertEqual(metadata.angles.map(degrees), [0])
  }

  func testSingleDirectionPreferredCarPlayLaneNeedsNoRemainingAngles() throws {
    guard #available(iOS 18.0, *) else { throw XCTSkip("Structured lanes require iOS 18") }
    let metadata = CarPlayLaneMetadata.lane(
      for: makeLane(ways: [.right], recommended: .right))
    let highlightedAngle = try XCTUnwrap(metadata.highlightedAngle)

    XCTAssertEqual(degrees(highlightedAngle), 90)
    XCTAssertTrue(metadata.angles.isEmpty)
  }

  func testInstructionVariants() {
    func variants(junctionRef: String = "",
                  destinationRef: String = "",
                  destination: String = "") -> [String] {
      NavigationInstructionFormatter.instructionVariants(roadName: "Storoveien",
                                                         roadRef: "150",
                                                         junctionRef: junctionRef,
                                                         destinationRef: destinationRef,
                                                         destination: destination)
    }

    XCTAssertEqual(variants(), ["150 Storoveien", "Storoveien", "150"])
    XCTAssertEqual(variants(junctionRef: "67"), ["Exit 67: 150 Storoveien", "Exit 67"])
    XCTAssertEqual(variants(destinationRef: "E6"), ["E6"])
    XCTAssertEqual(variants(destination: "Smestad"), ["Smestad"])
    XCTAssertEqual(variants(junctionRef: "67", destinationRef: "E6"), ["Exit 67: E6", "Exit 67"])
    XCTAssertEqual(variants(junctionRef: "67", destination: "Smestad"),
                   ["Exit 67 → Smestad", "Exit 67"])
    XCTAssertEqual(variants(destinationRef: "E6", destination: "Smestad"), ["E6 → Smestad", "E6"])
    XCTAssertEqual(variants(junctionRef: "67", destinationRef: "E6", destination: "Smestad"),
                   ["Exit 67: E6 → Smestad", "Exit 67: E6", "Exit 67"])

    XCTAssertEqual(variants(junctionRef: "6A",
                            destinationRef: "US 101 South",
                            destination: "San Jose; San Francisco"),
                   ["Exit 6A: US 101 South → San Jose / San Francisco",
                    "Exit 6A: US 101 South → San Jose",
                    "Exit 6A: US 101 South",
                    "Exit 6A"])

    // No structured data at all yields no variants, so callers keep their fallback.
    let empty = NavigationInstructionFormatter.instructionVariants(roadName: "",
                                                                   roadRef: "",
                                                                   junctionRef: "",
                                                                   destinationRef: "",
                                                                   destination: "")
    XCTAssertTrue(empty.isEmpty)
  }

  func testCarPlayRoadFollowingVariantsUseDestinationRefAndCompactDestinations() {
    let variants = NavigationInstructionFormatter.carPlayRoadFollowingManeuverVariants(
      roadName: "Bayshore Freeway",
      roadRef: "US 101",
      destinationRef: "US 101 South",
      destination: "San Jose; San Francisco")

    XCTAssertEqual(variants, [
      "US 101 South → San Jose / San Francisco",
      "US 101 South → San Jose",
      "US 101 South",
    ])
    XCTAssertFalse(variants.contains { $0.contains("Exit") })
  }

  func testCarPlayRoadFollowingVariantsUseDestinationsWithoutRef() {
    let variants = NavigationInstructionFormatter.carPlayRoadFollowingManeuverVariants(
      roadName: "Storoveien",
      roadRef: "150;Ring 3",
      destinationRef: "",
      destination: "Grefsen;Sandaker")

    XCTAssertEqual(variants, ["Grefsen / Sandaker", "Grefsen"])
  }

  func testCarPlayRoadFollowingVariantsFallBackToRoadNameAndRef() {
    let variants = NavigationInstructionFormatter.carPlayRoadFollowingManeuverVariants(
      roadName: "Bayshore Freeway",
      roadRef: "CA 85",
      destinationRef: "",
      destination: "")

    XCTAssertEqual(variants, ["CA 85 Bayshore Freeway", "Bayshore Freeway", "CA 85"])
  }

  func testCarPlayInstrumentClusterMetadataSetsDestinationsAndLocalizedExit() throws {
    guard #available(iOS 17.4, *) else { throw XCTSkip("Navigation metadata requires iOS 17.4") }
    let maneuver = CPManeuver()
    let routeInfo = makeRouteInfo(roadName: "",
                                  roadRef: "",
                                  junctionRef: " 6A ",
                                  destinationRef: "US 101 South",
                                  destination: "San Jose; San Francisco",
                                  isLink: true)

    CarPlayInstrumentClusterMetadata.apply(to: maneuver, description: CarPlayManeuverDescription(routeInfo: routeInfo))

    let roadFollowingVariants = try XCTUnwrap(maneuver.roadFollowingManeuverVariants)
    XCTAssertEqual(roadFollowingVariants, [
      "US 101 South → San Jose / San Francisco",
      "US 101 South → San Jose",
      "US 101 South",
    ])
    XCTAssertEqual(maneuver.highwayExitLabel, "Exit 6A")
    XCTAssertFalse(roadFollowingVariants.contains { $0.contains("Exit 6A") })
  }

  func testCarPlayInstrumentClusterMetadataLeavesEmptyRoadTextUnset() throws {
    guard #available(iOS 17.4, *) else { throw XCTSkip("Navigation metadata requires iOS 17.4") }
    let maneuver = CPManeuver()
    let routeInfo = makeRouteInfo(roadName: "",
                                  roadRef: "",
                                  junctionRef: "",
                                  destinationRef: "",
                                  destination: "",
                                  isLink: false)

    CarPlayInstrumentClusterMetadata.apply(to: maneuver, description: CarPlayManeuverDescription(routeInfo: routeInfo))

    XCTAssertNil(maneuver.roadFollowingManeuverVariants)
    XCTAssertTrue(maneuver.highwayExitLabel.isEmpty)
  }

  func testCarPlayRoadShieldInstructionVariants() {
    let shields = RoadShieldInfo(
      targetRoadShields: [
        RoadShield(type: .genericBlue, text: "SP246", additionalText: nil),
        RoadShield(type: .genericGreen, text: "E 70", additionalText: "East"),
      ],
      junctionRoadShields: [])

    let variants = NavigationInstructionFormatter.carPlayInstructionVariants(
      roadName: "Passo Xon",
      roadRef: "SP246;E 70 East",
      junctionRef: "",
      destinationRef: "",
      destination: "",
      isLeftHandTraffic: false,
      shields: shields)

    XCTAssertEqual(variants.text.first, "SP246;E 70 East Passo Xon")
    XCTAssertFalse(variants.attributed.isEmpty)
    let attachmentCounts = variants.attributed.map { attachments(in: $0).count }
    XCTAssertTrue(attachmentCounts.contains(2), "The richest variant should retain every shield")
    XCTAssertTrue(attachmentCounts.contains(1), "A compact primary-shield variant should be available")
    XCTAssertTrue(variants.attributed.contains { $0.string.contains("East") })

    for instruction in variants.attributed {
      for attachment in attachments(in: instruction) {
        XCTAssertTrue(type(of: attachment) == NSTextAttachment.self)
        guard let image = attachment.image else {
          XCTFail("Every road-shield attachment should contain an image")
          continue
        }
        XCTAssertLessThanOrEqual(image.size.width, 64)
        XCTAssertLessThanOrEqual(image.size.height, 25)
      }
    }
  }

  func testRoadWithoutDestinationUsesShieldedRoadFallback() {
    let shields = RoadShieldInfo(
      targetRoadShields: [
        RoadShield(type: .genericGreen, text: "150", additionalText: nil),
        RoadShield(type: .genericWhite, text: "Ring 3", additionalText: nil),
      ],
      junctionRoadShields: [])

    let variants = NavigationInstructionFormatter.carPlayInstructionVariants(
      roadName: "Storoveien",
      roadRef: "150;Ring 3",
      junctionRef: "",
      destinationRef: "",
      destination: "",
      isLeftHandTraffic: false,
      shields: shields)

    XCTAssertEqual(variants.text.first, "150;Ring 3 Storoveien")
    XCTAssertEqual(attachments(in: variants.attributed.first!).count, 2)
    XCTAssertTrue(variants.attributed.first!.string.contains("Storoveien"))

    let phoneInstruction = NavigationInstructionFormatter.attributedInstruction(
      nextStreet: "Storoveien",
      roadName: "Storoveien",
      roadRef: "150;Ring 3",
      junctionRef: "",
      destinationRef: "",
      destination: "",
      isLeftHandTraffic: false,
      shields: shields,
      textSize: 16,
      textColor: nil)
    XCTAssertEqual(attachments(in: phoneInstruction).count, 2)
    XCTAssertTrue(phoneInstruction.string.contains("Storoveien"))
  }

  func testDestinationWithoutDestinationRefExcludesRoadFallback() {
    let unexpectedRoadShields = RoadShieldInfo(
      targetRoadShields: [
        RoadShield(type: .genericGreen, text: "150", additionalText: nil),
        RoadShield(type: .genericWhite, text: "Ring 3", additionalText: nil),
      ],
      junctionRoadShields: [])

    let variants = NavigationInstructionFormatter.carPlayInstructionVariants(
      roadName: "Storoveien",
      roadRef: "150;Ring 3",
      junctionRef: "",
      destinationRef: "",
      destination: "Grefsen;Sandaker",
      isLeftHandTraffic: false,
      shields: unexpectedRoadShields)

    XCTAssertEqual(variants.text, ["Grefsen / Sandaker", "Grefsen"])
    XCTAssertTrue(variants.attributed.isEmpty)
    XCTAssertFalse(variants.text.contains { $0.contains("Ring 3") || $0.contains("Storoveien") })

    let phoneInstruction = NavigationInstructionFormatter.attributedInstruction(
      nextStreet: "Grefsen / Sandaker",
      roadName: "Storoveien",
      roadRef: "150;Ring 3",
      junctionRef: "",
      destinationRef: "",
      destination: "Grefsen;Sandaker",
      isLeftHandTraffic: false,
      shields: unexpectedRoadShields,
      textSize: 16,
      textColor: nil)
    XCTAssertEqual(phoneInstruction.string, "Grefsen / Sandaker")
    XCTAssertTrue(attachments(in: phoneInstruction).isEmpty)
  }

  func testDestinationRefOnlyUsesShieldAndExcludesRoadFallback() {
    let shields = RoadShieldInfo(
      targetRoadShields: [RoadShield(type: .genericGreen, text: "E6", additionalText: nil)],
      junctionRoadShields: [])

    let variants = NavigationInstructionFormatter.carPlayInstructionVariants(
      roadName: "Storoveien",
      roadRef: "150",
      junctionRef: "",
      destinationRef: "E6",
      destination: "",
      isLeftHandTraffic: false,
      shields: shields)

    XCTAssertEqual(variants.text, ["E6"])
    XCTAssertEqual(variants.attributed.count, 1)
    XCTAssertEqual(attachments(in: variants.attributed[0]).count, 1)
    XCTAssertFalse(variants.attributed[0].string.contains("Storoveien"))

    let phoneInstruction = NavigationInstructionFormatter.attributedInstruction(
      nextStreet: "E6",
      roadName: "Storoveien",
      roadRef: "150",
      junctionRef: "",
      destinationRef: "E6",
      destination: "",
      isLeftHandTraffic: false,
      shields: shields,
      textSize: 16,
      textColor: nil)
    XCTAssertEqual(attachments(in: phoneInstruction).count, 1)
    XCTAssertFalse(phoneInstruction.string.contains("Storoveien"))
  }

  func testDestinationWithoutDestinationRefRetainsJunctionShield() {
    let shields = RoadShieldInfo(
      targetRoadShields: [RoadShield(type: .genericGreen, text: "150", additionalText: nil)],
      junctionRoadShields: [RoadShield(type: .genericGreen, text: "67", additionalText: nil)])

    let variants = NavigationInstructionFormatter.carPlayInstructionVariants(
      roadName: "Storoveien",
      roadRef: "150",
      junctionRef: "67",
      destinationRef: "",
      destination: "Smestad",
      isLeftHandTraffic: false,
      shields: shields)

    XCTAssertEqual(variants.text, ["Exit 67 → Smestad", "Exit 67"])
    XCTAssertEqual(variants.attributed.map { attachments(in: $0).count }, [1, 1])
    XCTAssertTrue(variants.attributed[0].string.contains("Smestad"))
    XCTAssertFalse(variants.attributed.contains { $0.string.contains("Storoveien") })

    let phoneInstruction = NavigationInstructionFormatter.attributedInstruction(
      nextStreet: "Smestad",
      roadName: "Storoveien",
      roadRef: "150",
      junctionRef: "67",
      destinationRef: "",
      destination: "Smestad",
      isLeftHandTraffic: false,
      shields: shields,
      textSize: 16,
      textColor: nil)
    XCTAssertEqual(attachments(in: phoneInstruction).count, 1)
    XCTAssertTrue(phoneInstruction.string.contains("Smestad"))
    XCTAssertFalse(phoneInstruction.string.contains("Storoveien"))
  }

  func testCarPlayExitShieldInstructionVariants() {
    let shields = RoadShieldInfo(
      targetRoadShields: [RoadShield(type: .usHighway, text: "101", additionalText: "South")],
      junctionRoadShields: [RoadShield(type: .genericGreen, text: "6A", additionalText: nil)])

    let variants = NavigationInstructionFormatter.carPlayInstructionVariants(
      roadName: "",
      roadRef: "",
      junctionRef: "6A",
      destinationRef: "US 101 South",
      destination: "San Jose; San Francisco",
      isLeftHandTraffic: false,
      shields: shields)

    XCTAssertEqual(variants.text.first, "Exit 6A: US 101 South → San Jose / San Francisco")
    XCTAssertEqual(attachments(in: variants.attributed.first!).count, 2)
    XCTAssertTrue(variants.attributed.first!.string.contains("South"))
    XCTAssertTrue(variants.attributed.first!.string.contains("San Jose / San Francisco"))
    XCTAssertTrue(variants.text.allSatisfy { $0.contains("Exit 6A") })
    XCTAssertTrue(variants.attributed.allSatisfy { !attachments(in: $0).isEmpty },
                  "Every attributed variant for a numbered exit must retain its junction shield")
    XCTAssertEqual(attachments(in: variants.attributed.last!).count, 1)
    XCTAssertFalse(variants.attributed.last!.string.contains("San Jose"),
                   "The shortest attributed fallback should be the junction shield alone")

    let phoneInstruction = NavigationInstructionFormatter.attributedInstruction(
      nextStreet: "US 101 South > San Jose / San Francisco",
      roadName: "",
      roadRef: "",
      junctionRef: "6A",
      destinationRef: "US 101 South",
      destination: "San Jose; San Francisco",
      isLeftHandTraffic: false,
      shields: shields,
      textSize: 16,
      textColor: nil)
    XCTAssertEqual(attachments(in: phoneInstruction).count, 2)
    XCTAssertTrue(phoneInstruction.string.contains("South"))
    XCTAssertTrue(phoneInstruction.string.contains("San Jose / San Francisco"))
  }

  func testCarPlayAttributedVariantsRequireShields() {
    let variants = NavigationInstructionFormatter.carPlayInstructionVariants(
      roadName: "Bayshore Freeway",
      roadRef: "CA 85",
      junctionRef: "",
      destinationRef: "",
      destination: "",
      isLeftHandTraffic: false,
      shields: nil)

    XCTAssertEqual(variants.text.first, "CA 85 Bayshore Freeway")
    XCTAssertTrue(variants.attributed.isEmpty)
  }

  func testRoundaboutPrefixAppliesToPlainAndAttributedVariants() {
    let shields = RoadShieldInfo(
      targetRoadShields: [RoadShield(type: .genericBlue, text: "SP246", additionalText: nil)],
      junctionRoadShields: [])
    let variants = NavigationInstructionFormatter.carPlayInstructionVariants(
      roadName: "Passo Xon",
      roadRef: "SP246",
      junctionRef: "",
      destinationRef: "",
      destination: "",
      isLeftHandTraffic: false,
      shields: shields)

    let prefixed = NavigationInstructionFormatter.prefixCarPlayInstructionVariants(variants, with: "3rd exit")
    XCTAssertTrue(prefixed.text.allSatisfy { $0.hasPrefix("3rd exit, ") })
    XCTAssertTrue(prefixed.attributed.allSatisfy { $0.string.hasPrefix("3rd exit, ") })
    XCTAssertFalse(attachments(in: prefixed.attributed.first!).isEmpty)
  }

  private func attachments(in attributedString: NSAttributedString) -> [NSTextAttachment] {
    var result = [NSTextAttachment]()
    attributedString.enumerateAttribute(.attachment,
                                        in: NSRange(location: 0, length: attributedString.length)) { value, _, _ in
      if let attachment = value as? NSTextAttachment {
        result.append(attachment)
      }
    }
    return result
  }

  private func makeRouteInfo(routeID: UInt64 = 1,
                             turnIndex: UInt32 = 1,
                             carDirection: CarDirection = .turnLeft,
                             distanceToTurn: Double = 100,
                             firstNotificationDistanceMeters: Double = 0,
                             secondNotificationDistanceMeters: Double = 0,
                             lanes: [LaneInfo] = [],
                             roadName: String = "Main Street",
                             roadRef: String = "",
                             junctionRef: String = "",
                             destinationRef: String = "",
                             destination: String = "",
                             isLink: Bool = false) -> RouteInfo {
    return RouteInfo(routeID: routeID,
                     turnIndex: turnIndex,
                     timeToTarget: 100,
                     targetDistance: 1,
                     targetUnitsIndex: 1,
                     distanceToTurn: distanceToTurn,
                     turnUnitsIndex: 0,
                     turnImageName: "ic_cp_simple_left",
                     firstNotificationDistanceMeters: firstNotificationDistanceMeters,
                     secondNotificationDistanceMeters: secondNotificationDistanceMeters,
                     speedMps: 10,
                     speedLimitMps: 50,
                     roundExitNumber: 0,
                     lanes: lanes,
                     roadName: roadName,
                     roadRef: roadRef,
                     junctionRef: junctionRef,
                     destinationRef: destinationRef,
                     destination: destination,
                     isLink: isLink,
                     roadShields: nil,
                     currentRoadName: "Current Street",
                     carDirectionIndex: carDirection.rawValue,
                     isLeftHandTraffic: false)
  }

  private func makePlan(routeID: UInt64 = 1,
                        _ steps: [(turnIndex: UInt32, direction: CarDirection, distanceFromStart: Double)])
    -> CarPlayRoutePlan {
    var previousDistance = 0.0
    return CarPlayRoutePlan(routeID: routeID, steps: steps.map { step in
      defer { previousDistance = step.distanceFromStart }
      return CarPlayRouteStep(turnIndex: step.turnIndex,
                              carDirection: step.direction,
                              exitNumber: step.direction.isRoundabout ? 2 : 0,
                              distanceFromStartMeters: step.distanceFromStart,
                              distanceFromPreviousMeters: step.distanceFromStart - previousDistance,
                              roadName: "Main Street",
                              roadRef: "",
                              junctionRef: "",
                              destinationRef: "",
                              destination: "",
                              isLink: false,
                              lanes: [])
    })
  }

  private func makeLane(ways: [LaneWay], recommended: LaneWay) -> LaneInfo {
    return LaneInfo(laneWays: ways.map { NSNumber(value: $0.rawValue) },
                    recommendedWay: recommended.rawValue)
  }

  private func degrees(_ angle: Measurement<UnitAngle>) -> Double {
    return angle.converted(to: .degrees).value
  }

  private func averageVisibleLuminance(of image: UIImage) throws -> CGFloat {
    let pixels = try rgbaPixels(of: image)
    var total: CGFloat = 0
    var count: CGFloat = 0
    for index in stride(from: 0, to: pixels.count, by: 4) where pixels[index + 3] > 32 {
      let alpha = CGFloat(pixels[index + 3])
      let red = min(255, CGFloat(pixels[index]) * 255 / alpha)
      let green = min(255, CGFloat(pixels[index + 1]) * 255 / alpha)
      let blue = min(255, CGFloat(pixels[index + 2]) * 255 / alpha)
      total += (0.2126 * red + 0.7152 * green + 0.0722 * blue) / 255
      count += 1
    }
    XCTAssertGreaterThan(count, 0, "The image should contain visible pixels")
    return count == 0 ? 0 : total / count
  }

  private func visiblePixelCount(of image: UIImage) throws -> Int {
    let pixels = try rgbaPixels(of: image)
    return stride(from: 3, to: pixels.count, by: 4).filter { pixels[$0] > 32 }.count
  }

  private func visibleBounds(of image: UIImage) throws -> CGRect {
    let pixels = try rgbaPixels(of: image)
    let width = try XCTUnwrap(image.cgImage).width
    var minX = Int.max, minY = Int.max, maxX = -1, maxY = -1
    for index in stride(from: 3, to: pixels.count, by: 4) where pixels[index] > 32 {
      let pixel = index / 4
      minX = min(minX, pixel % width)
      maxX = max(maxX, pixel % width)
      minY = min(minY, pixel / width)
      maxY = max(maxY, pixel / width)
    }
    XCTAssertGreaterThanOrEqual(maxX, 0, "The image should contain visible pixels")
    guard maxX >= 0 else { return .zero }
    return CGRect(x: minX, y: minY, width: maxX - minX + 1, height: maxY - minY + 1)
  }

  private func rgbaPixels(of image: UIImage) throws -> [UInt8] {
    let cgImage = try XCTUnwrap(image.cgImage)
    let width = cgImage.width
    let height = cgImage.height
    var pixels = [UInt8](repeating: 0, count: width * height * 4)
    let context = try XCTUnwrap(CGContext(data: &pixels,
                                         width: width,
                                         height: height,
                                         bitsPerComponent: 8,
                                         bytesPerRow: width * 4,
                                         space: CGColorSpaceCreateDeviceRGB(),
                                         bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue))
    context.draw(cgImage, in: CGRect(x: 0, y: 0, width: width, height: height))
    return pixels
  }
}
