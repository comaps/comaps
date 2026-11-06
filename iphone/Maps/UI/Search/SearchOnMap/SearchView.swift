import SwiftUI

@available(iOS 17.0, *)
struct SearchView: View {
  enum Placement {
    case sheet
    case panel
  }

  let model: SearchModel
  let placement: Placement

  @FocusState private var isFieldFocused: Bool

  var body: some View {
    VStack(spacing: 0) {
      SearchHeader(model: model, placement: placement, isFieldFocused: $isFieldFocused)
      SearchContent(model: model)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
    .onAppear {
      isFieldFocused = model.isTyping
    }
    .onChange(of: model.isTyping) {
      isFieldFocused = model.isTyping
    }
    .onChange(of: isFieldFocused) {
      model.didChangeFocus(isFieldFocused)
    }
  }
}

// MARK: - Header
@available(iOS 17.0, *)
private struct SearchHeader: View {
  let model: SearchModel
  let placement: SearchView.Placement
  let isFieldFocused: FocusState<Bool>.Binding

  var body: some View {
    HStack(spacing: 12) {
      SearchField(model: model, isFocused: isFieldFocused)
      SearchCloseButton(action: model.didTapClose)
        .keyboardShortcut(.cancelAction)
    }
    .padding(.horizontal)
    .padding(.top, placement == .sheet ? 20 : 12)
    .padding(.bottom, 12)
  }
}

@available(iOS 17.0, *)
private struct SearchCloseButton: View {
  let action: @MainActor () -> Void

  var body: some View {
    if #available(iOS 26.0, *) {
      Button(role: .close, action: action) {
        Label(L("close"), systemImage: "xmark")
      }
      .labelStyle(.iconOnly)
      .buttonStyle(.glass)
      .buttonBorderShape(.circle)
    } else {
      Button(action: action) {
        Image(systemName: "xmark")
          .font(.subheadline.weight(.semibold))
          .foregroundStyle(.secondary)
          .padding(9)
          .background(Color(uiColor: .tertiarySystemFill), in: .circle)
          .frame(minWidth: 44, minHeight: 44)
          .contentShape(.rect)
      }
      .buttonStyle(.plain)
      .accessibilityLabel(L("close"))
    }
  }
}

@available(iOS 17.0, *)
private struct SearchField: View {
  @Bindable var model: SearchModel
  let isFocused: FocusState<Bool>.Binding

  var body: some View {
    HStack(spacing: 6) {
      Image(systemName: "magnifyingglass")
        .foregroundStyle(.secondary)
        .accessibilityHidden(true)
      TextField(L("search"), text: $model.query)
        .focused(isFocused)
        .submitLabel(.search)
        .onSubmit(model.submit)
        .accessibilityAddTraits(.isSearchField)
      if !model.text.isEmpty {
        Button {
          model.query = ""
        } label: {
          Image(systemName: "xmark.circle.fill")
            .foregroundStyle(.secondary)
            .frame(minWidth: 44, minHeight: 44)
            .contentShape(.rect)
        }
        .buttonStyle(.plain)
        .accessibilityLabel(L("clear"))
      }
    }
    .padding(.leading, 8)
    .padding(.trailing, model.text.isEmpty ? 8 : 0)
    .frame(minHeight: 44)
    .background(Color(uiColor: .tertiarySystemFill), in: .rect(cornerRadius: 10))
  }
}

// MARK: - Content
@available(iOS 17.0, *)
private struct SearchContent: View {
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

@available(iOS 17.0, *)
private struct SearchResultsList: View {
  let results: [SearchResult]
  let model: SearchModel

  var body: some View {
    List(results, id: \.index) { result in
      Button {
        model.select(result)
      } label: {
        SearchResultRow(result: result)
      }
      .listRowBackground(Color.clear)
    }
    .listStyle(.plain)
    .scrollContentBackground(.hidden)
    .scrollDismissesKeyboard(.immediately)
  }
}

@available(iOS 17.0, *)
private struct SearchHistoryAndCategories: View {
  let model: SearchModel

  @AppStorage("SearchTabViewController_selectedIndexKey") private var selectedTab = SearchModel.Tab.categories

  var body: some View {
    VStack(spacing: 0) {
      Picker(L("search"), selection: $selectedTab) {
        Text(L("history")).tag(SearchModel.Tab.history)
        Text(L("categories")).tag(SearchModel.Tab.categories)
      }
      .pickerStyle(.segmented)
      .labelsHidden()
      .padding(.horizontal)
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

@available(iOS 17.0, *)
private struct SearchHistoryList: View {
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
                  .foregroundStyle(.secondary)
              }
            }
            .listRowBackground(Color.clear)
          }
        }
        Section {
          Button(L("clear_search"), role: .destructive, action: model.clearHistory)
            .listRowBackground(Color.clear)
        }
      }
      .listStyle(.plain)
      .scrollContentBackground(.hidden)
      .scrollDismissesKeyboard(.immediately)
    }
  }
}

@available(iOS 17.0, *)
private struct SearchCategoriesList: View {
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
      .listRowBackground(Color.clear)
    }
    .listStyle(.plain)
    .scrollContentBackground(.hidden)
    .scrollDismissesKeyboard(.immediately)
  }
}
