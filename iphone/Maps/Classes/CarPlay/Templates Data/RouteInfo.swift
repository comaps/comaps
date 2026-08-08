import CarPlay

/// Possible direction of a single navigation lane.
///
/// > Warning: Raw values MUST match the native ``routing::turns::lanes::LaneWay`` enum
/// > (see libs/routing/lanes/lane_way.hpp for details).
enum LaneWay: UInt8 {
  case none = 0
  case reverseLeft
  case sharpLeft
  case left
  case mergeToLeft
  case slightLeft
  case through
  case slightRight
  case mergeToRight
  case right
  case sharpRight
  case reverseRight

  /// Direction of the lane as a CarPlay angle.
  ///
  /// 0 degrees points straight ahead, negative values turn left and positive values turn right.
  /// Angles from https://developer.apple.com/download/files/CarPlay-Developer-Guide.pdf
  var angle: Measurement<UnitAngle> {
    let degrees: Double
    switch self {
    case .none, .through: degrees = 0
    case .slightLeft, .mergeToLeft: degrees = -45
    case .left: degrees = -90
    case .sharpLeft: degrees = -135
    case .reverseLeft: degrees = -180
    case .slightRight, .mergeToRight: degrees = 45
    case .right: degrees = 90
    case .sharpRight: degrees = 135
    case .reverseRight: degrees = 180
    }
    return Measurement(value: degrees, unit: .degrees)
  }

  /// SF Symbol used to draw this lane direction in the CarPlay lane strip.
  /// Approximate: sharp/normal turns share a glyph
  var symbolName: String {
    switch self {
    case .none, .through: return "arrow.up"
    case .slightLeft, .mergeToLeft: return "arrow.up.left"
    case .left, .sharpLeft: return "arrow.turn.up.left"
    case .reverseLeft: return "arrow.uturn.left"
    case .slightRight, .mergeToRight: return "arrow.up.right"
    case .right, .sharpRight: return "arrow.turn.up.right"
    case .reverseRight: return "arrow.uturn.right"
    }
  }

  /// Name of the turn-arrow asset (NavigationDashboard/Turn/*) used to draw lane guidance
  var turnImageName: String {
    switch self {
    case .none, .through: return "straight"
    case .slightLeft, .mergeToLeft: return "slight_left"
    case .left: return "simple_left"
    case .sharpLeft: return "sharp_left"
    case .reverseLeft: return "uturn_left"
    case .slightRight, .mergeToRight: return "slight_right"
    case .right: return "simple_right"
    case .sharpRight: return "sharp_right"
    case .reverseRight: return "uturn_right"
    }
  }
}

/// High-level driving maneuver for the upcoming turn.
///
/// > Warning: Raw values MUST match the native ``routing::turns::CarDirection`` enum
/// > (see libs/routing/turns.hpp for details).
enum CarDirection: UInt8 {
  case none = 0
  case goStraight
  case turnRight
  case turnSharpRight
  case turnSlightRight
  case turnLeft
  case turnSharpLeft
  case turnSlightLeft
  case uTurnLeft
  case uTurnRight
  case enterRoundAbout
  case leaveRoundAbout
  case stayOnRoundAbout
  case startAtEndOfStreet
  case reachedYourDestination
  case exitHighwayToLeft
  case exitHighwayToRight

  var isRoundabout: Bool {
    switch self {
    case .enterRoundAbout, .leaveRoundAbout, .stayOnRoundAbout: return true
    default: return false
    }
  }

  /// CarPlay maneuver type for the instrument cluster / HUD.
  @available(iOS 17.4, *)
  var cpManeuverType: CPManeuverType {
    switch self {
    case .none, .goStraight: return .straightAhead
    case .turnRight: return .rightTurn
    case .turnSharpRight: return .sharpRightTurn
    case .turnSlightRight: return .slightRightTurn
    case .turnLeft: return .leftTurn
    case .turnSharpLeft: return .sharpLeftTurn
    case .turnSlightLeft: return .slightLeftTurn
    case .uTurnLeft, .uTurnRight: return .uTurn
    case .enterRoundAbout, .stayOnRoundAbout: return .enterRoundabout
    case .leaveRoundAbout: return .exitRoundabout
    case .startAtEndOfStreet: return .startRoute
    case .reachedYourDestination: return .arriveAtDestination
    case .exitHighwayToLeft: return .highwayOffRampLeft
    case .exitHighwayToRight: return .highwayOffRampRight
    }
  }

