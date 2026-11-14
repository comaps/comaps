@objc
final class AlertPresenter: NSObject {
  @objc static let shared = AlertPresenter()

  private let closeOnBackgroundAlerts = NSHashTable<UIAlertController>.weakObjects()
  private let locationAlerts = NSHashTable<UIAlertController>.weakObjects()

  private override init() {
    super.init()
    NotificationCenter.default.addObserver(forName: UIApplication.didEnterBackgroundNotification,
                                           object: nil,
                                           queue: .main) { [weak self] _ in
      self?.closeAlertsOnBackground()
    }
  }

  // MARK: - Actions

  @objc(presentLocationAlertOnCancel:)
  func presentLocationAlert(onCancel: @escaping () -> Void) {
    let message = [
      L("location_services_disabled_message"), "", L("location_services_disabled_1"),
      L("location_services_disabled_2"), L("location_services_disabled_3")
    ].joined(separator: "\n")
    let alert = makeAlert(title: L("location_services_disabled_header"), message: message)
    addAction(to: alert, title: L("cancel"), style: .cancel, handler: onCancel)
    addAction(to: alert, title: L("settings"), style: .default) {
      guard let url = URL(string: UIApplication.openSettingsURLString) else { return }
      UIApplication.shared.open(url)
    }
    locationAlerts.add(alert)
    present(alert, closeOnBackground: true)
  }

  @objc
  func presentLocationServicesDisabledAlert() {
    let message = [
      L("location_services_disabled_message"), "", L("location_services_disabled_1"),
      L("location_services_disabled_on_device_2"), L("location_services_disabled_on_device_3"),
      L("location_services_disabled_on_device_4"), "", L("location_services_disabled_on_device_additional_message")
    ].joined(separator: "\n")
    let alert = makeAlert(title: L("location_services_disabled_header"), message: message)
    addAction(to: alert, title: L("ok"), style: .default)
    locationAlerts.add(alert)
    present(alert, closeOnBackground: true)
  }

  @objc
  func presentLocationServiceNotSupportedAlert() {
    presentInfoAlert(title: L("current_location_unknown_error_title"),
                     message: L("current_location_unknown_error_message"))
  }

  @objc
  func presentUnknownCurrentPositionAlert() {
    presentInfoAlert(title: L("unknown_current_position"))
  }

  @objc(presentRoutingDisclaimerAlertOnAccept:)
  func presentRoutingDisclaimerAlert(onAccept: @escaping () -> Void) {
    let message = [
      L("dialog_routing_disclaimer_priority"), L("dialog_routing_disclaimer_precision"),
      L("dialog_routing_disclaimer_recommendations"), L("dialog_routing_disclaimer_borders"),
      L("dialog_routing_disclaimer_beware")
    ].joined(separator: "\n\n")
    let alert = makeAlert(title: L("dialog_routing_disclaimer_title"), message: message)
    addAction(to: alert, title: L("decline"), style: .cancel)
    addAction(to: alert, title: L("accept"), style: .default, handler: onAccept)
    present(alert)
  }

  @objc(presentPoint2PointAlertWithNeedToRebuild:onRebuild:)
  func presentPoint2PointAlert(needToRebuild: Bool, onRebuild: @escaping () -> Void) {
    if needToRebuild {
      presentDefaultAlert(title: L("p2p_only_from_current"),
                          message: L("p2p_reroute_from_current"),
                          confirmTitle: L("ok"),
                          cancelTitle: L("cancel"),
                          onConfirm: onRebuild)
    } else {
      presentInfoAlert(title: L("p2p_only_from_current"))
    }
  }

  @objc(presentUnableToCalculateRouteAlertOnSettings:)
  func presentUnableToCalculateRouteAlert(onSettings: @escaping () -> Void) {
    presentDefaultAlert(title: L("unable_to_calc_alert_title"),
                        message: L("unable_to_calc_alert_subtitle"),
                        confirmTitle: L("settings"),
                        cancelTitle: L("cancel"),
                        onConfirm: onSettings)
  }

