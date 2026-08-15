import CarPlay

struct BookmarkListTemplateContext {
  let categoryId: MWMMarkGroupID
}

enum CarPlayListLimiter {
  static func applyingMaximumItemCount<Item>(_ maximumItemCount: Int,
                                              to items: [Item],
                                              overflowItem: @autoclosure () -> Item) -> [Item] {
    guard maximumItemCount > 0 else { return [] }
    guard items.count > maximumItemCount else { return items }

    var limitedItems = Array(items.prefix(maximumItemCount - 1))
    limitedItems.append(overflowItem())
    return limitedItems
  }
}

final class ListTemplateBuilder {
  enum ListTemplateType {
    case history
    case bookmarkLists
    case bookmarks(category: BookmarkGroup)
    case searchResults(results: [MWMCarPlaySearchResultObject])
  }
  
  enum BarButtonType {
    case bookmarks
    case search
  }
  
  // MARK: - CPListTemplate builder
  class func buildListTemplate(for type: ListTemplateType) -> CPListTemplate {
    var title = ""
    var trailingNavigationBarButtons = [CPBarButton]()
    switch type {
    case .history:
      title = L("pick_destination")
      let bookmarksButton = buildBarButton(type: .bookmarks) { _ in
        let listTemplate = ListTemplateBuilder.buildListTemplate(for: .bookmarkLists)
        CarPlayService.shared.pushTemplate(listTemplate, animated: true)
      }
      let searchButton = buildBarButton(type: .search) { _ in
        if CarPlayService.shared.isKeyboardLimited {
          CarPlayService.shared.showKeyboardAlert()
        } else {
          let searchTemplate = SearchTemplateBuilder.buildSearchTemplate()
          CarPlayService.shared.pushTemplate(searchTemplate, animated: true)
        }
      }
      trailingNavigationBarButtons = [searchButton, bookmarksButton]
    case .bookmarkLists:
      title = L("bookmarks")
    case .searchResults:
      title = L("search_results")
    case .bookmarks(let category):
      title = category.title
    }

    let sections = buildSectionsForType(type)
    let template = CPListTemplate(title: title, sections: sections)
    if case .bookmarks(let category) = type {
      template.userInfo = BookmarkListTemplateContext(categoryId: category.categoryId)
    }
    template.trailingNavigationBarButtons = trailingNavigationBarButtons
    return template
  }

  class func refreshBookmarks(in template: CPListTemplate, categoryId: MWMMarkGroupID) {
    template.updateSections(buildBookmarksSections(categoryId: categoryId))
  }

  private class func buildSectionsForType(_ type: ListTemplateType) -> [CPListSection] {
    switch type {
    case .history:
      return buildHistorySections()
    case .bookmarks(let category):
      return buildBookmarksSections(categoryId: category.categoryId)
    case .bookmarkLists:
      return buildBookmarkListsSections()
    case .searchResults(let results):
      return buildSearchResultsSections(results)
    }
  }

  private class func buildHistorySections() -> [CPListSection] {
    let searchQueries = FrameworkHelper.obtainLastSearchQueries()
    let items = searchQueries.map({ (text) -> CPListItem in
      let item = CPListItem(text: text, detailText: nil, image: UIImage(named: "recent"))
      item.userInfo = ListItemInfo(type: CPConstants.ListItemType.history,
                                   metadata: nil)
      configureSelectionHandler(for: item)
      return item
    })
    return [CPListSection(items: items)]
  }

  private class func buildBookmarkListsSections() -> [CPListSection] {
    let bookmarkManager = BookmarksManager.shared()
    let categories = bookmarkManager.sortedUserCategories()
    let items: [CPListItem] = categories.compactMap({ category in
      if category.bookmarksCount == 0 { return nil }
      let placesString = category.placesCountTitle()
      let item = CPListItem(text: category.title, detailText: placesString)
      item.userInfo = ListItemInfo(type: CPConstants.ListItemType.bookmarkLists,
                                   metadata: CategoryInfo(category: category))
      configureSelectionHandler(for: item)
      return item
    })
    return [CPListSection(items: items)]
  }

  private class func buildBookmarksSections(categoryId: MWMMarkGroupID) -> [CPListSection] {
    let bookmarkManager = BookmarksManager.shared()
    let bookmarks = bookmarkManager.bookmarks(forCategory: categoryId)
    var items = bookmarks.map({ (bookmark) -> CPListItem in
      let item = CPListItem(text: bookmark.prefferedName, detailText: bookmark.address)
      item.userInfo = ListItemInfo(type: CPConstants.ListItemType.bookmarks,
                                   metadata: BookmarkInfo(categoryId: categoryId,
                                                          bookmarkId: bookmark.bookmarkId))
      configureSelectionHandler(for: item)
      return item
    })
    let sourceItemCount = items.count
    let maximumItemCount = CPListTemplate.maximumItemCount
    let cropWarning = CPListItem(text: L("not_all_shown_bookmarks_carplay"),
                                 detailText: L("switch_to_phone_bookmarks_carplay"))
    cropWarning.isEnabled = false
    items = CarPlayListLimiter.applyingMaximumItemCount(maximumItemCount,
                                                        to: items,
                                                        overflowItem: cropWarning)
    LOG(.info,
        "[CarPlayList] bookmarks category=\(categoryId) source=\(sourceItemCount) maximum=\(maximumItemCount) displayed=\(items.count) limited=\(CarPlayService.shared.isListLimited)")
    return [CPListSection(items: items)]
  }

  private class func buildSearchResultsSections(_ results: [MWMCarPlaySearchResultObject]) -> [CPListSection] {
    return [CPListSection(items: buildSearchResultItems(results, configureHandlers: true))]
  }

  class func buildSearchResultItems(_ results: [MWMCarPlaySearchResultObject],
                                    configureHandlers: Bool) -> [CPListItem] {
    return results.enumerated().map({ index, object in
      let item = CPListItem(text: object.title, detailText: object.address)
      item.userInfo = ListItemInfo(type: CPConstants.ListItemType.searchResults,
                                   metadata: SearchResultInfo(results: results, selectedIndex: index))
      if configureHandlers {
        configureSelectionHandler(for: item)
      }
      return item
    })
  }

  class func configureSelectionHandler(for item: CPListItem) {
    item.handler = { selectedItem, completionHandler in
      guard let selectedItem = selectedItem as? CPListItem else {
        completionHandler()
        return
      }
      CarPlayService.shared.handleListItemSelection(selectedItem,
                                                    completionHandler: completionHandler)
    }
  }

  // MARK: - CPBarButton builder
  private class func buildBarButton(type: BarButtonType, action: ((CPBarButton) -> Void)?) -> CPBarButton {
    switch type {
    case .bookmarks:
      return CPBarButton(image: UIImage(systemName: "list.star")!, handler: action)
    case .search:
      return CPBarButton(image: UIImage(systemName: "keyboard.fill")!, handler: action)
    }
  }
}
