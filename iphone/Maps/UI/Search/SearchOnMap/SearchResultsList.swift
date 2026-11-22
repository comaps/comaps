import SwiftUI

struct SearchResultsList: View {
  let results: [SearchResult]
  let model: SearchModel

  @Environment(\.accessibilityReduceMotion) private var reduceMotion

  var body: some View {
    let rows = IdentifiedSearchResult.rows(for: results)
    List(rows) { row in
      Button {
        model.select(row.result)
      } label: {
        SearchResultRow(result: row.result)
      }
      .listRowInsets(SearchView.rowInsets)
      .listRowBackground(Color.clear)
    }
    .listStyle(.plain)
    .scrollContentBackground(.hidden)
    .scrollDismissesKeyboard(.immediately)
    .animation(reduceMotion ? nil : .snappy(duration: 0.25), value: rows.map(\.id))
  }
}

private struct IdentifiedSearchResult: Identifiable {
  struct Key: Hashable {
    let itemType: SearchItemType
    let title: String
    let suggestion: String
    let x: Double
    let y: Double

    init(_ result: SearchResult) {
      itemType = result.itemType
      title = result.titleText
      suggestion = result.itemType == .suggestion ? result.suggestion : ""
      x = result.point.x
      y = result.point.y
    }
  }

  struct ID: Hashable {
    let key: Key
    let occurrence: Int
  }

  let id: ID
  let result: SearchResult

  static func rows(for results: [SearchResult]) -> [IdentifiedSearchResult] {
    var occurrences: [Key: Int] = [:]
    return results.map { result in
      let key = Key(result)
      let occurrence = occurrences[key, default: 0]
      occurrences[key] = occurrence + 1
      return IdentifiedSearchResult(id: ID(key: key, occurrence: occurrence), result: result)
    }
  }
}
