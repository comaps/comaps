import SwiftUI

struct SearchHistoryAndCategories: View {
  let model: SearchModel

  @AppStorage("SearchTabViewController_selectedIndexKey") private var selectedTab = SearchModel.Tab.categories

  var body: some View {
    VStack(spacing: 0) {
      Picker(L("search"), selection: $selectedTab) {
        Text(L("categories")).tag(SearchModel.Tab.categories)
        Text(L("history")).tag(SearchModel.Tab.history)
      }
      .pickerStyle(.segmented)
      .labelsHidden()
      .padding(.horizontal, SearchView.horizontalMargin)
      .padding(.bottom, 8)

      switch selectedTab {
      case .history:
        SearchHistoryList(history: model.history, model: model)
      case .categories:
        SearchCategoriesList(categories: model.categories, model: model)
      }
    }
    .onAppear {
      if model.history.isEmpty {
        selectedTab = .categories
      }
    }
  }
}
