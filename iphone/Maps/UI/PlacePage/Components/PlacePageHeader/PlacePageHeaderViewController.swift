protocol PlacePageHeaderViewProtocol: AnyObject {
  var presenter: PlacePageHeaderPresenterProtocol?  { get set }
  var isExpandViewHidden: Bool { get set }
  var isShadowViewHidden: Bool { get set }

  func setTitle(_ title: String?, secondaryTitle: String?, branch: String?)
  func showShareTrackMenu()
}

class PlacePageHeaderViewController: UIViewController {
  var presenter: PlacePageHeaderPresenterProtocol?

  @IBOutlet private var headerView: PlacePageHeaderView!
  @IBOutlet private var titleLabel: UILabel?
  @IBOutlet private var expandView: UIView!
  @IBOutlet private var shadowView: UIView!
  @IBOutlet private var grabberView: UIView!

  @IBOutlet weak var closeButton: CircleImageButton!
  @IBOutlet weak var shareButton: CircleImageButton!

  private var titleText: String?
  private var secondaryText: String?
  private var branchText: String?

  override func viewDidLoad() {
    super.viewDidLoad()
    presenter?.configure()
    let tap = UITapGestureRecognizer(target: self, action: #selector(onExpandPressed(sender:)))
    expandView.addGestureRecognizer(tap)
    headerView.layer.maskedCorners = [.layerMinXMinYCorner, .layerMaxXMinYCorner]
    iPadSpecific { [weak self] in
      self?.grabberView.isHidden = true
    }
    closeButton.setImage(UIImage(named: "ic_close")!)
    shareButton.setImage(UIImage(named: "ic_share")!)

    if presenter?.objectType == .track {
      configureTrackSharingMenu()
    }

    let interaction = UIContextMenuInteraction(delegate: self)
    titleLabel?.addInteraction(interaction)
  }

  @objc func onExpandPressed(sender: UITapGestureRecognizer) {
    presenter?.onExpandPress()
  }

  @IBAction private func onCloseButtonPressed(_ sender: Any) {
    presenter?.onClosePress()
  }

  @IBAction func onShareButtonPressed(_ sender: Any) {
    presenter?.onShareButtonPress(from: shareButton)
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    guard traitCollection.userInterfaceStyle != previousTraitCollection?.userInterfaceStyle else { return }
    setTitle(titleText, secondaryTitle: secondaryText, branch: branchText)
  }
}

extension PlacePageHeaderViewController: PlacePageHeaderViewProtocol {
  var isExpandViewHidden: Bool {
    get {
      expandView.isHidden
    }
    set {
      expandView.isHidden = newValue
    }
  }

  var isShadowViewHidden: Bool {
    get {
      shadowView.isHidden
    }
    set {
      shadowView.isHidden = newValue
    }
  }

  func setTitle(_ title: String?, secondaryTitle: String?, branch: String? = nil) {
    titleText = title
    secondaryText = secondaryTitle
    branchText = branch
    
    // XCode 13 is not smart enough to detect that title is used below, and requires explicit unwrapped variable.
    guard let unwrappedTitle = title else {
      titleLabel?.attributedText = nil
      return
    }

    let titleAttributes: [NSAttributedString.Key: Any] = [
      .font: StyleManager.shared.theme!.fonts.semibold20,
      .foregroundColor: UIColor.blackPrimaryText()
    ]

    let attributedText = NSMutableAttributedString(string: unwrappedTitle, attributes: titleAttributes)
    
    // Add branch with thinner font weight if present and not already in title
    if let branch = branch, !branch.isEmpty {
      let branchAttributes: [NSAttributedString.Key: Any] = [
        .font: StyleManager.shared.theme!.fonts.regular20,
        .foregroundColor: UIColor.blackPrimaryText()
      ]
      attributedText.append(NSAttributedString(string: " \(branch)", attributes: branchAttributes))
    }

    guard let unwrappedSecondaryTitle = secondaryTitle else {
      titleLabel?.attributedText = attributedText
      return
    }

    let paragraphStyle = NSMutableParagraphStyle()
    paragraphStyle.paragraphSpacingBefore = 2
    let secondaryTitleAttributes: [NSAttributedString.Key: Any] = [
      .font: StyleManager.shared.theme!.fonts.medium16,
      .foregroundColor: UIColor.blackPrimaryText(),
      .paragraphStyle: paragraphStyle
    ]

    attributedText.append(NSAttributedString(string: "\n" + unwrappedSecondaryTitle, attributes: secondaryTitleAttributes))
    titleLabel?.attributedText = attributedText
  }

  func showShareTrackMenu() {
      // The menu will be shown by the shareButton itself
  }

  private func configureTrackSharingMenu() {
    let menu = UIMenu(title: "", image: nil, children: [
      UIAction(title: L("export_file"), image: nil, handler: { [weak self] _ in
        guard let self else { return }
        self.presenter?.onExportTrackButtonPress(.text, from: self.shareButton)
      }),
      UIAction(title: L("export_file_gpx"), image: nil, handler: { [weak self] _ in
        guard let self else { return }
        self.presenter?.onExportTrackButtonPress(.gpx, from: self.shareButton)
      }),
    ])
    shareButton.menu = menu
    shareButton.showsMenuAsPrimaryAction = true
  }
}

extension PlacePageHeaderViewController: UIContextMenuInteractionDelegate {
  func contextMenuInteraction(_ interaction: UIContextMenuInteraction, configurationForMenuAtLocation location: CGPoint) -> UIContextMenuConfiguration? {
    return UIContextMenuConfiguration(identifier: nil, previewProvider: nil, actionProvider: { suggestedActions in
      let copyAction = UIAction(title: L("copy_to_clipboard"), image: UIImage(systemName: "document.on.clipboard")) { action in
          UIPasteboard.general.string = self.titleLabel?.text
      }
      return UIMenu(title: "", children: [copyAction])
    })
  }
}
