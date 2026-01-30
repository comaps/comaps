extension Settings {
    /// A language
    protocol Language: Codable, Identifiable, Equatable, Comparable {
        // MARK: Properties
        
        /// The id
        var id: String { get }
        
        
        /// The localized name
        var localizedName: String { get }
    }
}



// MARK: - Equatable
extension Settings.Language {
    static func == (lhs: Self, rhs: Self) -> Bool {
        return lhs.id == rhs.id
    }
}



// MARK: - Comparable
extension Settings.Language {
    static func < (lhs: Self, rhs: Self) -> Bool {
        return lhs.localizedName.localizedCaseInsensitiveCompare(rhs.localizedName) == .orderedAscending
    }
}