  @objc(presentRoutingError:)
  func presentRoutingError(_ code: RouterResultCode) {
    switch code {
    case .noCurrentPosition:
      presentInfoAlert(title: L("dialog_routing_check_gps"),
                       message: "\(L("dialog_routing_error_location_not_found"))\n\n\(L("dialog_routing_location_turn_wifi"))")
    case .startPointNotFound:
      presentInfoAlert(title: L("dialog_routing_change_start"),
                       message: "\(L("dialog_routing_start_not_determined"))\n\n\(L("dialog_routing_select_closer_start"))")
    case .endPointNotFound:
      presentInfoAlert(title: L("dialog_routing_change_end"),
                       message: "\(L("dialog_routing_end_not_determined"))\n\n\(L("dialog_routing_select_closer_end"))")
    case .pointsInDifferentMWM:
      presentInfoAlert(title: L("routing_failed_cross_mwm_building"))
    case .transitRouteNotFoundNoNetwork:
      presentInfoAlert(title: L("transit_not_found"))
    case .transitRouteNotFoundTooLongPedestrian:
      presentInfoAlert(title: L("dialog_pedestrian_route_is_long_header"),
                       message: L("dialog_pedestrian_route_is_long_message"))
    case .routeNotFoundRedressRouteError, .routeNotFound, .inconsistentMWMandRoute:
      presentInfoAlert(title: L("dialog_routing_unable_locate_route"), message: L("dialog_routing_change_start_or_end"))
    case .routeFileNotExist, .fileTooOld:
      presentInfoAlert(title: L("dialog_routing_download_files"), message: L("dialog_routing_download_and_update_all"))
    case .internalError:
      presentInfoAlert(title: L("dialog_routing_system_error"),
                       message: "\(L("dialog_routing_application_error"))\n\n\(L("dialog_routing_try_again"))")
    case .intermediatePointNotFound:
      presentInfoAlert(title: L("dialog_routing_change_intermediate"),
                       message: L("dialog_routing_intermediate_not_determined"))
    case .cancelled, .noError, .hasWarnings, .needMoreMaps:
      break
    }
  }

  @objc(presentMissingMapsAlertWithCountries:code:onCancel:onDownload:onComplete:)
  func presentMissingMapsAlert(countries: [String],
                               code: RouterResultCode,
                               onCancel: @escaping () -> Void,
                               onDownload: @escaping ([String], @escaping () -> Void) -> Void,
                               onComplete: @escaping () -> Void) {
    MissingMapsAlert.present(countries: countries,
                             code: code,
                             presenter: self,
                             onCancel: onCancel,
                             onDownload: onDownload,
                             onComplete: onComplete)
  }

  @objc
  func presentNoConnectionAlert() {
    let alert = makeAlert(title: L("common_check_internet_connection_dialog"))
    addAction(to: alert, title: L("ok"), style: .default)
    present(alert, closeOnBackground: true)
  }

  @objc
  func presentDeleteMapProhibitedAlert() {
    let alert = makeAlert(title: L("downloader_delete_map"), message: L("downloader_delete_map_while_routing_dialog"))
    addAction(to: alert, title: L("ok"), style: .default)
    present(alert, closeOnBackground: true)
  }

  @objc(presentUnsavedEditsAlertOnDelete:)
  func presentUnsavedEditsAlert(onDelete: @escaping () -> Void) {
    let alert = makeAlert(title: L("please_note"), message: L("downloader_delete_map_dialog"))
    addAction(to: alert, title: L("cancel"), style: .cancel)
    addAction(to: alert, title: L("delete"), style: .destructive, handler: onDelete)
    present(alert)
  }

  @objc(presentNoWiFiAlertOnConfirm:onCancel:)
  func presentNoWiFiAlert(onConfirm: @escaping () -> Void, onCancel: (() -> Void)?) {
    let alert = makeAlert(title: L("download_over_mobile_header"), message: L("download_over_mobile_message"))
    addAction(to: alert, title: L("cancel"), style: .cancel, handler: onCancel)
    addAction(to: alert, title: L("use_cellular_data"), style: .default, handler: onConfirm)
    present(alert, closeOnBackground: true)
  }

  @objc(presentMobileInternetAlertOnAlways:onToday:onNotToday:)
  func presentMobileInternetAlert(onAlways: @escaping () -> Void,
                                  onToday: @escaping () -> Void,
                                  onNotToday: @escaping () -> Void) {
    let alert = makeAlert(title: L("mobile_data"), message: L("mobile_data_dialog"))
    addAction(to: alert, title: L("mobile_data_option_always"), style: .default, handler: onAlways)
    addAction(to: alert, title: L("mobile_data_option_today"), style: .default, handler: onToday)
    addAction(to: alert, title: L("mobile_data_option_not_today"), style: .cancel, handler: onNotToday)
    present(alert)
  }

  @objc
  func presentNotEnoughSpaceAlert() {
    let alert = makeAlert(title: L("downloader_no_space_title"), message: L("migration_no_space_message"))
    addAction(to: alert, title: L("ok"), style: .default)
    present(alert, closeOnBackground: true)
  }

