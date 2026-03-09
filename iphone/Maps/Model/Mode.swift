import SwiftUI

@objc enum Mode: Int, Identifiable, CaseIterable {
    case walking
    case cycling
    case driving
    case publicTransport
    
    
    
    // MARK: Properties
    
    /// The id
    public var id: Int {
        return Int(self.rawValue)
    }
    
    
    /// The description text
    var description: String {
        switch self {
            case .walking:
                return String(localized: "mode_walking")
            case .cycling:
                return String(localized: "mode_cycling")
            case .driving:
                return String(localized: "mode_driving")
            case .publicTransport:
                return String(localized: "mode_public-transport")
            @unknown default:
                fatalError()
        }
    }
    
    
    /// The color
    var color: Color {
        switch self {
            case .walking:
                return .ModeColors.walking
            case .cycling:
                return .ModeColors.cycling
            case .driving:
                return .ModeColors.driving
            case .publicTransport:
                return .ModeColors.publicTransport
            @unknown default:
                fatalError()
        }
    }
    
    
    /// The image
    var image: Image {
        return Image(imageName)
    }
    
    
    /// The image name
    var imageName: String {
        switch self {
            case .walking:
                return "walking"
            case .cycling:
                return "cycling"
            case .driving:
                return "driving"
            case .publicTransport:
                return "publictransport"
            @unknown default:
                fatalError()
        }
    }
}