  /// CarPlay junction type for the instrument cluster / HUD.
  @available(iOS 17.4, *)
  var cpJunctionType: CPJunctionType {
    return isRoundabout ? .roundabout : .intersection
  }
}

/// A single navigation lane: the directions it allows and, optionally, the recommended one.
@objc(MWMLaneInfo)
final class LaneInfo: NSObject {
  /// Directions this lane allows, as ``LaneWay`` raw values.
  let laneWays: [UInt8]
  /// Recommended direction (``LaneWay/none`` raw value when this lane is not recommended).
  let recommendedWay: UInt8

  @objc init(laneWays: [NSNumber], recommendedWay: UInt8) {
    self.laneWays = laneWays.map { $0.uint8Value }
    self.recommendedWay = recommendedWay
  }
}

/// Instead of the legacy `GetFullRoadName`, build turn instruction strings variants as needed by CarPlay.
/// Ordered longest first, per Apple guidelines.
/// Reused by the phone navigation panel
@objc(MWMNavigationInstructionFormatter)
final class NavigationInstructionFormatter: NSObject {
  struct CarPlayVariants {
    let text: [String]
    let attributed: [NSAttributedString]
  }

  private enum InstructionPart {
    case text(String)
    case roadRef(text: String, shields: [RoadShield], isJunction: Bool)
  }

  private struct InstructionCandidate {
    let parts: [InstructionPart]

    var plainText: String {
      parts.map { part in
        switch part {
        case .text(let text), .roadRef(let text, _, _): return text
        }
      }.joined()
    }

    var hasCompactVariant: Bool {
      parts.contains { part in
        if case .roadRef(_, let shields, _) = part {
          return shields.count > 1
        }
        return false
      }
    }
  }

  @objc static func instructionVariants(roadName: String,
                                        roadRef: String,
                                        junctionRef: String,
                                        destinationRef: String,
                                        destination: String,
                                        isLink: Bool) -> [String] {
    instructionCandidates(roadName: roadName,
                          roadRef: roadRef,
                          junctionRef: junctionRef,
                          destinationRef: destinationRef,
                          destination: destination,
                          isLink: isLink,
                          shields: nil).map(\.plainText)
  }

  static func carPlayInstructionVariants(roadName: String,
                                         roadRef: String,
                                         junctionRef: String,
                                         destinationRef: String,
                                         destination: String,
                                         isLink: Bool,
                                         isLeftHandTraffic: Bool,
                                         shields: RoadShieldInfo?) -> CarPlayVariants {
    let candidates = instructionCandidates(roadName: roadName,
                                           roadRef: roadRef,
                                           junctionRef: junctionRef,
                                           destinationRef: destinationRef,
                                           destination: destination,
                                           isLink: isLink,
                                           shields: shields)
    let text = candidates.map(\.plainText)
    var attributed = [NSAttributedString]()
    var signatures = Set<String>()
    var hasAttachment = false

    for candidate in candidates {
      let rich = render(candidate, primaryShieldOnly: false, isLeftHandTraffic: isLeftHandTraffic)
      if signatures.insert(rich.signature).inserted {
        attributed.append(rich.value)
        hasAttachment = hasAttachment || rich.hasAttachment
      }
      if candidate.hasCompactVariant {
        let compact = render(candidate, primaryShieldOnly: true, isLeftHandTraffic: isLeftHandTraffic)
        if signatures.insert(compact.signature).inserted {
          attributed.append(compact.value)
          hasAttachment = hasAttachment || compact.hasAttachment
        }
      }
    }

    return CarPlayVariants(text: text, attributed: hasAttachment ? attributed : [])
  }

  static func prefixCarPlayInstructionVariants(_ variants: CarPlayVariants, with prefix: String) -> CarPlayVariants {
    let separator = variants.text.isEmpty ? "" : ", "
    let text = variants.text.isEmpty ? [prefix] : variants.text.map { prefix + separator + $0 }
    let attributed = variants.attributed.map { instruction in
      let result = NSMutableAttributedString(string: prefix + separator)
      result.append(instruction)
      return result
    }
    return CarPlayVariants(text: text, attributed: attributed)
  }