  @objc(presentDownloaderNoConnectionAlertOnRetry:onCancel:)
  func presentDownloaderNoConnectionAlert(onRetry: @escaping () -> Void, onCancel: @escaping () -> Void) {
    let alert = makeAlert(title: L("downloader_status_failed"), message: L("common_check_internet_connection_dialog"))
    addAction(to: alert, title: L("cancel"), style: .cancel, handler: onCancel)
    addAction(to: alert, title: L("downloader_retry"), style: .default, handler: onRetry)
    present(alert, closeOnBackground: true)
  }

  @objc
  func presentDownloaderNotEnoughSpaceAlert() {
    let alert = makeAlert(title: L("downloader_no_space_title"), message: L("downloader_no_space_message"))
    addAction(to: alert, title: L("close"), style: .default)
    present(alert, closeOnBackground: true)
  }

  @objc(presentDownloaderInternalErrorAlertOnRetry:onCancel:)
  func presentDownloaderInternalErrorAlert(onRetry: @escaping () -> Void, onCancel: @escaping () -> Void) {
    let alert = makeAlert(title: L("migration_download_error_dialog"))
    addAction(to: alert, title: L("cancel"), style: .cancel, handler: onCancel)
    addAction(to: alert, title: L("downloader_retry"), style: .default, handler: onRetry)
    present(alert, closeOnBackground: true)
  }

  @objc
  func presentIncorrectFeaturePositionAlert() {
    presentInfoAlert(title: L("dialog_incorrect_feature_position"), message: L("message_invalid_feature_position"))
  }

  @objc(presentPlaceDoesntExistAlertOnSend:)
  func presentPlaceDoesntExistAlert(onSend: @escaping (String) -> Void) {
    let alert = makeAlert(title: L("editor_place_doesnt_exist"), message: L("editor_place_doesnt_exist_description"))
    alert.addTextField { textField in
      textField.placeholder = L("editor_comment_hint")
      textField.autocapitalizationType = .sentences
    }
    addAction(to: alert, title: L("cancel"), style: .cancel)
    addAction(to: alert, title: L("editor_report_problem_send_button"), style: .default) { [weak alert] in
      onSend(alert?.textFields?.first?.text ?? "")
    }
    present(alert)
  }

  @objc(presentResetChangesAlertOnReset:)
  func presentResetChangesAlert(onReset: @escaping () -> Void) {
    let alert = makeAlert(title: L("editor_reset_edits_message"))
    addAction(to: alert, title: L("cancel"), style: .cancel)
    addAction(to: alert, title: L("editor_reset_edits_button"), style: .destructive, handler: onReset)
    present(alert)
  }

  @objc(presentDeleteFeatureAlertOnDelete:)
  func presentDeleteFeatureAlert(onDelete: @escaping () -> Void) {
    let alert = makeAlert(title: L("editor_remove_place_message"))
    addAction(to: alert, title: L("cancel"), style: .cancel)
    addAction(to: alert, title: L("editor_remove_place_button"), style: .destructive, handler: onDelete)
    present(alert)
  }

  @objc(presentPersonalInfoWarningAlertOnSend:)
  func presentPersonalInfoWarningAlert(onSend: @escaping () -> Void) {
    presentDefaultAlert(title: L("editor_share_to_all_dialog_title"),
                        message: "\(L("editor_share_to_all_dialog_message_1"))\n\(L("editor_share_to_all_dialog_message_2"))",
                        confirmTitle: L("editor_report_problem_send_button"),
                        cancelTitle: L("cancel"),
                        onConfirm: onSend)
  }

  @objc(presentTrackWarningAlertOnTurnOff:)
  func presentTrackWarningAlert(onTurnOff: @escaping () -> Void) {
    presentDefaultAlert(title: L("recent_track_background_dialog_title"),
                        message: L("recent_track_background_dialog_message"),
                        confirmTitle: L("off_recent_track_background_button"),
                        cancelTitle: L("continue_button"),
                        onConfirm: onTurnOff)
  }

  func presentCreateBookmarkCategoryAlert(maxCharacters: Int,
                                          minCharacters: Int,
                                          onCreate: @escaping (String) -> Bool) {
    presentCreateBookmarkCategoryAlert(maxCharacters: maxCharacters,
                                       minCharacters: minCharacters,
                                       onCreate: onCreate,
                                       text: nil,
                                       error: nil)
  }

