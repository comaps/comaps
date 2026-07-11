final class IndoorLevelPickerViewController: MWMViewController {

  private enum Constants {
    static let buttonSize = CGFloat(40)
    static let trailingOffset = CGFloat(10)
  }

  private let stackView = UIStackView()

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

    stackView.axis = .vertical
    stackView.spacing = 1
    stackView.translatesAutoresizingMaskIntoConstraints = false
    view.addSubview(stackView)
  }

  private func layout() {
    guard let superview = view.superview else { return }
    NSLayoutConstraint.activate([
      view.trailingAnchor.constraint(equalTo: superview.trailingAnchor, constant: -Constants.trailingOffset),
      view.centerYAnchor.constraint(equalTo: superview.centerYAnchor),
      stackView.topAnchor.constraint(equalTo: view.topAnchor),
      stackView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
      stackView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      stackView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
    ])
  }

  private func updateLevels() {
    let levels = IndoorManager.levels()
    let activeLevel = IndoorManager.activeLevel()

    stackView.arrangedSubviews.forEach { $0.removeFromSuperview() }
    view.isHidden = levels.isEmpty
    guard !levels.isEmpty else { return }

    for level in levels {
      let button = UIButton(type: .system)
      button.setTitle(level, for: .normal)
      button.backgroundColor = .systemBackground
      button.isEnabled = level != activeLevel
      button.addTarget(self, action: #selector(onLevelButtonPressed(_:)), for: .touchUpInside)
      NSLayoutConstraint.activate([
        button.widthAnchor.constraint(equalToConstant: Constants.buttonSize),
        button.heightAnchor.constraint(equalToConstant: Constants.buttonSize),
      ])
      stackView.addArrangedSubview(button)
    }
  }

  @objc
  private func onLevelButtonPressed(_ sender: UIButton) {
    guard let level = sender.title(for: .normal) else { return }
    IndoorManager.selectLevel(level)
  }
}

// MARK: - MWMIndoorObserver

extension IndoorLevelPickerViewController: MWMIndoorObserver {
  func onIndoorLevelsUpdated() {
    updateLevels()
  }
}