  private static func instructionCandidates(roadName: String,
                                            roadRef: String,
                                            junctionRef: String,
                                            destinationRef: String,
                                            destination: String,
                                            isLink: Bool,
                                            shields: RoadShieldInfo?) -> [InstructionCandidate] {
    func clean(_ s: String) -> String { s.trimmingCharacters(in: .whitespacesAndNewlines) }
    func joined(_ groups: [[InstructionPart]], separator: String) -> [InstructionPart] {
      Array(groups.filter { !$0.isEmpty }.joined(separator: [.text(separator)]))
    }

    let name = clean(roadName)
    let ref = clean(roadRef)
    let junctionRef = clean(junctionRef)
    let destinationRef = clean(destinationRef)
    let destination = clean(destination)

    let targetShields = shields?.targetRoadShields ?? []
    let junctionShields = shields?.junctionRoadShields ?? []
    let nameParts: [InstructionPart] = name.isEmpty ? [] : [.text(name)]
    let refParts: [InstructionPart] = ref.isEmpty ? [] : [.roadRef(text: ref,
                                                                  shields: targetShields,
                                                                  isJunction: false)]
    var candidates = [[InstructionPart]]()
    let hasExitInfo = !junctionRef.isEmpty || !destinationRef.isEmpty || !destination.isEmpty
    if isLink || hasExitInfo {
      let exitLabel = junctionRef.isEmpty ? "" : String(format: L("carplay_highway_exit"), junctionRef)
      let exitParts: [InstructionPart] = exitLabel.isEmpty ? [] : [.roadRef(text: exitLabel,
                                                                           shields: junctionShields,
                                                                           isJunction: true)]
      let destinationRefParts: [InstructionPart] = destinationRef.isEmpty ? [] : [
        .roadRef(text: destinationRef, shields: targetShields, isJunction: false),
      ]
      let lead = joined([exitParts, destinationRefParts], separator: ": ")
      // Destinations are "; "-separated; the first one is the primary place.
      let firstDestination = clean(String(destination.split(separator: ";", maxSplits: 1).first ?? ""))
      // Switch out ";" with a nicer separator.
      let destinationList = destination.split(separator: ";").map { clean(String($0)) }.filter { !$0.isEmpty }.joined(separator: " / ")
      let destinationParts: [InstructionPart] = destinationList.isEmpty ? [] : [.text(destinationList)]
      let firstDestinationParts: [InstructionPart] = firstDestination.isEmpty ? [] : [.text(firstDestination)]
      candidates = [
        joined([lead, destinationParts], separator: " → "),
        firstDestination == destination ? [] : joined([lead, firstDestinationParts], separator: " → "),
        lead,
        exitParts,
        // A link with no exit data at all (no junction/destination/ref) would produce nothing,
        // fall back to its plain road name/ref.
        joined([refParts, nameParts], separator: " "),
        nameParts,
      ]
    } else {
      candidates = [
        joined([refParts, nameParts], separator: " "),
        nameParts,
        refParts,
      ]
    }

    // Drop empties, dedupe as variants may be equal, then enforce descending length.
    var seen = Set<String>()
    return candidates
      .map(InstructionCandidate.init(parts:))
      .filter { !$0.plainText.isEmpty && seen.insert($0.plainText).inserted }
      .sorted { $0.plainText.count > $1.plainText.count }
  }

  private static func render(_ candidate: InstructionCandidate,
                             primaryShieldOnly: Bool,
                             isLeftHandTraffic: Bool) -> (value: NSAttributedString,
                                                          signature: String,
                                                          hasAttachment: Bool) {
    let result = NSMutableAttributedString()
    var signature = ""
    var hasAttachment = false

    for part in candidate.parts {
      switch part {
      case .text(let text):
        result.append(NSAttributedString(string: text))
        signature += text
      case .roadRef(let text, let shields, let isJunction):
        let selectedShields = primaryShieldOnly ? Array(shields.prefix(1)) : shields
        if selectedShields.isEmpty {
          result.append(NSAttributedString(string: text))
          signature += text
          continue
        }

        for (index, shield) in selectedShields.enumerated() {
          if index > 0 {
            result.append(NSAttributedString(string: " "))
            signature += " "
          }
          let attachment = NSTextAttachment()
          let image = carPlayShieldImage(for: shield,
                                         isJunction: isJunction,
                                         isLeftHandTraffic: isLeftHandTraffic)
          attachment.image = image
          attachment.bounds = CGRect(origin: .zero, size: image.size)
          result.append(NSAttributedString(attachment: attachment))
          signature += "[\(shield.type.rawValue):\(shield.text)]"
          hasAttachment = true
          if let additionalText = shield.additionalText, !additionalText.isEmpty {
            let additional = " " + additionalText
            result.append(NSAttributedString(string: additional))
            signature += additional
          }
        }
      }
    }
    return (result, signature, hasAttachment)
  }

