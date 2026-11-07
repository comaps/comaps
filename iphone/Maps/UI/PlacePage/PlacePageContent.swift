import SwiftUI

@MainActor
@Observable
final class PlacePageContent {
  struct ActionBarItems {
    var visible: [PlacePageAction] = []
    var overflow: [PlacePageAction] = []
  }

  struct UserMark {
    enum Kind {
      case bookmark(PlacePageBookmarkData)
      case track(PlacePageTrackData)
    }

    let kind: Kind
    let color: UIColor
    let category: String?
    let groupId: MWMMarkGroupID
    let description: String?
    let isHTMLDescription: Bool
  }

  let data: PlacePageData
  let isRoutePlanning = MWMNavigationDashboardManager.shared().state != .hidden
  let canAddStop = MWMRouter.canAddIntermediatePoint()

  private(set) var revision = 0
  private(set) var downloadProgress: Double?
  private(set) var distance: String?
  private(set) var direction: Angle?
  private(set) var speedAndAltitude: String?
  private(set) var activePointDistance: Double?
  private(set) var myPositionDistance: Double?
  private(set) var didRemoveBookmark = false

  private let bookmarksManager = BookmarksManager.shared()
  @ObservationIgnored private var observer: PlacePageObserver?
  @ObservationIgnored private var lastLocation: CLLocation?
  @ObservationIgnored private var isObservingTrackPoints = false

  init(data: PlacePageData) {
    self.data = data
    start()
  }

  var objectType: PlacePageObjectType { observed(data.objectType) }
  var preview: PlacePagePreviewData { observed(data.previewData) }
  var bookmark: PlacePageBookmarkData? { observed(data.bookmarkData) }
  var track: PlacePageTrackData? { observed(data.trackData) }
  var osmContribution: PlacePageOSMContributionData? { observed(data.osmContributionData) }
  var mapNodeStatus: MapNodeStatus? { observed(data.mapNodeAttributes?.nodeStatus) }

  var userMark: UserMark? {
    if let bookmark {
      return UserMark(kind: .bookmark(bookmark), color: bookmark.color.color, category: bookmark.bookmarkCategory,
                      groupId: bookmark.bookmarkGroupId, description: bookmark.bookmarkDescription,
                      isHTMLDescription: bookmark.isHtmlDescription)
    }
    if let track {
      return UserMark(kind: .track(track), color: track.color ?? .buttonRed(), category: track.trackCategory,
                      groupId: track.groupId, description: track.trackDescription, isHTMLDescription: false)
    }
    return nil
  }

  private func observed<Value>(_ value: Value) -> Value {
    _ = revision
    return value
  }

  var hasPreview: Bool {
    objectType != .trackRecording
  }

  var actionBarItems: ActionBarItems {
    var items = ActionBarItems()
    if data.isRoutePoint {
      items.visible = [.routeRemoveStop]
      return items
    }
    switch data.roadType {
    case .toll:
      items.visible = [.avoidToll]
      return items
    case .ferry:
      items.visible = [.avoidFerry]
      return items
    case .dirty:
      items.visible = [.avoidDirty]
      return items
    default:
      break
    }

    var additional: [PlacePageAction] = []
    func append(_ actions: [PlacePageAction]) {
      guard let first = actions.first else { return }
      items.visible.append(first)
      additional.append(contentsOf: actions.dropFirst())
    }

    switch mapNodeStatus {
    case .downloading, .applying, .inQueue, .error, .notDownloaded, .partly:
      items.visible.append(.download)
    default:
      break
    }

    let hasPhones = !(data.infoData?.phones ?? []).isEmpty && AppInfo.shared().canMakeCalls
    switch objectType {
    case .POI, .bookmark, .track:
      var first: [PlacePageAction] = isRoutePlanning ? [.routeFrom] : []
      if hasPhones {
        first.append(.call)
      }
      if !isRoutePlanning {
        first.append(.routeFrom)
      }
      append(first)
      var second: [PlacePageAction] = canAddStop ? [.routeAddStop] : []
      second.append(objectType == .track ? .deleteTrack : .bookmark)
      append(second)
      items.visible.append(.routeTo)
    case .trackRecording:
      append([.discardTrackRecording, .saveTrackRecording])
    @unknown default:
      break
    }

    if additional.count == 1 {
      items.visible.append(contentsOf: additional)
    } else {
      items.overflow = additional
    }
    return items
  }

