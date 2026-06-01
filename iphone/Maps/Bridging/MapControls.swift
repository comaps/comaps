import Combine
import SwiftUI

extension MapControls {
    /// If the map controls are hidden
    @objc static var areMapControlsHidden: Bool {
        get {
            return UserDefaults.standard.bool(forKey: mapControlsHiddenKey)
        }
        set {
            UserDefaults.standard.set(newValue, forKey: mapControlsHiddenKey)
        }
    }
    
    
    /// If the zoom buttons are hidden
    @objc static var areMapZoomButtonsHidden: Bool {
        get {
            return UserDefaults.standard.bool(forKey: mapZoomButtonsHiddenKey)
        }
        set {
            UserDefaults.standard.set(newValue, forKey: mapZoomButtonsHiddenKey)
        }
    }
    
    
    /// The mode
    static var mode: Mode {
        get {
            return Mode(rawValue: modeRawValue()) ?? .walking
        }
        set {
            setModeRawValue(newValue.rawValue)
        }
    }
    
    
    /// If the outdoor layer is being used
    @objc static var hasOutdoorLayer: Bool {
        get {
            return outdoorLayerEnabled()
        }
        set {
            setOutdoorLayerEnabled(newValue)
        }
    }
    
    
    /// If the contour lines layer is being used
    @objc static var hasContourLinesLayer: Bool {
        get {
            return contourLinesLayerEnabled()
        }
        set {
            setContourLinesLayerEnabled(newValue)
        }
    }
    
    
    /// If the buildings 3D layer is being used
    @objc static var hasBuildings3dLayer: Bool {
        get {
            return buildings3dLayerEnabled()
        }
        set {
            setBuildings3dLayerEnabled(newValue)
        }
    }
    
    
    /// The position mode
    static var positionMode: MapPositionButton.Mode {
        return MapPositionButton.Mode(rawValue: MapControls.positionModeRawValue()) ?? .locate
    }
    
    
    /// If there are map updates
    static var hasMapUpdates: Bool {
        return MapsAppDelegate.theApp().badgeNumber() > 0
    }
    
    
    /// If a track is currently being recorded
    static var isRecordingTrack: Bool {
        return TrackRecordingManager.shared.recordingState == .active
    }
    
    
    /// The key for storing if map controls are hidden
    static private let mapControlsHiddenKey: String = "MapControlsHidden"
    
    
    /// The key for storing if zoom buttons are hidden
    static private let mapZoomButtonsHiddenKey: String = "MapZoomButtonsHidden"
    
    
    /// The notification name for presenting the add place sheet
    @objc static let presentAddPlaceNotificationName: Notification.Name = Notification.Name(rawValue: "PresentAddPlace")
    
    
    /// The notification name for presenting the record track sheet
    @objc static let presentRecordTrackNotificationName: Notification.Name = Notification.Name(rawValue: "PresentRecordTrack")
    
    
    /// The notification name for presenting the location sharing sheet
    @objc static let presentShareLocationNotificationName: Notification.Name = Notification.Name(rawValue: "PresentShareLocation")
    
    
    /// The notification name for presenting the favourites sheet
    @objc static let presentFavouritesNotificationName: Notification.Name = Notification.Name(rawValue: "PresentFavourites")
    
    
    /// The notification name for presenting the search sheet
    @objc static let presentSearchNotificationName: Notification.Name = Notification.Name(rawValue: "PresentSearch")
    
    
    /// The notification name for presenting the map downloads sheet
    @objc static let presentMapDownloadsNotificationName: Notification.Name = Notification.Name(rawValue: "PresentMapDownloads")
    
    
    /// The notification name for presenting the settings sheet
    @objc static let presentSettingsNotificationName: Notification.Name = Notification.Name(rawValue: "PresentSettings")
    
    
    /// The notification name for presenting the profile sheet
    @objc static let presentProfileNotificationName: Notification.Name = Notification.Name(rawValue: "PresentProfile")
    
    
    /// The notification name for presenting the information sheet
    @objc static let presentInformationNotificationName: Notification.Name = Notification.Name(rawValue: "PresentInformation")
    
    
    /// The notification name for switching position mode
    @objc static let switchPositionModeNotificationName: Notification.Name = Notification.Name(rawValue: "SwitchPositionMode")
    
    
    /// The notification name for changing the track recording state
    @objc static let changeTrackRecordingNotificationName: Notification.Name = Notification.Name(rawValue: "ChangeTrackRecording")
    
    
    
    /// The notification name for updating maps
    @objc static let mapUpdatesNotificationName: Notification.Name = Notification.Name(rawValue: "MapUpdates")
    
    
    
    // MARK: Methods
    
    /// Zoom to a specific scale
    /// - Parameter scale: The scale
    @objc static func zoom(scale: CGFloat) {
        zoomScale(scale)
    }
}
