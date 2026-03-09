@objc(MWMNavigationAddPointToastView)
final class NavigationAddPointToastView: UIView {
  @IBOutlet private var actionButton: UIButton!
  @IBOutlet private weak var extraBottomBackground: UIView!
  @objc private(set) var isStart = true

  @objc func config(isStart: Bool, withLocationButton: Bool) {
    self.isStart = isStart
    let text = isStart ? L("routing_add_start_point") : L("routing_add_finish_point")
    actionButton.setTitle(text, for: .normal)
  }
}
