import SwiftUI

struct SearchContent: View {
  let model: SearchModel

  var body: some View {
    VStack(spacing: 0) {
      switch model.contentState {
      case .historyAndCategory:
        SearchHistoryAndCategories(model: model)
      case .searching:
        ProgressView()
          .accessibilityLabel(L("search"))
          .frame(maxWidth: .infinity, maxHeight: .infinity)
      case .results(let results):
        SearchResultsList(results: results.results, model: model)
      case .noResults:
        ContentUnavailableView(L("search_not_found"),
                               systemImage: "magnifyingglass",
                               description: Text(L("search_not_found_query")))
      }
    }
    .onChange(of: model.contentState) { oldState, newState in
      switch (oldState, newState) {
      case (.results, .results):
        break
      case (_, .results):
        AccessibilityNotification.Announcement(L("search_results")).post()
      case (_, .noResults):
        AccessibilityNotification.Announcement(L("search_not_found")).post()
      default:
        break
      }
    }
  }
}
