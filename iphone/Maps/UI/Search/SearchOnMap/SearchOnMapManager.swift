@objc
enum SearchOnMapState: Int {
  case searching
  case hidden
  case closed
}

@objc
enum SearchOnMapRoutingTooltipSearch: Int {
  case none
  case start
  case finish
}

@objc
protocol SearchOnMapManagerObserver: AnyObject {
  func searchManager(didChangeState state: SearchOnMapState)
}

@objcMembers
final class SearchOnMapManager: NSObject {
  private var interactor: SearchOnMapInteractor?
  private let observers = ListenerContainer<SearchOnMapManagerObserver>()

  var isSearching: Bool { interactor != nil }

  override init() {
    super.init()
  }

  // MARK: - Public methods
  @MainActor
  func startSearching(isRouting: Bool) {
    guard #available(iOS 17.0, *) else { return }
    if let interactor {
      interactor.handle(.openSearch)
      return
    }
    FrameworkHelper.deactivateMapSelection()
    let model = SearchModel.shared
    let presenter = SearchOnMapPresenter(isRouting: isRouting) { [weak self] state in
      self?.didChangeState(state)
    }
    let interactor = SearchOnMapInteractor(presenter: presenter, layout: model.layout)
    presenter.view = model
    self.interactor = interactor
    model.open(with: interactor)
  }

  func hide() {
    interactor?.handle(.hideSearch)
  }

  func close() {
    interactor?.handle(.closeSearch)
  }

  func setRoutingTooltip(_ tooltip: SearchOnMapRoutingTooltipSearch) {
    interactor?.routingTooltipSearch = tooltip
  }

  func setPlaceOnMapSelected(_ isSelected: Bool) {
    interactor?.handle(isSelected ? .didSelectPlaceOnMap : .didDeselectPlaceOnMap)
  }

  func setMapIsDragging() {
    interactor?.handle(.didStartDraggingMap)
  }

  @MainActor
  func setMapCovered(_ isCovered: Bool) {
    guard #available(iOS 17.0, *) else { return }
    SearchModel.shared.isMapCovered = isCovered
  }

  func searchText(_ searchText: SearchQuery) {
    interactor?.autoSelectFirstResult = searchText.autoSelectFirstResult
    interactor?.handle(.didSelect(searchText))
  }

  func addObserver(_ observer: SearchOnMapManagerObserver) {
    observers.addListener(observer)
  }

  func removeObserver(_ observer: SearchOnMapManagerObserver) {
    observers.removeListener(observer)
  }

  private func didChangeState(_ state: SearchOnMapState) {
    if state == .closed {
      interactor = nil
    }
    observers.forEach { observer in observer.searchManager(didChangeState: state) }
  }
}