  func stop() {
    guard let observer else { return }
    LocationManager.remove(observer: observer)
    bookmarksManager.remove(observer)
    if isObservingTrackPoints {
      bookmarksManager.resetElevationActivePointChanged()
      bookmarksManager.resetElevationMyPositionChanged()
      isObservingTrackPoints = false
    }
    data.onBookmarkStatusUpdate = nil
    data.onTrackRecordingProgressUpdate = nil
    data.onMapNodeStatusUpdate = nil
    data.onMapNodeProgressUpdate = nil
    self.observer = nil
  }

  func refresh() {
    switch data.objectType {
    case .bookmark:
      guard let bookmark = data.bookmarkData, bookmarksManager.hasBookmark(bookmark.bookmarkId) else {
        close()
        return
      }
    case .track:
      guard let track = data.trackData, bookmarksManager.hasTrack(track.trackId) else {
        close()
        return
      }
    default:
      return
    }
    FrameworkHelper.updatePlacePageData()
    data.updateBookmarkStatus()
  }

  // MARK: - Actions

  func close() {
    MapViewController.shared()?.dismissPlacePage()
  }

  func expand() {
    PlacePageModel.shared.expand()
  }

  func copy(_ text: String) {
    UIPasteboard.general.string = text
    UIImpactFeedbackGenerator(style: .medium).impactOccurred()
    Toast.show(withText: String(format: L("copied_to_clipboard"), text), alignment: .bottom)
  }

  func openURL(_ url: String) {
    MapViewController.shared()?.openUrl(url, externally: true)
  }

  func open(in app: OpenInApplication) {
    openURL(app.linkWith(coordinates: data.locationCoordinate, destinationName: preview.title))
  }

  func call(_ phone: PlacePagePhone) {
    MWMPlacePageManagerHelper.call(phone)
  }

  func share(from anchor: UIView?) {
    let activity: ActivityViewController
    switch objectType {
    case .POI, .bookmark:
      activity = ActivityViewController.share(forPlacePage: data)
    default:
      guard let coordinate = LocationManager.lastLocation()?.coordinate else {
        AlertPresenter.shared.presentUnknownCurrentPositionAlert()
        return
      }
      activity = ActivityViewController.share(forMyPosition: coordinate)
    }
    activity.present(inParentViewController: UIViewController.topViewController(), anchorView: anchor)
  }

  func exportTrack(_ type: KmlFileType, from anchor: UIView?) {
    guard let trackId = track?.trackId else { return }
    let title = preview.title ?? ""
    bookmarksManager.shareTrack(trackId, fileType: type) { status, url in
      switch status {
      case .success:
        guard let url else { return }
        let activity = ActivityViewController.share(for: url, message: title) { _, _, _, _ in
          BookmarksManager.shared().finishSharing()
        }
        activity.present(inParentViewController: UIViewController.topViewController(), anchorView: anchor)
      case .emptyCategory:
        AlertPresenter.shared.presentInfoAlert(title: L("bookmarks_error_title_share_empty"),
                                               message: L("bookmarks_error_message_share_empty"))
      case .archiveError, .fileError:
        AlertPresenter.shared.presentInfoAlert(title: L("dialog_routing_system_error"),
                                               message: L("bookmarks_error_message_share_general"))
      }
    }
  }

