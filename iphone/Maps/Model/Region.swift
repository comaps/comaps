import Combine

/// The settings
struct Region: Identifiable, Equatable, Comparable, Hashable {
    // MARK: Properties
    
    var id: String
    /// The current distance unit
    var name: String
    
    var description: String? = nil
    
    var downloadingSize: Measurement<UnitInformationStorage>? = nil
    var downloadedSize: Measurement<UnitInformationStorage>? = nil
        
    var subregions: [Region] = []
    
    var hasSubregions: Bool {
        return !subregions.isEmpty
    }
    
    var hasDownloads: Bool {
        return (!hasSubregions && state.isDownloaded) || numberOfTotalDownloadedSubregions > 0
    }
    
    var state: State {
        return RegionalManager.state(for: id)
    }
    
    var numberOfTotalSubregions: Int {
        return subregions.reduce(0) { count, subregion in
            if subregion.numberOfTotalSubregions > 0 {
                return count + subregion.numberOfTotalSubregions
            } else {
                return count + 1
            }
        }
    }
    
    var numberOfTotalDownloadedSubregions: Int {
        if hasSubregions {
            return subregions.reduce(0) { count, subregion in
                return count + subregion.numberOfTotalDownloadedSubregions
            }
        } else if state.isDownloaded {
            return 1
        } else {
            return 0
        }
    }
    
    func subregions(for searchTerms: String) -> [Region] {
        var matchingSubregions: [Region] = []
        if !searchTerms.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
            for subregion in subregions {
                if subregion.name.contains(searchTerms) {
                    matchingSubregions.append(subregion)
                }
            }
            for subregion in subregions {
                if let description = subregion.description, description.contains(searchTerms), !matchingSubregions.contains(subregion) {
                    matchingSubregions.append(subregion)
                }
            }
            for subregion in subregions {
                for subsubregion in subregion.subregions(for: searchTerms) {
                    if !matchingSubregions.contains(subregion) {
                        matchingSubregions.append(subsubregion)
                    }
                }
            }
        }
        return matchingSubregions
    }
    
    enum State: Equatable, Hashable {
        case notDownloaded
        case notDownloadedAndError
        case downloading(Double)
        case downloaded
        case downloadedButOutdated
        case downloadedButError
        
        var isDownloaded: Bool {
            switch self {
                case .downloaded:
                    return true
                case .downloadedButOutdated:
                    return true
                case .downloadedButError:
                    return true
                default:
                    return false
            }
        }
    }
}



// MARK: - Equatable
extension Region {
    static func == (lhs: Self, rhs: Self) -> Bool {
        return lhs.id == rhs.id && lhs.state == rhs.state
    }
}



// MARK: - Comparable
extension Region {
    static func < (lhs: Self, rhs: Self) -> Bool {
        return lhs.name.localizedCaseInsensitiveCompare(rhs.name) == .orderedAscending
    }
}
