import Combine
import AVFoundation

/// The settings
@objc class Settings: NSObject {
    // MARK: Properties
    
    // The notification name for changed routing options
    static let routingOptionsChangedNotificationName: Notification.Name = Notification.Name(rawValue: "RoutingOptionsChanged")
    
    
    /// Key for storing if the sync beta alert has been shown in the user defaults
    static private let userDefaultsKeyHasShownSyncBetaAlert = "kUDDidShowICloudSynchronizationEnablingAlert"
    
    
    /// Key for storing the type of action used for the bottom left main interface button in the user defaults
    static private let userDefaultsKeyLeftButtonType = "LeftButtonType"
    
    
    /// Key for storing the map appearance in the user defaults
    static private let userDefaultsKeyMapAppearance = "MapAppearance"
    
    
    /// The current distance unit
    static var distanceUnit: DistanceUnit {
        get {
            if SettingsBridge.measurementUnits() == .imperial {
                return .imperial
            } else {
                return .metric
            }
        }
        set {
            if newValue == .imperial {
                SettingsBridge.setMeasurementUnits(.imperial)
            } else {
                SettingsBridge.setMeasurementUnits(.metric)
            }
        }
    }
    
    
    /// If zoom buttons should be displayed
    @objc static var hasZoomButtons: Bool {
        get {
            return SettingsBridge.zoomButtonsEnabled()
        }
        set {
            SettingsBridge.setZoomButtonsEnabled(newValue)
        }
    }
    
    
    /// The type of action used for the bottom left main interface button
    static var leftButtonType: LeftButtonType {
        get {
            if let leftButtonTypeRawValue = UserDefaults.standard.string(forKey: userDefaultsKeyLeftButtonType), let leftButtonType = LeftButtonType(rawValue: leftButtonTypeRawValue) {
                return leftButtonType
            }
            
            return .help
        }
        set {
            UserDefaults.standard.set(newValue.rawValue, forKey: userDefaultsKeyLeftButtonType)
        }
    }
    
    
    /// If 3D buildings should be displayed
    @objc static var has3dBuildings: Bool {
        get {
            return SettingsBridge.buildings3dViewEnabled()
        }
        set {
            SettingsBridge.setBuildings3dViewEnabled(newValue)
        }
    }
    
    
    /// If automatic map downloads should be enabled
    @objc static var hasAutomaticDownload: Bool {
        get {
            return SettingsBridge.autoDownloadEnabled()
        }
        set {
            SettingsBridge.setAutoDownloadEnabled(newValue)
        }
    }
    
    
    /// The current mobile data policy
    @objc static var mobileDataPolicy: MobileDataPolicy {
        get {
            let networkPolicyPermission = NetworkPolicy.shared().permission
            if networkPolicyPermission == .always {
                return .always
            } else if networkPolicyPermission == .never {
                return .never
            } else {
                return .ask
            }
        }
        set {
            if newValue == .always {
                NetworkPolicy.shared().permission = .always
            } else if newValue == .never {
                NetworkPolicy.shared().permission = .never
            } else {
                NetworkPolicy.shared().permission = .ask
            }
        }
    }
    
    
    /// The current power saving mode
    @objc static var powerSavingMode: PowerSavingMode {
        get {
            return PowerSavingMode(rawValue: SettingsBridge.powerManagement()) ?? .never
        }
        set {
            SettingsBridge.setPowerManagement(newValue.rawValue)
        }
    }
    
    
    /// If an increased font size should be used for map labels
    @objc static var hasIncreasedFontsize: Bool {
        get {
            return SettingsBridge.largeFontSize()
        }
        set {
            SettingsBridge.setLargeFontSize(newValue)
        }
    }
    
    
    /// If names should be transliterated to Latin
    @objc static var shouldTransliterateToLatin: Bool {
        get {
            return SettingsBridge.transliteration()
        }
        set {
            SettingsBridge.setTransliteration(newValue)
        }
    }
    
    
    /// The available languages for the map
    static var availableLanguagesForMap: [MapLanguage] {
        var languages = SettingsBridge.availableMapLanguages().map { language in
            return MapLanguage(id: language.key, localizedName: language.value)
        }.sorted()
        languages.insert(MapLanguage(id: "default", localizedName: String(localized: "pref_maplanguage_local")), at: 0)
        languages.insert(MapLanguage(id: "auto", localizedName: String(localized: "auto")), at: 0)
        return languages
    }
    
    
    /// The current language for the map
    static var languageForMap: MapLanguage.ID {
        get {
            return SettingsBridge.mapLanguageCode()
        }
        set {
            SettingsBridge.setMapLanguageCode(newValue)
        }
    }
    
    
    /// If the alternative languages for the app only should be used when they are the local native language
    static var shouldLimitMapLanguageAlternativesToLocal: Bool {
        get {
            return SettingsBridge.mapLanguageLimitAlternativesToLocal()
        }
        set {
            SettingsBridge.setMapLanguageLimitAlternativesToLocal(newValue)
        }
    }
    
    
    /// If the compass should be calibrated
    @objc static var shouldCalibrateCompass: Bool {
        get {
            return SettingsBridge.compassCalibrationEnabled()
        }
        set {
            SettingsBridge.setCompassCalibrationEnabled(newValue)
        }
    }
    
    
    /// The current map appearance
    @objc static var mapAppearance: Appearance {
        get {
            let mapAppearanceRawValue = UserDefaults.standard.integer(forKey: userDefaultsKeyMapAppearance)
            if mapAppearanceRawValue != 0, let mapAppearance = Appearance(rawValue: mapAppearanceRawValue) {
                return mapAppearance
            }
            
            return .auto
        }
        set {
            UserDefaults.standard.set(newValue.rawValue, forKey: userDefaultsKeyMapAppearance)
            ThemeManager.invalidate()
        }
    }
    
    
    /// The current appearance
    @objc static var appearance: Appearance {
        get {
            let theme = SettingsBridge.theme()
            if theme == MWMTheme.day {
                return .light
            } else if theme == MWMTheme.night {
                return .dark
            } else {
                return .auto
            }
        }
        set {
            if newValue == .light {
                SettingsBridge.setTheme(MWMTheme.day)
            } else if newValue == .dark {
                SettingsBridge.setTheme(MWMTheme.night)
            } else {
                SettingsBridge.setTheme(MWMTheme.auto)
            }
        }
    }
    
    
    /// If the bookmarks should be synced via iCloud
    @objc static var shouldSync: Bool {
        get {
            return SettingsBridge.iCLoudSynchronizationEnabled()
        }
        set {
            SettingsBridge.setICLoudSynchronizationEnabled(newValue)
        }
    }
    
    
    /// If the sync beta alert has been shown
    @objc static var hasShownSyncBetaAlert: Bool {
        get {
            return UserDefaults.standard.bool(forKey: userDefaultsKeyHasShownSyncBetaAlert)
        }
        set {
            UserDefaults.standard.set(newValue, forKey: userDefaultsKeyHasShownSyncBetaAlert)
        }
    }
    
    
    /// The publisher for state changes of the iCloud sync
    static var syncStatePublisher: AnyPublisher<SynchronizationManagerState, Never> {
        iCloudSynchronizaionManager.shared.notifyObservers()
        return iCloudSynchronizaionManager.shared.statePublisher
    }
    
    
    /// If our custom logging is enabled
    @objc static var isLogging: Bool {
        get {
            return SettingsBridge.isFileLoggingEnabled()
        }
        set {
            SettingsBridge.setFileLoggingEnabled(newValue)
        }
    }
    
    
    /// The formatter for the size of the log file
    @objc static var logSizeFormatter: MeasurementFormatter {
        let measurementFormatter = MeasurementFormatter()
        measurementFormatter.unitStyle = .medium
        measurementFormatter.unitOptions = .naturalScale
        return measurementFormatter
    }
    
    
    /// The size of the log file in bytes
    @objc static var logSize: Measurement<UnitInformationStorage> {
        return Measurement<UnitInformationStorage>(value: Double(SettingsBridge.logFileSize()), unit: .bytes)
    }
    
    
    /// If the perspective view should be used during routing
    @objc static var hasPerspectiveViewWhileRouting: Bool {
        get {
            return SettingsBridge.perspectiveViewEnabled()
        }
        set {
            SettingsBridge.setPerspectiveViewEnabled(newValue)
        }
    }
    
    
    /// If auto zoom should be used during routing
    @objc static var hasAutoZoomWhileRouting: Bool {
        get {
            return SettingsBridge.autoZoomEnabled()
        }
        set {
            SettingsBridge.setAutoZoomEnabled(newValue)
        }
    }
    
    
    /// If voice guidance should be provided during routing
    @objc static var shouldProvideVoiceRouting: Bool {
        get {
            return MWMTextToSpeech.isTTSEnabled()
        }
        set {
            MWMTextToSpeech.setTTSEnabled(newValue)
        }
    }
    
    
    /// The available languages for voice guidance during routing
    static var availableLanguagesForVoiceRouting: [VoiceRoutingLanguage] {
        return MWMTextToSpeech.availableLanguages().map { language in
            return VoiceRoutingLanguage(id: language.key, localizedName: language.value)
        }.sorted()
    }
    
    
    /// The current language for voice guidance during routing
    @objc static var languageForVoiceRouting: VoiceRoutingLanguage.ID {
        get {
            return MWMTextToSpeech.selectedLanguage()
        }
        set {
            MWMTextToSpeech.tts().setNotificationsLocale(newValue)
        }
    }
    
    
    /// The voice used for voice guidance during routing
    @objc static var voiceForVoiceRouting: String? {
        if let voice = MWMTextToSpeech.tts().voice() {
            return voice.name
        }
        
        return nil
    }
    
    
    /// If street names should be announced in the voice guidance during routing
    @objc static var shouldAnnounceStreetnamesWhileVoiceRouting: Bool {
        get {
            return MWMTextToSpeech.isStreetNamesTTSEnabled()
        }
        set {
            MWMTextToSpeech.setStreetNamesTTSEnabled(newValue)
        }
    }
    
    
    /// The current announcement of speed traps in the voice guidance during routing
    @objc static var announcingSpeedTrapsWhileVoiceRouting: AnnouncingSpeedTrapsWhileVoiceRouting {
        get {
            return AnnouncingSpeedTrapsWhileVoiceRouting(rawValue: MWMTextToSpeech.speedCameraMode()) ?? .never
        }
        set {
            MWMTextToSpeech.setSpeedCameraMode(newValue.rawValue)
        }
    }
    
    
    /// If toll roads should be avoided during routing
    @objc static var shouldAvoidTollRoadsWhileRouting: Bool {
        get {
            return RoutingOptions().avoidToll
        }
        set {
            let routingOptions = RoutingOptions()
            routingOptions.avoidToll = newValue
            routingOptions.save()
            
            NotificationCenter.default.post(name: routingOptionsChangedNotificationName, object: nil)
        }
    }
    
    
    /// If unpaved roads should be avoided during routing
    @objc static var shouldAvoidUnpavedRoadsWhileRouting: Bool {
        get {
            return RoutingOptions().avoidDirty
        }
        set {
            let routingOptions = RoutingOptions()
            routingOptions.avoidDirty = newValue
            routingOptions.save()
            
            NotificationCenter.default.post(name: routingOptionsChangedNotificationName, object: nil)
        }
    }
    
    
    /// If paved roads should be avoided during routing
    @objc static var shouldAvoidPavedRoadsWhileRouting: Bool {
        get {
            return RoutingOptions().avoidPaved
        }
        set {
            let routingOptions = RoutingOptions()
            routingOptions.avoidPaved = newValue
            routingOptions.save()
            
            NotificationCenter.default.post(name: routingOptionsChangedNotificationName, object: nil)
        }
    }
    
    
    /// If ferries should be avoided during routing
    @objc static var shouldAvoidFerriesWhileRouting: Bool {
        get {
            return RoutingOptions().avoidFerry
        }
        set {
            let routingOptions = RoutingOptions()
            routingOptions.avoidFerry = newValue
            routingOptions.save()
            
            NotificationCenter.default.post(name: routingOptionsChangedNotificationName, object: nil)
        }
    }
    
    
    /// If motorways should be avoided during routing
    @objc static var shouldAvoidMotorwaysWhileRouting: Bool {
        get {
            return RoutingOptions().avoidMotorway
        }
        set {
            let routingOptions = RoutingOptions()
            routingOptions.avoidMotorway = newValue
            routingOptions.save()
            
            NotificationCenter.default.post(name: routingOptionsChangedNotificationName, object: nil)
        }
    }
    
    
    /// If steps should be avoided during routing
    @objc static var shouldAvoidStepsWhileRouting: Bool {
        get {
            return RoutingOptions().avoidSteps
        }
        set {
            let routingOptions = RoutingOptions()
            routingOptions.avoidSteps = newValue
            routingOptions.save()
            
            NotificationCenter.default.post(name: routingOptionsChangedNotificationName, object: nil)
        }
    }
    
    
    
