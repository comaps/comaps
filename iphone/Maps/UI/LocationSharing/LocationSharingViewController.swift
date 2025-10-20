import UIKit

/// View controller for managing live location sharing
@objc class LocationSharingViewController: UIViewController {

  private let service = LocationSharingService.shared

  // UI Components
  private let scrollView = UIScrollView()
  private let contentView = UIView()
  private let statusLabel = UILabel()
  private let descriptionLabel = UILabel()
  private let urlTextView = UITextView()
  private let startStopButton = UIButton(type: .system)
  private let copyButton = UIButton(type: .system)
  private let shareButton = UIButton(type: .system)

  override func viewDidLoad() {
    super.viewDidLoad()

    title = NSLocalizedString("location_sharing_title", comment: "")
    view.backgroundColor = .systemBackground

    setupUI()
    updateUI()
  }

  // MARK: - UI Setup

  private func setupUI() {
    // Setup scroll view
    scrollView.translatesAutoresizingMaskIntoConstraints = false
    view.addSubview(scrollView)

    contentView.translatesAutoresizingMaskIntoConstraints = false
    scrollView.addSubview(contentView)

    // Status label
    statusLabel.font = .systemFont(ofSize: 16, weight: .medium)
    statusLabel.textColor = .label
    statusLabel.numberOfLines = 0
    statusLabel.translatesAutoresizingMaskIntoConstraints = false
    contentView.addSubview(statusLabel)

    // Description label
    descriptionLabel.font = .systemFont(ofSize: 14)
    descriptionLabel.textColor = .secondaryLabel
    descriptionLabel.numberOfLines = 0
    descriptionLabel.text = NSLocalizedString("location_sharing_description", comment: "")
    descriptionLabel.translatesAutoresizingMaskIntoConstraints = false
    contentView.addSubview(descriptionLabel)

    // URL text view
    urlTextView.font = .monospacedSystemFont(ofSize: 12, weight: .regular)
    urlTextView.textColor = .label
    urlTextView.backgroundColor = .secondarySystemBackground
    urlTextView.layer.cornerRadius = 8
    urlTextView.isEditable = false
    urlTextView.isScrollEnabled = false
    urlTextView.textContainerInset = UIEdgeInsets(top: 12, left: 12, bottom: 12, right: 12)
    urlTextView.translatesAutoresizingMaskIntoConstraints = false
    contentView.addSubview(urlTextView)

    // Buttons
    setupButtons()

    // Constraints
    setupConstraints()
  }

  private func setupButtons() {
    // Start/Stop button
    startStopButton.titleLabel?.font = .systemFont(ofSize: 17, weight: .semibold)
    startStopButton.addTarget(self, action: #selector(startStopTapped), for: .touchUpInside)
    startStopButton.translatesAutoresizingMaskIntoConstraints = false
    contentView.addSubview(startStopButton)

    // Copy button
    copyButton.setTitle(NSLocalizedString("location_sharing_copy_url", comment: ""), for: .normal)
    copyButton.addTarget(self, action: #selector(copyTapped), for: .touchUpInside)
    copyButton.translatesAutoresizingMaskIntoConstraints = false
    contentView.addSubview(copyButton)

    // Share button
    shareButton.setTitle(NSLocalizedString("location_sharing_share_url", comment: ""), for: .normal)
    shareButton.addTarget(self, action: #selector(shareTapped), for: .touchUpInside)
    shareButton.translatesAutoresizingMaskIntoConstraints = false
    contentView.addSubview(shareButton)
  }

  private func setupConstraints() {
    NSLayoutConstraint.activate([
      // Scroll view
      scrollView.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor),
      scrollView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      scrollView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      scrollView.bottomAnchor.constraint(equalTo: view.bottomAnchor),

      // Content view
      contentView.topAnchor.constraint(equalTo: scrollView.topAnchor),
      contentView.leadingAnchor.constraint(equalTo: scrollView.leadingAnchor),
      contentView.trailingAnchor.constraint(equalTo: scrollView.trailingAnchor),
      contentView.bottomAnchor.constraint(equalTo: scrollView.bottomAnchor),
      contentView.widthAnchor.constraint(equalTo: scrollView.widthAnchor),

      // Status label
      statusLabel.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 16),
      statusLabel.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 16),
      statusLabel.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -16),

      // Description label
      descriptionLabel.topAnchor.constraint(equalTo: statusLabel.bottomAnchor, constant: 8),
      descriptionLabel.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 16),
      descriptionLabel.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -16),

