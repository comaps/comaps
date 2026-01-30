import Foundation

/// The settings
struct Grouping<T>: Identifiable, Equatable, Comparable, Hashable where T: Comparable {
    // MARK: Properties
    var id: String {
        return key
    }
    
    var key: String
    
    var value: [T]
}



// MARK: - Equatable
extension Grouping {
    static func == (lhs: Self, rhs: Self) -> Bool {
        return lhs.key == rhs.key
    }
}



// MARK: - Comparable
extension Grouping {
    static func < (lhs: Self, rhs: Self) -> Bool {
        return lhs.key.localizedCaseInsensitiveCompare(rhs.key) == .orderedAscending
    }
}




// MARK: Hashable
extension Grouping {
    /// Hash the essential components of this value by feeding them into a hasher
    /// - Parameter hasher: The hasher
    func hash(into hasher: inout Hasher) {
        hasher.combine(key)
    }
}
