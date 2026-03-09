import SwiftUI

extension MapCustomButton {
    /// The kind of the custom map button
    enum Kind: String, Codable, CaseIterable, Identifiable {
        case addPlace = "AddPlace"
        case recordTrack = "RecordTrack"
        case shareLocation = "ShareLocation"
        case favourites = "Favourites"
        case downloadMaps = "DownloadMaps"
        case settings = "Settings"
        case information = "Information"
        
        
        
        // MARK: Properties
        
        /// The id
        var id: Self { self }
        
        
        /// The description text
        var description: String {
            switch self {
                case .addPlace:
                    return String(localized: "placepage_add_place_button")
                case .recordTrack:
                    return MapControls.isRecordingTrack ? String(localized: "recording_track") : String(localized: "record_track")
                case .shareLocation:
                    return String(localized: "share_my_location")
                case .favourites:
                    return String(localized: "bookmarks_and_tracks")
                case .downloadMaps:
                    return String(localized: "download_maps")
                case .settings:
                    return String(localized: "settings")
                case .information:
                    return String(localized: "help")
            }
        }
        
        
        /// The image
        var image: Image {
            switch self {
                case .addPlace:
                    return Image(systemName: "plus")
                case .recordTrack:
                    return MapControls.isRecordingTrack ? Image("track.badge.record") : Image("track")
                case .shareLocation:
                    return Image(systemName: "square.and.arrow.up")
                case .favourites:
                    return Image(systemName: "star.fill")
                case .downloadMaps:
                    return MapControls.hasMapUpdates ? Image("download.badge") : Image("download")
                case .settings:
                    return Image(systemName: "gearshape.fill")
                case .information:
                    return Image(systemName: "info.circle")
            }
        }
        
        
        /// The action
        var action: () {
            switch self {
                case .addPlace:
                    return NotificationCenter.default.post(Notification(name: MapControls.presentAddPlaceNotificationName))
                case .recordTrack:
                    return NotificationCenter.default.post(Notification(name: MapControls.presentRecordTrackNotificationName))
                case .shareLocation:
                    return NotificationCenter.default.post(Notification(name: MapControls.presentShareLocationNotificationName))
                case .favourites:
                    return NotificationCenter.default.post(Notification(name: MapControls.presentFavouritesNotificationName))
                case .downloadMaps:
                    return NotificationCenter.default.post(Notification(name: MapControls.presentMapDownloadsNotificationName))
                case .settings:
                    return NotificationCenter.default.post(Notification(name: MapControls.presentSettingsNotificationName))
                case .information:
                    return NotificationCenter.default.post(Notification(name: MapControls.presentInformationNotificationName))
            }
        }
        
        
        /// If a badge is being shown
        var hasBadge: Bool {
            return (self == .downloadMaps && MapControls.hasMapUpdates) || (self == .recordTrack && MapControls.isRecordingTrack)
        }
    }
}
