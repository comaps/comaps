import XCTest
import UIKit
@testable import CoMaps__Debug_

final class CarPlayServiceTests: XCTestCase {

  var carPlayService: CarPlayService!

  override func setUp() {
    super.setUp()
    carPlayService = CarPlayService()
  }

  override func tearDown() {
    carPlayService = nil
    super.tearDown()
  }

  func testCreateEstimates() {
    let routeInfo = RouteInfo(timeToTarget: 100,
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

  func testInstructionVariants() {
    // Motorway exit: the richest (first) variant joins the exit label, the destination ref and the
    // destinations, and the array is ordered longest-first.
    let exit = NavigationInstructionFormatter.instructionVariants(roadName: "",
                                                                  roadRef: "",
                                                                  junctionRef: "6A",
                                                                  destinationRef: "US 101 South",
                                                                  destination: "San Jose; San Francisco",
                                                                  isLink: true)
    XCTAssertEqual(exit.first, "Exit 6A: US 101 South → San Jose / San Francisco")
    XCTAssertEqual(exit, exit.sorted { $0.count > $1.count })

    // Plain road: ref and name are combined, no brackets.
    let road = NavigationInstructionFormatter.instructionVariants(roadName: "Bayshore Freeway",
                                                                  roadRef: "CA 85",
                                                                  junctionRef: "",
                                                                  destinationRef: "",
                                                                  destination: "",
                                                                  isLink: false)
    XCTAssertEqual(road.first, "CA 85 Bayshore Freeway")

    // No structured data at all yields no variants, so callers keep their fallback.
    let empty = NavigationInstructionFormatter.instructionVariants(roadName: "",
                                                                   roadRef: "",
                                                                   junctionRef: "",
                                                                   destinationRef: "",
                                                                   destination: "",
                                                                   isLink: false)
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
      isLink: false,
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
      isLink: true,
      isLeftHandTraffic: false,
      shields: shields)

    XCTAssertEqual(variants.text.first, "Exit 6A: US 101 South → San Jose / San Francisco")
    XCTAssertEqual(attachments(in: variants.attributed.first!).count, 2)
    XCTAssertTrue(variants.attributed.first!.string.contains("South"))
    XCTAssertTrue(variants.attributed.first!.string.contains("San Jose / San Francisco"))
  }

  func testCarPlayAttributedVariantsRequireShields() {
    let variants = NavigationInstructionFormatter.carPlayInstructionVariants(
      roadName: "Bayshore Freeway",
      roadRef: "CA 85",
      junctionRef: "",
      destinationRef: "",
      destination: "",
      isLink: false,
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
      isLink: false,
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
}