  private func presentCreateBookmarkCategoryAlert(maxCharacters: Int,
                                                  minCharacters: Int,
                                                  onCreate: @escaping (String) -> Bool,
                                                  text: String?,
                                                  error: String?) {
    let alert = makeAlert(title: L("bookmarks_create_new_group"), message: error)
    addAction(to: alert, title: L("cancel"), style: .cancel)
    let createAction = addAction(to: alert, title: L("create").capitalized, style: .default) { [weak self, weak alert] in
      let name = alert?.textFields?.first?.text ?? ""
      if !onCreate(name) {
        self?.presentCreateBookmarkCategoryAlert(maxCharacters: maxCharacters,
                                                 minCharacters: minCharacters,
                                                 onCreate: onCreate,
                                                 text: name,
                                                 error: L("bookmarks_error_title_list_name_already_taken"))
      }
    }
    let length = text?.count ?? 0
    createAction.isEnabled = length > minCharacters && length <= maxCharacters
    alert.addTextField { [weak alert] textField in
      textField.placeholder = L("bookmark_set_name")
      textField.text = text
      textField.autocapitalizationType = .sentences
      textField.clearButtonMode = .whileEditing
      textField.addAction(UIAction { action in
        let length = (action.sender as? UITextField)?.text?.count ?? 0
        createAction.isEnabled = length > minCharacters && length <= maxCharacters
        alert?.message = length > maxCharacters ? L("bookmarks_error_title_list_name_too_long") : nil
      }, for: .editingChanged)
    }
    present(alert)
  }

  @objc(presentInfoAlertWithTitle:message:)
  func presentInfoAlert(title: String, message: String? = nil) {
    let alert = makeAlert(title: title, message: message)
    addAction(to: alert, title: L("ok"), style: .default)
    present(alert)
  }

  func presentBugReportAlert(title: String) {
    let alert = makeAlert(title: title, message: L("bugreport_alert_message"))
    addAction(to: alert, title: L("cancel"), style: .cancel)
    addAction(to: alert, title: L("report_a_bug"), style: .default) {
      MailComposer.sendBugReportWith(title: title)
    }
    present(alert, closeOnBackground: true)
  }

  private func presentDefaultAlert(title: String,
                                   message: String?,
                                   confirmTitle: String,
                                   cancelTitle: String?,
                                   onConfirm: (() -> Void)?) {
    let alert = makeAlert(title: title, message: message)
    if let cancelTitle {
      addAction(to: alert, title: cancelTitle, style: .cancel)
    }
    addAction(to: alert, title: confirmTitle, style: .default, handler: onConfirm)
    present(alert)
  }

  // MARK: - Presentation

  private func makeAlert(title: String, message: String? = nil) -> UIAlertController {
    UIAlertController(title: title, message: message, preferredStyle: .alert)
  }

  @discardableResult
  private func addAction(to alert: UIAlertController,
                         title: String,
                         style: UIAlertAction.Style,
                         handler: (() -> Void)? = nil) -> UIAlertAction {
    let action = UIAlertAction(title: title, style: style) { _ in handler?() }
    alert.addAction(action)
    if style == .default {
      alert.preferredAction = action
    }
    return action
  }

  private var rootViewController: UIViewController? {
    MapsAppDelegate.theApp().window?.rootViewController
  }

  private var topViewController: UIViewController? {
    guard var topViewController = rootViewController, topViewController.isViewLoaded else { return nil }
    while let presented = topViewController.presentedViewController, !presented.isBeingDismissed {
      topViewController = presented
    }
    return topViewController
  }

  func present(_ alert: UIAlertController, closeOnBackground: Bool = false) {
    guard let topViewController else { return }

    if let coordinator = topViewController.transitionCoordinator {
      coordinator.animate(alongsideTransition: nil) { [weak self] _ in
        self?.present(alert, closeOnBackground: closeOnBackground)
      }
      return
    }

    if let topAlert = topViewController as? UIAlertController,
       locationAlerts.contains(alert),
       locationAlerts.contains(topAlert) {
      return
    }

    LOG(.info, "Alert: \(alert.title ?? "")")
    if closeOnBackground {
      closeOnBackgroundAlerts.add(alert)
    }
    MapsAppDelegate.theApp().window?.endEditing(true)
    topViewController.present(alert, animated: true)
  }

  private func closeAlertsOnBackground() {
    var viewController = rootViewController
    while let presented = viewController?.presentedViewController {
      viewController = presented
      if let alert = presented as? UIAlertController, closeOnBackgroundAlerts.contains(alert) {
        alert.presentingViewController?.dismiss(animated: false)
        return
      }
    }
  }
}
