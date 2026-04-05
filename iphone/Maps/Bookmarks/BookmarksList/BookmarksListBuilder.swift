final class BookmarksListBuilder {
  static func build(markGroupId: MWMMarkGroupID,
                    bookmarksCoordinator: BookmarksCoordinator?,
                    delegate: UIViewController? = nil) -> BookmarksListViewController {
    let viewController = BookmarksListViewController()
    let router = BookmarksListRouter(MapViewController.shared()!, bookmarksCoordinator: bookmarksCoordinator)
    let interactor = BookmarksListInteractor(markGroupId: markGroupId)
    let presenter = BookmarksListPresenter(view: viewController,
                                           router: router,
                                           delegate: delegate,
                                           interactor: interactor)
    viewController.presenter = presenter
    return viewController
  }
}
