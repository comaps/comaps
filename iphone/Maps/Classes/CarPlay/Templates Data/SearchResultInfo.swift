struct SearchResultInfo: InfoMetadata {
  let results: [MWMCarPlaySearchResultObject]
  let selectedIndex: Int

  init(results: [MWMCarPlaySearchResultObject], selectedIndex: Int) {
    self.results = results
    self.selectedIndex = selectedIndex
  }

  var selectedFirstResults: [MWMCarPlaySearchResultObject]? {
    return CarPlaySearchResultOrdering.selectedFirst(results, selectedIndex: selectedIndex)
  }
}

enum CarPlaySearchResultOrdering {
  static func selectedFirst<Result>(_ results: [Result], selectedIndex: Int) -> [Result]? {
    guard results.indices.contains(selectedIndex) else { return nil }
    guard selectedIndex != results.startIndex else { return results }

    var reorderedResults = results
    let selectedResult = reorderedResults.remove(at: selectedIndex)
    reorderedResults.insert(selectedResult, at: reorderedResults.startIndex)
    return reorderedResults
  }
}
