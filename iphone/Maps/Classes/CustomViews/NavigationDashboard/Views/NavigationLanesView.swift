/// Horizontal strip of upcoming-turn lane recommendations shown in the phone
/// navigation panel.
@objc(NavigationLanesView)
final class NavigationLanesView: UIView {
  private let inactiveLaneAlpha: CGFloat = 0.4

  private let stackView: UIStackView = {
    let stack = UIStackView()
    stack.axis = .horizontal
    stack.distribution = .fillEqually
    stack.alignment = .fill
    stack.spacing = 2
    stack.translatesAutoresizingMaskIntoConstraints = false
    return stack
  }()

  override init(frame: CGRect) {
    super.init(frame: frame)
    setup()
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
    setup()
  }

  private func setup() {
    // Match the adjacent turn box (FirstTurnView style): green guidance background
    // with rounded corners.
    backgroundColor = UIColor.linkBlue()
    layer.cornerRadius = 8
    layer.cornerCurve = .continuous
    layer.masksToBounds = true

    addSubview(stackView)
    NSLayoutConstraint.activate([
      stackView.leadingAnchor.constraint(equalTo: leadingAnchor, constant: 8),
      stackView.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -8),
      stackView.topAnchor.constraint(equalTo: topAnchor, constant: 8),
      stackView.bottomAnchor.constraint(equalTo: bottomAnchor, constant: -8),
    ])
  }

  /// Replaces the displayed lanes. Pass an empty array to hide the strip.
  @objc func setLanes(_ lanes: [LaneInfo]) {
    for view in stackView.arrangedSubviews {
      stackView.removeArrangedSubview(view)
      view.removeFromSuperview()
    }
    isHidden = lanes.isEmpty
    for lane in lanes {
      stackView.addArrangedSubview(makeLaneView(lane))
    }
  }

  private func makeLaneView(_ lane: LaneInfo) -> UIView {
    let recommended = LaneWay(rawValue: lane.recommendedWay)
    let isActive = recommended != nil && recommended != LaneWay.none
    // Recommended lanes draw recommended direction, others the first allowed direction
    // Not yet implemented that a lane may have multiple directions
    let way = isActive ? recommended!
      : (lane.laneWays.compactMap { LaneWay(rawValue: $0) }.first ?? .through)

    let imageView = UIImageView(image: UIImage(named: way.turnImageName)?.withRenderingMode(.alwaysTemplate))
    imageView.tintColor = UIColor.white()
    imageView.contentMode = .scaleAspectFit
    imageView.alpha = isActive ? 1.0 : inactiveLaneAlpha
    // Keep each lane cell square so the strip width scales with the lane count.
    let square = imageView.widthAnchor.constraint(equalTo: imageView.heightAnchor)
    square.priority = .defaultHigh
    square.isActive = true
    return imageView
  }
}
