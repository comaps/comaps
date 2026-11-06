import Observation

@available(iOS 17.0, *)
@MainActor
@Observable
final class SearchModel {
  static let shared = SearchModel()

  enum Tab: Int {
    case history
    case categories
  }

  private(set) var isOpen = false
  private(set) var contentState = SearchOnMap.ViewModel.initial.contentState
  private(set) var step = SearchOnMap.ViewModel.initial.presentationStep
  private(set) var isTyping = SearchOnMap.ViewModel.initial.isTyping
  private(set) var text = ""
  private(set) var history: [String] = []
  private(set) var categories: [String] = []
  var isMapCovered = false
  var isSuspended = false

  var layout = SearchOnMap.Layout.sheet {
    didSet {
      guard layout != oldValue, let interactor else { return }
      interactor.layout = layout
      interactor.handle(.didUpdatePresentationStep(step))
    }
  }

  var isCollapsed: Bool { isOpen && layout == .sheet && step == .compact }

  var isPanelPresented: Bool {
    layout == .panel && isOpen && step != .hidden
  }

  var isSheetPresented: Bool {
    get { layout == .sheet && isOpen && step != .hidden && step != .compact && !isMapCovered && !isSuspended }
    set {
      if !newValue && isSheetPresented {
        if text.isEmpty {
          didTapClose()
        } else {
          didSelectStep(.compact)
        }
      }
    }
  }

  var query: String {
    get { text }
    set { didEditText(newValue) }
  }

  @ObservationIgnored private var interactor: SearchOnMapInteractor?
  private let frameworkHelper = MWMSearchFrameworkHelper.self

  private init() {}

  func open(with interactor: SearchOnMapInteractor) {
    self.interactor = interactor
    apply(.initial)
    text = ""
    if categories.isEmpty {
      categories = frameworkHelper.searchCategories()
    }
    reloadHistory()
    isOpen = true
    interactor.handle(.openSearch)
  }

  // MARK: - User actions
  func didEditText(_ newText: String) {
    guard newText != text else { return }
    text = newText
    guard !newText.isEmpty else {
      interactor?.handle(.clearButtonDidTap)
      return
    }
    interactor?.handle(.didType(query(newText)))
  }

  func submit() {
    guard !text.isEmpty else { return }
    interactor?.handle(.searchButtonDidTap(query(text)))
  }

  func didChangeFocus(_ isFocused: Bool) {
    guard isFocused != isTyping else { return }
    interactor?.handle(isFocused ? .didStartTyping : .didEndTyping)
  }

  func select(_ result: SearchResult) {
    interactor?.handle(.didSelectResult(result, withQuery: query(text)))
  }

  func select(history query: String) {
    interactor?.handle(.didSelect(SearchQuery(query.trimmingCharacters(in: .whitespacesAndNewlines), source: .history)))
  }

  func select(category: String) {
    let preferredLanguage = AppInfo.shared().languageId
    let language = frameworkHelper.isLanguageSupported(preferredLanguage) ? preferredLanguage : "en"
    interactor?.handle(.didSelect(SearchQuery(L(category, languageCode: language), locale: language, source: .category)))
  }

  func clearHistory() {
    frameworkHelper.clearSearchHistory()
    reloadHistory()
  }

  func didSelectStep(_ step: ModalPresentationStep) {
    interactor?.handle(.didUpdatePresentationStep(step))
  }

  func expand() {
    interactor?.handle(.didStartTyping)
  }

  func didTapClose() {
    interactor?.handle(.closeSearch)
  }

  // MARK: - Private methods
  private func reloadHistory() {
    var seen = Set<String>()
    history = frameworkHelper.lastSearchQueries().filter { seen.insert($0).inserted }
  }

  private func apply(_ viewModel: SearchOnMap.ViewModel) {
    contentState = viewModel.contentState
    step = viewModel.presentationStep
    isTyping = viewModel.isTyping
  }

  private func query(_ text: String) -> SearchQuery {
    SearchQuery(text, locale: keyboardLanguage, source: .typedText)
  }

  private var keyboardLanguage: String? {
    UIApplication.shared.connectedScenes
      .compactMap { ($0 as? UIWindowScene)?.keyWindow }
      .first?
      .textInputMode?
      .primaryLanguage
  }
}

// MARK: - SearchOnMapView
@available(iOS 17.0, *)
extension SearchModel: @preconcurrency SearchOnMapView {
  func render(_ viewModel: SearchOnMap.ViewModel) {
    if let searchingText = viewModel.searchingText {
      text = searchingText
    }
    if viewModel.contentState == .historyAndCategory {
      reloadHistory()
    }
    apply(viewModel)
  }

  func show() {
    interactor?.handle(.openSearch)
  }

  func close() {
    isOpen = false
    interactor = nil
  }
}
