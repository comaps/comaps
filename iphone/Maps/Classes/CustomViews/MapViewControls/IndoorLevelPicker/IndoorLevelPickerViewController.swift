final class IndoorLevelPickerViewController: MWMViewController {

  private enum Constants {
    static let buttonWidth = CGFloat(48)
    static let buttonHeight = CGFloat(44)
    static let spacing = CGFloat(1)
    static let trailingOffset = CGFloat(10)
    static let cornerRadius = CGFloat(8)
    // Cap at 4 buttons high
    static let maxVisibleHeight = buttonHeight * 4 + spacing * 3
  }

  private let scrollView = UIScrollView()
  private let stackView = UIStackView()
  private var trailingConstraint = NSLayoutConstraint()

  private static var availableArea: CGRect = .zero
  private static var trailingConstraintValue: CGFloat {
    if availableArea == .zero {
      return -Constants.trailingOffset
    }
    return -(UIScreen.main.bounds.maxX - availableArea.maxX + Constants.trailingOffset)
  }

  @objc
  init() {
    super.init(nibName: nil, bundle: nil)
    let ownerViewController = MapViewController.shared()
    ownerViewController?.addChild(self)
    ownerViewController?.controlsView.addSubview(view)
    setupView()
    layout()
    IndoorManager.add(self)
    updateLevels()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  deinit {
    IndoorManager.remove(self)
  }

  private func setupView() {
    view.translatesAutoresizingMaskIntoConstraints = false
    view.isHidden = true

    scrollView.translatesAutoresizingMaskIntoConstraints = false
    scrollView.showsVerticalScrollIndicator = false
    scrollView.layer.cornerRadius = Constants.cornerRadius
    scrollView.clipsToBounds = true
    // Separator colour shows through the 1pt gaps between buttons.
    scrollView.backgroundColor = .separator
    view.addSubview(scrollView)

    stackView.axis = .vertical
    stackView.spacing = Constants.spacing
    stackView.translatesAutoresizingMaskIntoConstraints = false
    scrollView.addSubview(stackView)
  }

  private func layout() {
    guard let superview = view.superview else { return }
    trailingConstraint = view.trailingAnchor.constraint(equalTo: superview.trailingAnchor,
                                                         constant: Self.trailingConstraintValue)
    NSLayoutConstraint.activate([
      trailingConstraint,
      view.centerYAnchor.constraint(equalTo: superview.centerYAnchor),
      view.widthAnchor.constraint(equalToConstant: Constants.buttonWidth),

      scrollView.topAnchor.constraint(equalTo: view.topAnchor),
      scrollView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
      scrollView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      scrollView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      scrollView.heightAnchor.constraint(lessThanOrEqualToConstant: Constants.maxVisibleHeight),

      stackView.topAnchor.constraint(equalTo: scrollView.contentLayoutGuide.topAnchor),
      stackView.bottomAnchor.constraint(equalTo: scrollView.contentLayoutGuide.bottomAnchor),
      stackView.leadingAnchor.constraint(equalTo: scrollView.contentLayoutGuide.leadingAnchor),
      stackView.trailingAnchor.constraint(equalTo: scrollView.contentLayoutGuide.trailingAnchor),
      stackView.widthAnchor.constraint(equalTo: scrollView.frameLayoutGuide.widthAnchor),
    ])
  }

  private func updateLevels() {
    let levels = IndoorManager.viewportLevels()
    let activeLevel = IndoorManager.activeLevel()

    stackView.arrangedSubviews.forEach { $0.removeFromSuperview() }
    view.isHidden = levels.isEmpty
    guard !levels.isEmpty else { return }

    for level in levels {
      stackView.addArrangedSubview(makeLevelButton(title: level, active: level == activeLevel))
    }

    // Center the active floor so there's visible cutoff at the edges
    scrollView.layoutIfNeeded()
    if let index = levels.firstIndex(of: activeLevel), index < stackView.arrangedSubviews.count {
      let activeFrame = stackView.arrangedSubviews[index].frame
      let maxOffset = max(0, scrollView.contentSize.height - scrollView.bounds.height)
      let centeredY = activeFrame.midY - scrollView.bounds.height / 2
      scrollView.contentOffset = CGPoint(x: 0, y: min(max(0, centeredY), maxOffset))
    }
  }

  private func makeLevelButton(title: String, active: Bool) -> UIButton {
    let button = UIButton(type: .custom)
    button.setTitle(title, for: .normal)
    button.titleLabel?.font = .systemFont(ofSize: 15, weight: active ? .semibold : .regular)
    // Highlight the currently selected floor
    button.backgroundColor = active ? .linkBlue() : .systemBackground
    button.setTitleColor(active ? .white : .label, for: .normal)
    button.isEnabled = !active
    button.addTarget(self, action: #selector(onLevelButtonPressed(_:)), for: .touchUpInside)
    NSLayoutConstraint.activate([
      button.widthAnchor.constraint(equalToConstant: Constants.buttonWidth),
      button.heightAnchor.constraint(equalToConstant: Constants.buttonHeight),
    ])
    return button
  }

  @objc
  private func onLevelButtonPressed(_ sender: UIButton) {
    guard let level = sender.title(for: .normal) else { return }
    IndoorManager.selectLevel(level)
  }

  private func updateLayout() {
    guard let superview = view.superview else { return }
    superview.animateConstraints {
      self.trailingConstraint.constant = Self.trailingConstraintValue
    }
  }

  // Called by SideButtonsArea whenever the usable right-side region changes (search opened, tab bar
  // shown, orientation change...). Keeps the picker aligned with the zoom buttons.
  static func updateAvailableArea(_ frame: CGRect) {
    availableArea = frame
    guard let picker = MapViewController.shared()?.controlsManager.indoorLevelPicker else { return }
    DispatchQueue.main.async {
      picker.updateLayout()
    }
  }
}

// MARK: - MWMIndoorObserver

extension IndoorLevelPickerViewController: MWMIndoorObserver {
  func onIndoorLevelsUpdated() {
    updateLevels()
  }
}
