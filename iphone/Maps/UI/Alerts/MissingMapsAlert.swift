import CoMapsInterop
import CxxStdlib

final class MissingMapsAlert: NSObject, StorageObserver {
  private let presenter: AlertPresenter
  private let title: String
  private let message: String
  private let onCancel: () -> Void
  private let onDownload: ([String], @escaping () -> Void) -> Void
  private let onComplete: () -> Void

  private var countries: [String]
  private var countriesDescription = ""
  private var isDownloading = false
  private weak var alert: UIAlertController?

  static func present(countries: [String],
                      code: RouterResultCode,
                      presenter: AlertPresenter,
                      onCancel: @escaping () -> Void,
                      onDownload: @escaping ([String], @escaping () -> Void) -> Void,
                      onComplete: @escaping () -> Void) {
    assert(!countries.isEmpty, "countries can not be empty.")
    let title: String
    let message: String
    switch code {
    case .inconsistentMWMandRoute, .routeNotFound, .routeFileNotExist:
      title = L("dialog_routing_download_files")
      message = L("dialog_routing_download_and_update_all")
    case .fileTooOld:
      title = L("dialog_routing_download_files")
      message = L("dialog_routing_download_and_update_maps")
    case .needMoreMaps:
      title = L("dialog_routing_download_and_build_cross_route")
      message = L("dialog_routing_download_cross_route")
    default:
      assertionFailure("Incorrect code!")
      return
    }
    let missingMapsAlert = MissingMapsAlert(countries: countries,
                                            title: title,
                                            message: message,
                                            presenter: presenter,
                                            onCancel: onCancel,
                                            onDownload: onDownload,
                                            onComplete: onComplete)
    missingMapsAlert.updateCountriesList()
    Storage.shared().add(missingMapsAlert)
    missingMapsAlert.presentPrompt()
  }

  private init(countries: [String],
               title: String,
               message: String,
               presenter: AlertPresenter,
               onCancel: @escaping () -> Void,
               onDownload: @escaping ([String], @escaping () -> Void) -> Void,
               onComplete: @escaping () -> Void) {
    self.countries = countries
    self.title = title
    self.message = message
    self.presenter = presenter
    self.onCancel = onCancel
    self.onDownload = onDownload
    self.onComplete = onComplete
    super.init()
  }

  private var countryIds: interop.CountryIds {
    var ids = interop.CountryIds()
    countries.forEach { ids.push_back(std.string($0)) }
    return ids
  }

  private func updateCountriesList() {
    countries = interop.OutdatedCountries(countryIds).map { String($0) }
    let names = countries.map { String(interop.CountryName(std.string($0))) }
    let totalSize = countries.reduce(UInt64(0)) { $0 + interop.CountrySize(std.string($1)) }
    countriesDescription = "\(L("downloader_status_maps")): \(names.joined(separator: ", ")) (\(formattedSize(totalSize)))"
  }

  private var promptMessage: String {
    "\(message)\n\n\(countriesDescription)"
  }

  // MARK: - Alerts

  private func presentPrompt() {
    let alert = UIAlertController(title: title, message: promptMessage, preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: L("later"), style: .cancel) { _ in
      self.close(completion: self.onCancel)
    })
    let download = UIAlertAction(title: L("download"), style: .default) { _ in
      self.startDownload()
    }
    alert.addAction(download)
    alert.preferredAction = download
    present(alert)
  }

  private func presentProgress() {
    let alert = UIAlertController(title: L("downloading"), message: countriesDescription, preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: L("cancel"), style: .cancel) { _ in
      self.countries.forEach { Storage.shared().cancelDownloadNode($0) }
      self.close(completion: self.onCancel)
    })
    present(alert)
  }

  private func present(_ alert: UIAlertController) {
    self.alert = alert
    presenter.present(alert)
  }

  private func startDownload() {
    updateCountriesList()
    if countries.isEmpty {
      close(completion: onComplete)
      return
    }
    onDownload(countries) {
      self.isDownloading = true
      self.presentProgress()
    }
  }

  private func close(completion: (() -> Void)?) {
    Storage.shared().remove(self)
    if let alert, let presenting = alert.presentingViewController, !alert.isBeingDismissed {
      presenting.dismiss(animated: true, completion: completion)
    } else {
      completion?()
    }
  }

  // MARK: - StorageObserver

  func processCountryEvent(_ countryId: String) {
    guard countries.contains(countryId) else { return }
    if isDownloading {
      if interop.HasFailedCountries(countryIds) {
        if interop.IsAutoRetryDownloadFailed() {
          close(completion: nil)
        }
        return
      }
      let progress = interop.OverallProgress(countryIds)
      if progress.m_downloaded == progress.m_total {
        close(completion: onComplete)
      }
    } else {
      updateCountriesList()
      if countries.isEmpty {
        close(completion: onComplete)
      } else {
        alert?.message = promptMessage
      }
    }
  }

  func processCountry(_ countryId: String, downloadedBytes: UInt64, totalBytes: UInt64) {
    guard isDownloading, countries.contains(countryId) else { return }
    let progress = interop.OverallProgress(countryIds)
    guard progress.m_total != 0 else { return }
    let fraction = Double(progress.m_downloaded) / Double(progress.m_total)
    let percent = NumberFormatter.localizedString(from: NSNumber(value: fraction), number: .percent)
    alert?.title = "\(L("downloading")) \(percent)"
  }
}