  private static func carPlayShieldImage(for shield: RoadShield,
                                         isJunction: Bool,
                                         isLeftHandTraffic: Bool) -> UIImage {
    let image = RoadShieldRenderer.image(for: shield,
                                         textSize: 16,
                                         drawOutline: true,
                                         isJunction: isJunction,
                                         isLeftHandTraffic: isLeftHandTraffic)
    let maximumSize = CGSize(width: 64, height: 16)
    let scale = min(1, maximumSize.width / image.size.width, maximumSize.height / image.size.height)
    guard scale < 1 else { return image }

    let size = CGSize(width: floor(image.size.width * scale), height: floor(image.size.height * scale))
    return UIGraphicsImageRenderer(size: size).image { _ in
      image.draw(in: CGRect(origin: .zero, size: size))
    }
  }

  /// Builds an attributed turn instruction with inline drawn road shields, mirroring Android's
  /// `RoadShieldUtils.composeInstruction`. Falls back to `nextStreet` when there is nothing to compose.
  ///
  /// - Parameter textSize: point size of the destination label; shields are drawn slightly smaller.
  /// - Parameter textColor: color for the plain text runs, or `nil` to inherit the label's own color.
  @objc static func attributedInstruction(nextStreet: String,
                                          roadName: String,
                                          roadRef: String,
                                          junctionRef: String,
                                          destinationRef: String,
                                          destination: String,
                                          isLink: Bool,
                                          isLeftHandTraffic: Bool,
                                          shields: RoadShieldInfo?,
                                          textSize: CGFloat,
                                          textColor: UIColor?) -> NSAttributedString {
    if roadName.isEmpty && roadRef.isEmpty && junctionRef.isEmpty && destinationRef.isEmpty && destination.isEmpty {
      return NSAttributedString(string: nextStreet)
    }

    let textAttributes: [NSAttributedString.Key: Any] = textColor.map { [.foregroundColor: $0] } ?? [:]
    let result = NSMutableAttributedString()
    let shieldTextSize = textSize * 0.85
    // Font matching the destination label, used to vertically center shields on the text line.
    let lineFont = UIFont.boldSystemFont(ofSize: textSize)

    func appendText(_ string: String) {
      result.append(NSAttributedString(string: string, attributes: textAttributes))
    }

    func appendShields(_ list: [RoadShield], isJunction: Bool) {
      for (i, shield) in list.enumerated() {
        if i > 0 {
          appendText(" ")
        }
        let image = RoadShieldRenderer.image(for: shield, textSize: shieldTextSize, drawOutline: true,
                                             isJunction: isJunction, isLeftHandTraffic: isLeftHandTraffic)
        let attachment = NSTextAttachment()
        attachment.image = image
        // Center the shield on the font's cap-height midline (bounds.origin.y offsets the image's
        // bottom from the baseline) so it aligns with the surrounding text.
        let yOffset = lineFont.capHeight / 2.0 - image.size.height / 2.0
        attachment.bounds = CGRect(x: 0, y: yOffset, width: image.size.width, height: image.size.height)
        result.append(NSAttributedString(attachment: attachment))
        // additionalText (e.g. a US highway's "East") is drawn next to the shield, not inside it.
        if let additional = shield.additionalText, !additional.isEmpty {
          appendText(" " + additional)
        }
      }
    }

    let hasExitInfo = !junctionRef.isEmpty || !destinationRef.isEmpty || !destination.isEmpty

    // These follow roughly the same rules as GetFullRoadName in routing_session.cpp.
    if isLink || hasExitInfo {
      if !junctionRef.isEmpty {
        if let shields, shields.hasJunctionRoadShields {
          appendShields(shields.junctionRoadShields, isJunction: true)
        } else {
          appendText(junctionRef)
        }
      }

      if !destinationRef.isEmpty {
        if result.length > 0 {
          appendText(": ")
        }
        if let shields, shields.hasTargetRoadShields {
          appendShields(shields.targetRoadShields, isJunction: false)
        } else {
          appendText(destinationRef)
        }
      }

      if !destination.isEmpty {
        if result.length > 0 {
          appendText("\n")
        }
        let parts = destination.split(separator: ";").map { $0.trimmingCharacters(in: .whitespaces) }
        appendText(parts.joined(separator: " / "))
      } else if !roadName.isEmpty {
        if result.length > 0 {
          appendText(" ")
        }
        appendText(roadName)
      }
    } else {
      if !roadRef.isEmpty, let shields, shields.hasTargetRoadShields {
        appendShields(shields.targetRoadShields, isJunction: false)
      }
      if !roadName.isEmpty {
        if result.length > 0 {
          appendText(" ")
        }
        appendText(roadName)
      }
    }

    if result.length == 0 {
      return NSAttributedString(string: nextStreet)
    }

    return result
  }
}

