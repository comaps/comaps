import UIKit

class BottomMenuItemCell: UITableViewCell {

  private enum Constants {
    static let badgeSpacing: CGFloat = 8
    static let badgeBackgroundWidth: CGFloat = 32
  }

  @IBOutlet private var label: UILabel!
  @IBOutlet private var badgeBackground: UIView!
  @IBOutlet private var badgeCountLabel: UILabel!
  @IBOutlet private var separator: UIView!
  @IBOutlet private var icon: UIImageView!
  @IBOutlet private var badgeSpacingConstraint: NSLayoutConstraint!
  @IBOutlet private var badgeBackgroundWidthConstraint: NSLayoutConstraint!
  var anchorView: UIView {
    get {
      return icon
    }
  }

  private(set) var isEnabled: Bool = true

  func configure(imageName: String, title: String, badgeCount: UInt = .zero, enabled: Bool = true) {
    if imageName == "help" {
      icon.image = Settings.LeftButtonType.help.image
    } else if imageName == "plus" {
      icon.image = Settings.LeftButtonType.addPlace.image
    } else if imageName == "track" {
      icon.image = Settings.LeftButtonType.recordTrack.image
    } else if imageName == "ic_menu_download" || imageName == "ic_menu_donate" {
      icon.image = UIImage(named: imageName)
    } else {
      let configuration = UIImage.SymbolConfiguration(pointSize: 22, weight: .semibold)
      icon.image = UIImage(systemName: imageName, withConfiguration: configuration)!
    }
    
    label.text = title
    badgeBackground.isHidden = badgeCount == 0
    badgeCountLabel.text = "\(badgeCount)"
    if badgeCount == 0 {
      badgeSpacingConstraint.constant = 0
      badgeBackgroundWidthConstraint.constant = 0
    } else {
      badgeSpacingConstraint.constant = Constants.badgeSpacing
      badgeBackgroundWidthConstraint.constant = Constants.badgeBackgroundWidth
    }
    isEnabled = enabled
    icon.setStyleAndApply(isEnabled ? .black : .gray)
    label.setFontStyleAndApply(isEnabled ? .blackPrimary : .blackHint)
  }
}
