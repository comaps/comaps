import CarPlay

@objc(CarPlayDashboardSceneDelegate)
final class CarPlayDashboardSceneDelegate: UIResponder, CPTemplateApplicationDashboardSceneDelegate {

  private weak var dashboardController: CPDashboardController?
  private var isObservingBookmarks = false

  func templateApplicationDashboardScene(
    _ dashboardScene: CPTemplateApplicationDashboardScene,
    didConnect dashboardController: CPDashboardController,
    to window: UIWindow
  ) {
    LOG(.info, "CarPlayDashboardSceneDelegate: didConnect")
    CarPlayService.shared.dashboardConnected(window: window)
    self.dashboardController = dashboardController
    refreshDashboardButtons()
    let manager = BookmarksManager.shared()
    if !isObservingBookmarks {
      isObservingBookmarks = true
      manager.add(self)
    }
    if !manager.areBookmarksLoaded() {
      manager.loadBookmarks()
    }
  }

  func templateApplicationDashboardScene(
    _ dashboardScene: CPTemplateApplicationDashboardScene,
    didDisconnect dashboardController: CPDashboardController,
    from window: UIWindow
  ) {
    LOG(.info, "CarPlayDashboardSceneDelegate: didDisconnect")
    if isObservingBookmarks {
      BookmarksManager.shared().remove(self)
      isObservingBookmarks = false
    }
    self.dashboardController = nil
    CarPlayService.shared.dashboardDisconnected()
    window.rootViewController = nil
  }

  func sceneDidBecomeActive(_ scene: UIScene) {
    LOG(.info, "[CarPlayHost] dashboard scene sceneDidBecomeActive (state=\(scene.activationState.rawValue))")
    CarPlayService.shared.dashboardDidBecomeActive()
  }

  func sceneWillResignActive(_ scene: UIScene) {
    LOG(.info, "[CarPlayHost] dashboard scene sceneWillResignActive")
    CarPlayService.shared.dashboardDidResignActive()
  }

  // MARK: - Private

  private func refreshDashboardButtons() {
    dashboardController?.shortcutButtons = makeDashboardButtons()
  }

  /// Show first two bookmarks on the dashboard, or a placeholder if none
  private func makeDashboardButtons() -> [CPDashboardButton] {
    let manager = BookmarksManager.shared()
    guard let category = manager.sortedUserCategories().first(where: { $0.bookmarksCount > 0 }) else {
      return [searchPlaceholderButton()]
    }
    let bookmarks = manager.bookmarks(forCategory: category.categoryId).suffix(2).reversed()
    guard !bookmarks.isEmpty else { return [searchPlaceholderButton()] }
    return bookmarks.map { bookmark in
      CPDashboardButton(
        titleVariants: [bookmark.prefferedName],
        subtitleVariants: bookmark.address.isEmpty ? [] : [bookmark.address],
        image: UIImage(systemName: "star.fill") ?? UIImage(),
        handler: { _ in
          CarPlayService.shared.navigateToBookmarkFromDashboard(bookmark: bookmark)
        }
      )
    }
  }

  /// TODO: Make the placeholder actually do something
  private func searchPlaceholderButton() -> CPDashboardButton {
    CPDashboardButton(
      titleVariants: [L("search")],
      subtitleVariants: [],
      image: UIImage(systemName: "magnifyingglass") ?? UIImage(),
      handler: { _ in }
    )
  }
}

// MARK: - BookmarksObserver
extension CarPlayDashboardSceneDelegate: BookmarksObserver {
  func onBookmarksLoadFinished() {
    refreshDashboardButtons()
  }

  func onBookmarkDeleted(_: MWMMarkID) {
    refreshDashboardButtons()
  }

  func onBookmarksCategoryDeleted(_: MWMMarkGroupID) {
    refreshDashboardButtons()
  }
}

final class CarPlayDashboardMapViewController: UIViewController {
  private(set) var mapView: EAGLView?
  private let placeholderImageView = UIImageView(image: UIImage(named: "ic_carplay_activated"))

  override func viewDidLoad() {
    super.viewDidLoad()
    view.backgroundColor = UIColor.carplayPlaceholderBackground()

    placeholderImageView.contentMode = .scaleAspectFit
    placeholderImageView.translatesAutoresizingMaskIntoConstraints = false
    placeholderImageView.isHidden = mapView != nil
    view.addSubview(placeholderImageView)
    NSLayoutConstraint.activate([
      placeholderImageView.centerXAnchor.constraint(equalTo: view.centerXAnchor),
      placeholderImageView.centerYAnchor.constraint(equalTo: view.centerYAnchor),
      placeholderImageView.widthAnchor.constraint(equalToConstant: 80),
      placeholderImageView.heightAnchor.constraint(equalToConstant: 80)
    ])
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    guard let mapView else { return }
    if !mapView.drapeEngineCreated && !MapsAppDelegate.isTestsEnvironment() {
      mapView.createDrapeEngine()
    }
    FrameworkHelper.setVisibleViewport(view.bounds, scaleFactor: mapView.contentScaleFactor)
    CarPlayService.shared.mapViewportDidBecomeReady(mapView)
  }

  func addMapView(_ mapView: EAGLView) {
    removeMapView()
    mapView.translatesAutoresizingMaskIntoConstraints = false
    self.mapView = mapView
    mapView.frame = view.bounds
    view.insertSubview(mapView, at: 0)
    NSLayoutConstraint.activate([
      mapView.topAnchor.constraint(equalTo: view.topAnchor),
      mapView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
      mapView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      mapView.trailingAnchor.constraint(equalTo: view.trailingAnchor)
    ])
    placeholderImageView.isHidden = true
  }

  func removeMapView() {
    if let mapView {
      mapView.removeFromSuperview()
      self.mapView = nil
    }
    placeholderImageView.isHidden = false
  }
}
