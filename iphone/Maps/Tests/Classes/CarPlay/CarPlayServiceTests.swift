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

  func testCreateEstimates() {
    let routeInfo = RouteInfo(routeID: 1,
                              turnIndex: 1,
                              timeToTarget: 100,
                              targetDistance: 25.2,
                              targetUnitsIndex: 1, // km
                              distanceToTurn: 0.5,
                              turnUnitsIndex: 0, // m
                              turnImageName: nil,
                              nextTurnImageName: nil,
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
    let content = CarPlayManeuverContent(routeInfo: makeRouteInfo())
    let nextIdenticalTurn = CarPlayManeuverContent(routeInfo: makeRouteInfo(turnIndex: 2))
    var state = CarPlayManeuverRefreshState()

    XCTAssertEqual(state.decision(for: content), .replacePrimary(.initial))
    state.didDisplay(content)
    XCTAssertEqual(state.decision(for: content), .none)
    XCTAssertEqual(state.decision(for: nextIdenticalTurn), .replacePrimary(.primaryAdvanced),
                   "Consecutive visually identical turns must still replace the primary")
  }

  func testManeuverRefreshStateReplacesPrimaryForNewRouteWithSameTurnIndex() {
    let firstRoute = CarPlayManeuverContent(routeInfo: makeRouteInfo(routeID: 1, turnIndex: 4))
    let reroute = CarPlayManeuverContent(routeInfo: makeRouteInfo(routeID: 2, turnIndex: 4))
    var state = CarPlayManeuverRefreshState()
    state.didDisplay(firstRoute)

    XCTAssertEqual(state.decision(for: reroute), .replacePrimary(.routeChanged))
  }

  func testManeuverRefreshStateRetainsPrimaryForSecondaryChange() {
    let withoutSecondary = CarPlayManeuverContent(routeInfo: makeRouteInfo())
    let withSecondary = CarPlayManeuverContent(
      routeInfo: makeRouteInfo(nextTurnImageName: "ic_cp_simple_right_then"))
    var state = CarPlayManeuverRefreshState()

    state.didDisplay(withoutSecondary)

    XCTAssertEqual(state.decision(for: withSecondary), .retainPrimary(.supplementaryChanged))
    state.didDisplay(withSecondary)
    XCTAssertEqual(state.decision(for: withSecondary), .none)
  }

  func testManeuverRefreshStateIgnoresDistanceOnlyChanges() {
    let far = CarPlayManeuverContent(routeInfo: makeRouteInfo(distanceToTurn: 500))
    let near = CarPlayManeuverContent(routeInfo: makeRouteInfo(distanceToTurn: 20))
    var state = CarPlayManeuverRefreshState()
    state.didDisplay(far)

    XCTAssertEqual(state.decision(for: near), .none)
  }

  func testManeuverRefreshStateRetainsPrimaryForLaneGuidanceChanges() {
    let withoutLanes = CarPlayManeuverContent(routeInfo: makeRouteInfo())
    let leftLane = makeLane(ways: [.left], recommended: .left)
    let rightLane = makeLane(ways: [.right], recommended: .right)
    let withLeftLane = CarPlayManeuverContent(routeInfo: makeRouteInfo(lanes: [leftLane]))
    let withRightLane = CarPlayManeuverContent(routeInfo: makeRouteInfo(lanes: [rightLane]))
    let withUnrecommendedLeftLane = CarPlayManeuverContent(
      routeInfo: makeRouteInfo(lanes: [makeLane(ways: [.left], recommended: .none)]))
    var state = CarPlayManeuverRefreshState()

    state.didDisplay(withoutLanes)
    XCTAssertEqual(state.decision(for: withLeftLane), .retainPrimary(.supplementaryChanged))
    state.didDisplay(withLeftLane)
    XCTAssertEqual(state.decision(for: withRightLane), .retainPrimary(.supplementaryChanged))
    state.didDisplay(withLeftLane)
    XCTAssertEqual(state.decision(for: withUnrecommendedLeftLane), .retainPrimary(.supplementaryChanged))
  }

  func testManeuverRefreshStateResetForcesReplacement() {
    let content = CarPlayManeuverContent(routeInfo: makeRouteInfo())
    var state = CarPlayManeuverRefreshState()
    state.didDisplay(content)
    state.reset()

    XCTAssertEqual(state.decision(for: content), .replacePrimary(.initial))
  }

  func testManeuverRefreshStateCanForceRerouteReplacement() {
    let content = CarPlayManeuverContent(routeInfo: makeRouteInfo())
    var state = CarPlayManeuverRefreshState()
    state.didDisplay(content)

    XCTAssertEqual(state.decision(for: content, forcing: .reroute), .replacePrimary(.reroute))
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
      CarPlayManeuverSymbol.image(named: "ic_cp_simple_left", displayScale: displayScale))
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
      CarPlayManeuverSymbol.image(named: "ic_cp_round", displayScale: displayScale))
    let numbered = try XCTUnwrap(
      CarPlayManeuverSymbol.image(named: "ic_cp_round", exitNumber: 3, displayScale: displayScale))
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
                             nextTurnImageName: String? = nil,
                             lanes: [LaneInfo] = []) -> RouteInfo {
    return RouteInfo(routeID: routeID,
                     turnIndex: turnIndex,
                     timeToTarget: 100,
                     targetDistance: 1,
                     targetUnitsIndex: 1,
                     distanceToTurn: distanceToTurn,
                     turnUnitsIndex: 0,
                     turnImageName: "ic_cp_simple_left",
                     nextTurnImageName: nextTurnImageName,
                     speedMps: 10,
                     speedLimitMps: 50,
                     roundExitNumber: 0,
                     lanes: lanes,
                     roadName: "Main Street",
                     roadRef: "",
                     junctionRef: "",
                     destinationRef: "",
                     destination: "",
                     isLink: false,
                     roadShields: nil,
                     currentRoadName: "Current Street",
                     carDirectionIndex: carDirection.rawValue,
                     isLeftHandTraffic: false)
  }

  private func makeLane(ways: [LaneWay], recommended: LaneWay) -> LaneInfo {
    return LaneInfo(laneWays: ways.map { NSNumber(value: $0.rawValue) },
                    recommendedWay: recommended.rawValue)
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
