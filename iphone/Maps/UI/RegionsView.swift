import SwiftUI

/// View for the about information
struct RegionsView: View {
    // MARK: Properties
    
    /// The dismiss action of the environment
    @Environment(\.dismiss) private var dismiss
    
    var region: Region = RegionalManager.root
    
    var isOnlyShowingDownloaded = false
    
    @State var regionGroupings: [Grouping<Region>] = []
    
    var filteredRegionGroupings: [Grouping<Region>] {
        if searchTerms.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
            return regionGroupings
        } else {
            return [Grouping(key: String(), value: region.subregions(for: searchTerms))]
        }
    }
    
    @State var searchTerms: String = ""
    
    /// The time to refresh the current date
    private let timer = Timer.publish(every: 1, on: .current, in: .common).autoconnect()
    
    
    /// The current date
    @State var now: Date = Date.now
    
    
    
    /// The actual view
    var body: some View {
        List {
            ForEach(filteredRegionGroupings) { regionGrouping in
                Section {
                    ForEach(regionGrouping.value) { regionInGrouping in
                        if regionInGrouping.hasSubregions {
                            NavigationLink {
                                RegionsView(region: regionInGrouping, isOnlyShowingDownloaded: isOnlyShowingDownloaded)
                            } label: {
                                RegionView(region: regionInGrouping, isOnlyShowingDownloaded: isOnlyShowingDownloaded)
                            }
                        } else {
                            RegionView(region: regionInGrouping, isOnlyShowingDownloaded: isOnlyShowingDownloaded)
                        }
                    }
                } header: {
                    Text(regionGrouping.key)
                }
            }
        }
        .accentColor(.accent)
        .searchable(text: $searchTerms, placement: .navigationBarDrawer, prompt: "Search")
        .navigationBarTitleDisplayMode(.inline)
        .navigationTitle(region.name)
        .toolbar {
            ToolbarItem(placement: .destructiveAction) {
                if isOnlyShowingDownloaded, region != RegionalManager.root {
                        Button(role: .destructive) {
                            RegionalManager.delete(for: region.id)
                        } label: {
                            Text("delete")
                    }
                }
            }
        }
        .onAppear {
            var groupings: Set<Grouping<Region>> = []
            let subregions = region.subregions.filter { subregion in
                return !isOnlyShowingDownloaded || subregion.hasDownloads
            }
            for subregion in subregions {
                if let firstCharacter = subregion.name.localizedUppercase.first {
                    let firstCharacterAsString = subregions.count > 20 ? String(firstCharacter) : String()
                    if let grouping = groupings.first(where: { grouping in
                        grouping.key == firstCharacterAsString
                    }) {
                        var subregions = grouping.value
                        subregions.append(subregion)
                        groupings.update(with: Grouping(key: firstCharacterAsString, value: subregions))
                    } else {
                        groupings.insert(Grouping(key: firstCharacterAsString, value: [subregion]))
                    }
                }
            }
            regionGroupings = groupings.sorted()
        }
        .onReceive(timer) {_ in
            if isOnlyShowingDownloaded {
                var groupings: Set<Grouping<Region>> = []
                let subregions = region.subregions.filter { subregion in
                    return !isOnlyShowingDownloaded || subregion.hasDownloads
                }
                for subregion in subregions {
                    if let firstCharacter = subregion.name.localizedUppercase.first {
                        let firstCharacterAsString = subregions.count > 20 ? String(firstCharacter) : String()
                        if let grouping = groupings.first(where: { grouping in
                            grouping.key == firstCharacterAsString
                        }) {
                            var subregions = grouping.value
                            subregions.append(subregion)
                            groupings.update(with: Grouping(key: firstCharacterAsString, value: subregions))
                        } else {
                            groupings.insert(Grouping(key: firstCharacterAsString, value: [subregion]))
                        }
                    }
                }
                regionGroupings = groupings.sorted()
            }
        }
        
    }
}