@objc(MWMRouteInfo)
class RouteInfo: NSObject {
  let timeToTarget: TimeInterval
  let targetDistance: Double
  let targetUnits: UnitLength
  let distanceToTurn: Double
  let turnUnits: UnitLength
  let turnImageName: String?
  let nextTurnImageName: String?
  let speedMps: Double
  let speedLimitMps: Double?
  let roundExitNumber: Int
  let lanes: [LaneInfo]
  /// Structured (shield-resolved) components of the next turn's road, mirroring the native
  /// RouteSegment::RoadNameInfo. Used to build width-appropriate CarPlay instruction variants.
  /// Any may be empty.
  let roadName: String
  let roadRef: String
  let junctionRef: String
  let destinationRef: String
  let destination: String
  let isLink: Bool
  let roadShields: RoadShieldInfo?
  let currentRoadName: String
  /// Upcoming maneuver direction, used for CarPlay instrument-cluster metadata.
  let carDirection: CarDirection
  let isLeftHandTraffic: Bool

  @objc init(timeToTarget: TimeInterval,
             targetDistance: Double,
             targetUnitsIndex: UInt8,
             distanceToTurn: Double,
             turnUnitsIndex: UInt8,
             turnImageName: String?,
             nextTurnImageName: String?,
             speedMps: Double,
             speedLimitMps: Double,
             roundExitNumber: Int,
             lanes: [LaneInfo],
             roadName: String,
             roadRef: String,
             junctionRef: String,
             destinationRef: String,
             destination: String,
             isLink: Bool,
             roadShields: RoadShieldInfo?,
             currentRoadName: String,
             carDirectionIndex: UInt8,
             isLeftHandTraffic: Bool) {
    self.timeToTarget = timeToTarget
    self.targetDistance = targetDistance
    self.targetUnits = RouteInfo.unitLength(for: targetUnitsIndex)
    self.distanceToTurn = distanceToTurn
    self.turnUnits = RouteInfo.unitLength(for: turnUnitsIndex)
    self.turnImageName = turnImageName
    self.nextTurnImageName = nextTurnImageName
    self.speedMps = speedMps
    // speedLimitMps >= 0 means known limited speed.
    self.speedLimitMps = speedLimitMps < 0 ? nil : speedLimitMps
    self.roundExitNumber = roundExitNumber
    self.lanes = lanes
    self.roadName = roadName
    self.roadRef = roadRef
    self.junctionRef = junctionRef
    self.destinationRef = destinationRef
    self.destination = destination
    self.isLink = isLink
    self.roadShields = roadShields
    self.currentRoadName = currentRoadName
    self.carDirection = CarDirection(rawValue: carDirectionIndex) ?? .none
    self.isLeftHandTraffic = isLeftHandTraffic
  }


  /// > Warning: Order of enum values MUST BE the same with
  /// > native ``Distance::Units`` enum (see platform/distance.hpp for details).
  class func unitLength(for targetUnitsIndex: UInt8) -> UnitLength {
    switch targetUnitsIndex {
    case 0:
      return .meters
    case 1:
      return .kilometers
    case 2:
      return .feet
    case 3:
      return .miles
    default:
      return .meters
    }
  }
}
