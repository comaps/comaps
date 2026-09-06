import SwiftUI

extension MapZoomButton {
    /// The kind of the map zoom button
    enum Kind: String, Codable, CaseIterable, Identifiable {
        case `in` = "In"
        case out = "Out"
        
        
        
        // MARK: Properties
        
        /// The id
        var id: Self { self }
        
        
        /// The description text
        var description: String {
            switch self {
                case .in:
                    return String(localized: "zoom_in")
                case .out:
                    return String(localized: "zoom_out")
            }
        }
        
        
        /// The image
        var image: Image {
            switch self {
                case .in:
                    return Image(systemName: "plus")
                case .out:
                    return Image(systemName: "minus")
            }
        }
    }
}