      // URL text view
      urlTextView.topAnchor.constraint(equalTo: descriptionLabel.bottomAnchor, constant: 16),
      urlTextView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 16),
      urlTextView.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -16),

      // Start/Stop button
      startStopButton.topAnchor.constraint(equalTo: urlTextView.bottomAnchor, constant: 24),
      startStopButton.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 16),
      startStopButton.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -16),
      startStopButton.heightAnchor.constraint(equalToConstant: 50),

      // Copy button
      copyButton.topAnchor.constraint(equalTo: startStopButton.bottomAnchor, constant: 12),
      copyButton.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 16),
      copyButton.heightAnchor.constraint(equalToConstant: 44),

      // Share button
      shareButton.topAnchor.constraint(equalTo: startStopButton.bottomAnchor, constant: 12),
      shareButton.leadingAnchor.constraint(equalTo: copyButton.trailingAnchor, constant: 12),
      shareButton.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -16),
      shareButton.widthAnchor.constraint(equalTo: copyButton.widthAnchor),
      shareButton.heightAnchor.constraint(equalToConstant: 44),
      shareButton.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -16),
    ])
  }

  // MARK: - Update UI

  private func updateUI() {
    let isSharing = service.isSharing

    // Update status
    statusLabel.text = isSharing
      ? NSLocalizedString("location_sharing_status_active", comment: "")
      : NSLocalizedString("location_sharing_status_inactive", comment: "")

    // Update URL
    if let shareUrl = service.shareUrl, isSharing {
      urlTextView.text = shareUrl
      urlTextView.isHidden = false
    } else {
      urlTextView.isHidden = true
    }

    // Update buttons
    let startStopTitle = isSharing
      ? NSLocalizedString("location_sharing_stop", comment: "")
      : NSLocalizedString("location_sharing_start", comment: "")
    startStopButton.setTitle(startStopTitle, for: .normal)
    startStopButton.backgroundColor = isSharing ? .systemRed : .systemBlue
    startStopButton.setTitleColor(.white, for: .normal)
    startStopButton.layer.cornerRadius = 8

    copyButton.isHidden = !isSharing
    shareButton.isHidden = !isSharing
  }

  // MARK: - Actions

  @objc private func startStopTapped() {
    if service.isSharing {
      stopSharing()
    } else {
      startSharing()
    }
  }

  private func startSharing() {
    guard let shareUrl = service.startSharing() else {
      showAlert(message: NSLocalizedString("location_sharing_failed_to_start", comment: ""))
      return
    }

    // Auto-copy URL to clipboard
    UIPasteboard.general.string = shareUrl

    showAlert(message: NSLocalizedString("location_sharing_started", comment: ""))
    updateUI()
  }

  private func stopSharing() {
    service.stopSharing()
    showAlert(message: NSLocalizedString("location_sharing_stopped", comment: ""))
    updateUI()
  }

  @objc private func copyTapped() {
    guard let shareUrl = service.shareUrl else { return }

    UIPasteboard.general.string = shareUrl
    showAlert(message: NSLocalizedString("location_sharing_url_copied", comment: ""))
  }

  @objc private func shareTapped() {
    guard let shareUrl = service.shareUrl else { return }

    let message = String(format: NSLocalizedString("location_sharing_share_message", comment: ""), shareUrl)
    let activityVC = UIActivityViewController(activityItems: [message], applicationActivities: nil)
    activityVC.popoverPresentationController?.sourceView = shareButton
    present(activityVC, animated: true)
  }

  // MARK: - Helpers

  private func showAlert(message: String) {
    let alert = UIAlertController(title: nil, message: message, preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: NSLocalizedString("close", comment: ""), style: .default))
    present(alert, animated: true)
  }
}