  func perform(_ action: PlacePageAction) {
    switch action {
    case .download:
      toggleMapDownload()
    case .call:
      if let phone = data.infoData?.phones.first {
        call(phone)
      }
    case .routeFrom:
      MWMPlacePageManagerHelper.route(from: data)
    case .routeTo:
      MWMPlacePageManagerHelper.route(to: data)
    case .routeAddStop:
      MWMPlacePageManagerHelper.routeAddStop(data)
    case .routeRemoveStop:
      MWMPlacePageManagerHelper.routeRemoveStop(data)
    case .bookmark:
      if bookmark != nil {
        didRemoveBookmark = true
        MWMPlacePageManagerHelper.removeBookmark(data)
      } else {
        MWMPlacePageManagerHelper.addBookmark(data)
      }
    case .deleteTrack:
      guard track != nil else { return }
      MWMPlacePageManagerHelper.removeTrack(data)
      close()
    case .saveTrackRecording:
      TrackRecordingManager.shared.stopAndSave { [weak self] result in
        switch result {
        case .success:
          break
        case .trackIsEmpty:
          self?.close()
        }
      }
    case .discardTrackRecording:
      TrackRecordingManager.shared.stop { [weak self] _ in
        self?.close()
      }
    case .avoidToll:
      MWMPlacePageManagerHelper.avoidToll()
    case .avoidDirty:
      MWMPlacePageManagerHelper.avoidDirty()
    case .avoidFerry:
      MWMPlacePageManagerHelper.avoidFerry()
    }
  }

  func toggleMapDownload() {
    guard let countryId = data.mapNodeAttributes?.countryId else { return }
    switch mapNodeStatus {
    case .downloading, .inQueue, .applying:
      Storage.shared().cancelDownloadNode(countryId)
    case .notDownloaded, .partly, .onDiskOutOfDate, .error:
      Storage.shared().downloadNode(countryId)
    default:
      break
    }
  }

  func pickColor() {
    guard let userMark else { return }
    let pickerType: ColorPickerType
    switch userMark.kind {
    case .bookmark(let bookmark):
      pickerType = .bookmarkColorPicker(bookmark.color)
    case .track:
      pickerType = .defaultColorPicker(userMark.color)
    }
    ColorPicker.shared.present(from: UIViewController.topViewController(), pickerType: pickerType) { [weak self] color in
      self?.updateBookmarkOrTrack(color: color)
    }
  }

  func pickCategory() {
    guard let observer, let userMark else { return }
    let picker = SelectBookmarkGroupViewController(groupName: userMark.category ?? "", groupId: userMark.groupId)
    picker.delegate = observer
    UIViewController.topViewController().present(UINavigationController(rootViewController: picker), animated: true)
  }

  func editBookmarkOrTrack() {
    switch userMark?.kind {
    case .bookmark:
      MWMPlacePageManagerHelper.editBookmark(data)
    case .track:
      MWMPlacePageManagerHelper.editTrack(data)
    case nil:
      break
    }
  }

  func selectElevationPoint(_ point: CLLocationCoordinate2D, distance: Double) {
    guard let track, track.elevationProfileData?.isTrackRecording == false else { return }
    bookmarksManager.setElevationActivePoint(point, distance: distance, trackId: track.trackId)
    track.updateActivePointDistance(distance)
  }

  // MARK: - Updates

  private func start() {
    let observer = PlacePageObserver(content: self)
    self.observer = observer
    bookmarksManager.add(observer)
    data.onBookmarkStatusUpdate = { [weak self] in
      self?.reload()
    }
    data.onTrackRecordingProgressUpdate = { [weak self] in
      self?.reload()
    }
    data.onMapNodeStatusUpdate = { [weak self] in
      self?.reload()
    }
    data.onMapNodeProgressUpdate = { [weak self] downloadedBytes, totalBytes in
      guard totalBytes > 0 else { return }
      self?.downloadProgress = Double(downloadedBytes) / Double(totalBytes)
    }
    if hasPreview {
      LocationManager.add(observer: observer)
      if let location = LocationManager.lastLocation() {
        didUpdateLocation(location)
      }
      if let heading = LocationManager.lastHeading() {
        didUpdateHeading(heading)
      }
    }
    observeTrackPoints()
  }

  private func reload() {
    revision += 1
    if mapNodeStatus != .downloading {
      downloadProgress = nil
    }
    if objectType == .track, track == nil {
      close()
    }
  }

