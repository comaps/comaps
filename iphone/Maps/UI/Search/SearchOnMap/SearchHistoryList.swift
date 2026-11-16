import SwiftUI

struct SearchHistoryList: View {
  let history: [String]
  let model: SearchModel

  var body: some View {
    if history.isEmpty {
      ContentUnavailableView(L("search_history_title"),
                             systemImage: "clock",
                             description: Text(L("search_history_text")))
    } else {
      List {
        Section {
          ForEach(history, id: \.self) { query in
            Button {
              model.select(history: query)
            } label: {
              Label {
                Text(query)
                  .foregroundStyle(.primary)
              } icon: {
                Image(systemName: "clock")
                  .foregroundStyle(.primary)
              }
            }
            .listRowInsets(SearchView.rowInsets)
            .listRowBackground(Color.clear)
          }
        }
        Section {
          Button(L("clear_search"), role: .destructive, action: model.clearHistory)
            .listRowInsets(SearchView.rowInsets)
            .listRowBackground(Color.clear)
        }
      }
      .listStyle(.plain)
      .scrollContentBackground(.hidden)
      .scrollDismissesKeyboard(.immediately)
    }
  }
}
