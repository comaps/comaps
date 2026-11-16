import SwiftUI

struct SearchCategoriesList: View {
  let categories: [String]
  let model: SearchModel

  @ScaledMetric(relativeTo: .body) private var iconSize: CGFloat = 28

  var body: some View {
    List(categories, id: \.self) { category in
      Button {
        model.select(category: category)
      } label: {
        Label {
          Text(L(category))
            .foregroundStyle(.primary)
        } icon: {
          Image(decorative: "Search/Categories/" + category.replacingOccurrences(of: "category_", with: ""))
            .resizable()
            .scaledToFit()
            .frame(width: iconSize, height: iconSize)
        }
      }
      .listRowInsets(SearchView.rowInsets)
      .listRowBackground(Color.clear)
    }
    .listStyle(.plain)
    .scrollContentBackground(.hidden)
    .scrollDismissesKeyboard(.immediately)
  }
}