  private func observeTrackPoints() {
    guard objectType == .track, let track else { return }
    isObservingTrackPoints = true
    activePointDistance = track.activePointDistance
    myPositionDistance = track.myPositionDistance
    bookmarksManager.setElevationActivePointChanged(track.trackId) { [weak self] distance in
      self?.activePointDistance = distance
      self?.data.trackData?.updateActivePointDistance(distance)
    }
    bookmarksManager.setElevationMyPositionChanged(track.trackId) { [weak self] distance in
      self?.myPositionDistance = distance
    }
  }

  private func updateBookmarkOrTrack(color: UIColor? = nil, category: MWMMarkGroupID? = nil) {
    guard let userMark else { return }
    let category = category ?? userMark.groupId
    switch userMark.kind {
    case .bookmark(let bookmark):
      let bookmarkColor = color.flatMap { BookmarkColor.bookmarkColor(from: $0) } ?? bookmark.color
      MWMPlacePageManagerHelper.updateBookmark(data, color: bookmarkColor, category: category)
    case .track:
      MWMPlacePageManagerHelper.updateTrack(data, color: color ?? userMark.color, category: category)
    }
  }

  fileprivate func didSelectCategory(_ groupId: MWMMarkGroupID) {
    updateBookmarkOrTrack(category: groupId)
  }

  fileprivate func didDeleteCategory(_ groupId: MWMMarkGroupID) {
    if bookmark?.bookmarkGroupId == groupId {
      close()
    }
  }

  fileprivate func didUpdateLocation(_ location: CLLocation) {
    if data.isMyPosition {
      let altitude = "▲ \(AltitudeFormatter.altitudeString(fromMeters: location.altitude))"
      if location.speed > 0 && location.timestamp.timeIntervalSinceNow >= -2 {
        let speed = Measure(asSpeed: location.speed)
        speedAndAltitude = "\(altitude)  \(LocationManager.speedSymbolFor(location.speed))\(speed.valueAsString) \(speed.unit)"
      } else {
        speedAndAltitude = altitude
      }
    } else {
      let target = CLLocation(latitude: data.locationCoordinate.latitude, longitude: data.locationCoordinate.longitude)
      distance = DistanceFormatter.distanceString(fromMeters: location.distance(from: target))
      lastLocation = location
    }
  }

  fileprivate func didUpdateHeading(_ heading: CLHeading) {
    guard !data.isMyPosition, let lastLocation, heading.trueHeading > 0 else { return }
    let bearing = Double(GeoUtil.angle(atPoint: lastLocation.coordinate, toPoint: data.locationCoordinate))
    let target = .pi / 2 - bearing - heading.trueHeading * .pi / 180
    guard let current = direction?.radians else {
      direction = .radians(target)
      return
    }
    var delta = (target - current).truncatingRemainder(dividingBy: 2 * .pi)
    if delta > .pi {
      delta -= 2 * .pi
    } else if delta < -.pi {
      delta += 2 * .pi
    }
    direction = .radians(current + delta)
  }
}

private final class PlacePageObserver: NSObject, MWMLocationObserver, BookmarksObserver, SelectBookmarkGroupViewControllerDelegate {
  private weak var content: PlacePageContent?

  init(content: PlacePageContent) {
    self.content = content
  }

  func onLocationUpdate(_ location: CLLocation) {
    MainActor.assumeIsolated {
      content?.didUpdateLocation(location)
    }
  }

  func onHeadingUpdate(_ heading: CLHeading) {
    MainActor.assumeIsolated {
      content?.didUpdateHeading(heading)
    }
  }

  func onBookmarksLoadFinished() {
    MainActor.assumeIsolated {
      content?.refresh()
    }
  }

  func onBookmarksCategoryDeleted(_ groupId: MWMMarkGroupID) {
    MainActor.assumeIsolated {
      content?.didDeleteCategory(groupId)
    }
  }

  func bookmarkGroupViewController(_ viewController: SelectBookmarkGroupViewController,
                                   didSelect groupTitle: String,
                                   groupId: MWMMarkGroupID) {
    MainActor.assumeIsolated {
      viewController.dismiss(animated: true)
      content?.didSelectCategory(groupId)
    }
  }
}
