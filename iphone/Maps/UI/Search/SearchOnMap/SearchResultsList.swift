import SwiftUI

struct SearchResultsList: View {
  let results: [SearchResult]
  let model: SearchModel

  var body: some View {
    List(results, id: \.index) { result in
      Button {
        model.select(result)
      } label: {
        SearchResultRow(result: result)
      }
      .listRowInsets(SearchView.rowInsets)
      .listRowBackground(Color.clear)
    }
    .listStyle(.plain)
    .scrollContentBackground(.hidden)
    .scrollDismissesKeyboard(.immediately)
  }
}