    // MARK: Methods
    
    /// Create a bookmarks backup before enabling the sync beta
    /// - Parameter completionHandler: A compeltion handler, which returns if a backup has been created
    static func createBookmarksBackupBecauseOfSyncBeta(completionHandler: ((_ hasCreatedBackup: Bool) -> Void)?) {
        BookmarksManager.shared().shareAllCategories { status, url in
            switch status {
                case .success:
                    let window = (UIApplication.shared.connectedScenes.filter { $0.activationState == .foregroundActive }.first(where: { $0 is UIWindowScene }) as? UIWindowScene)?.keyWindow
                    if let viewController = window?.rootViewController?.presentedViewController {
                        let shareController = ActivityViewController.share(for: url, message: String(localized: "share_bookmarks_email_body")) { _, _, _, _ in
                            completionHandler?(true)
                        }
                        shareController.present(inParentViewController: viewController, anchorView: nil)
                    }
                case .emptyCategory:
                    Toast.show(withText: String(localized: "bookmarks_error_title_share_empty"))
                    completionHandler?(false)
                case .archiveError:
                    Toast.show(withText: String(localized: "dialog_routing_system_error"))
                    completionHandler?(false)
                case .fileError:
                    Toast.show(withText: String(localized: "dialog_routing_system_error"))
                    completionHandler?(false)
            }
        }
    }
    
    
    /// Play a test audio snippet for voice guidance during routing
    @objc static func playVoiceRoutingTest() {
        MWMTextToSpeech.playTest()
    }
}
