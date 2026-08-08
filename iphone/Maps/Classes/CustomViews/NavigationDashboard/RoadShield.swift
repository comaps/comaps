import UIKit

/// Visual style of a road shield. `ftypes::RoadShieldType` is collapsed onto these styles by
/// `roadShieldType()` in MWMNavigationDashboardManager+Entity.mm; keep the two in sync.
@objc(MWMRoadShieldType)
enum RoadShieldType: Int {
  case genericWhite
  case genericGreen
  case genericBlue
  case genericRed
  case genericOrange
  case usInterstate
  case usHighway
  case ukHighway

  /// Colors kept in sync with the Android `RoadShieldDrawable` and our data/styles.
  fileprivate enum Palette {
    static let white = UIColor(red: 1.0, green: 1.0, blue: 1.0, alpha: 1.0)
    static let black = UIColor(red: 0.0, green: 0.0, blue: 0.0, alpha: 1.0)
    static let yellow = UIColor(red: 1.0, green: 0xD4 / 255.0, blue: 0.0, alpha: 1.0)
    static let blue = UIColor(red: 0x1A / 255.0, green: 0x5E / 255.0, blue: 0xC1 / 255.0, alpha: 1.0)
    static let green = UIColor(red: 0x30 / 255.0, green: 0x93 / 255.0, blue: 0x02 / 255.0, alpha: 1.0)
    static let red = UIColor(red: 0xE6 / 255.0, green: 0x35 / 255.0, blue: 0x34 / 255.0, alpha: 1.0)
    static let orange = UIColor(red: 1.0, green: 0xBE / 255.0, blue: 0.0, alpha: 1.0)
  }

  var textColor: UIColor {
    switch self {
    case .genericWhite: return Palette.black
    case .genericGreen: return Palette.white
    case .genericBlue: return Palette.white
    case .genericRed: return Palette.white
    case .genericOrange: return Palette.black
    case .usInterstate: return Palette.white
    case .usHighway: return Palette.black
    case .ukHighway: return Palette.yellow
    }
  }

  var backgroundColor: UIColor {
    switch self {
    case .genericWhite: return Palette.white
    case .genericGreen: return Palette.green
    case .genericBlue: return Palette.blue
    case .genericRed: return Palette.red
    case .genericOrange: return Palette.orange
    case .usInterstate: return Palette.blue
    case .usHighway: return Palette.white
    case .ukHighway: return Palette.green
    }
  }
}

/// A single road shield resolved from the native `ftypes::RoadShield`.
@objc(MWMRoadShield)
final class RoadShield: NSObject {
  @objc let type: RoadShieldType
  /// Text drawn inside the shield box (already includes any network prefix, e.g. "BR-116").
  @objc let text: String
  /// Text drawn next to the shield, e.g. a US highway's direction "East". May be nil.
  @objc let additionalText: String?

  @objc init(type: RoadShieldType, text: String, additionalText: String?) {
    self.type = type
    self.text = text
    self.additionalText = additionalText
  }
}

/// Road shields for a street, mirroring the native `FollowingInfo::RoadShieldInfo`.
@objc(MWMRoadShieldInfo)
final class RoadShieldInfo: NSObject {
  /// Shields for the target (next) street.
  @objc let targetRoadShields: [RoadShield]
  /// Shields for the junction (exit).
  @objc let junctionRoadShields: [RoadShield]

  @objc init(targetRoadShields: [RoadShield], junctionRoadShields: [RoadShield]) {
    self.targetRoadShields = targetRoadShields
    self.junctionRoadShields = junctionRoadShields
  }

  var hasTargetRoadShields: Bool { !targetRoadShields.isEmpty }
  var hasJunctionRoadShields: Bool { !junctionRoadShields.isEmpty }
}

/// Draws a road shield into a `UIImage`, mirroring Android's `RoadShieldDrawable`.
enum RoadShieldRenderer {
  /// Renders a shield rounded-rect with bold text (and an optional exit arrow for junctions).
  static func image(for shield: RoadShield,
                    textSize: CGFloat,
                    drawOutline: Bool,
                    isJunction: Bool,
                    isLeftHandTraffic: Bool) -> UIImage {
    let displayText: String
    if isJunction {
      let arrow = isLeftHandTraffic ? "\u{2196}" : "\u{2197}"
      displayText = isLeftHandTraffic ? arrow + shield.text : shield.text + arrow
    } else {
      displayText = shield.text
    }

    let font = UIFont.boldSystemFont(ofSize: textSize)
    let attributes: [NSAttributedString.Key: Any] = [.font: font]
    let textSizeBounds = (displayText as NSString).size(withAttributes: attributes)

    let horizontalPadding = textSize * 0.3
    let verticalPadding = textSize * 0.1

    var width = textSizeBounds.width + horizontalPadding * 2
    var height = textSizeBounds.height + verticalPadding * 2
    let drawingOffset = height * 0.1
    let cornerRadius = height / 5.0
    width += drawingOffset
    height += drawingOffset

    let canvasSize = CGSize(width: ceil(width), height: ceil(height))
    let renderer = UIGraphicsImageRenderer(size: canvasSize)
    return renderer.image { _ in
      let rect = CGRect(x: drawingOffset,
                        y: drawingOffset,
                        width: canvasSize.width - drawingOffset * 2,
                        height: canvasSize.height - drawingOffset * 2)
      let path = UIBezierPath(roundedRect: rect, cornerRadius: cornerRadius)
      shield.type.backgroundColor.setFill()
      path.fill()

      if drawOutline {
        shield.type.textColor.setStroke()
        path.lineWidth = canvasSize.height * 0.05
        path.stroke()
      }

      let paragraph = NSMutableParagraphStyle()
      paragraph.alignment = .center
      let textAttributes: [NSAttributedString.Key: Any] = [
        .font: font,
        .foregroundColor: shield.type.textColor,
        .paragraphStyle: paragraph,
      ]
      let textRect = CGRect(x: 0,
                            y: (canvasSize.height - textSizeBounds.height) / 2.0,
                            width: canvasSize.width,
                            height: textSizeBounds.height)
      (displayText as NSString).draw(in: textRect, withAttributes: textAttributes)
    }
  }
}
