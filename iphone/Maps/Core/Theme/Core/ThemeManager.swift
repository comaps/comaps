@objc(MWMThemeManager)
final class ThemeManager: NSObject {
  private static let autoUpdatesInterval: TimeInterval = 30 * 60 // 30 minutes in seconds

  private static let instance = ThemeManager()
  private weak var timer: Timer?

  private override init() {
    super.init()
  }

  private func update(theme: MWMTheme) {
    updateSystemUserInterfaceStyle(theme)

    let actualTheme: MWMTheme = { theme in
      let isVehicleRouting = MWMRouter.isRoutingActive() && (MWMRouter.type() == .vehicle) && MWMRouter.hasSavedRoute()
      switch theme {
      case .day: fallthrough
      case .vehicleDay: return isVehicleRouting ? .vehicleDay : .day
      case .night: fallthrough
      case .vehicleNight: return isVehicleRouting ? .vehicleNight : .night
      case .auto:
        let isDarkModeEnabled = UIScreen.main.traitCollection.userInterfaceStyle == .dark
        guard isVehicleRouting else { return isDarkModeEnabled ? .night : .day }
        return isDarkModeEnabled ? .vehicleNight : .vehicleDay
      @unknown default:
        fatalError()
      }
    }(theme)

    let nightMode = UIColor.isNightMode()
    let newNightMode: Bool = { theme in
      switch theme {
      case .day: fallthrough
      case .vehicleDay: return false
      case .night: fallthrough
      case .vehicleNight: return true
      case .auto: assert(false); return false
      @unknown default:
        fatalError()
      }
    }(actualTheme)

    let isCarPlayActive = CarPlayService.shared.isCarplayActivated
    if !isCarPlayActive, Settings.mapAppearance == .light {
      if actualTheme == .vehicleDay || actualTheme == .vehicleNight {
          FrameworkHelper.setTheme(.vehicleDay)
      } else {
          FrameworkHelper.setTheme(.day)
      }
    } else if !isCarPlayActive, Settings.mapAppearance == .dark {
      if actualTheme == .vehicleDay || actualTheme == .vehicleNight {
        FrameworkHelper.setTheme(.vehicleNight)
      } else {
        FrameworkHelper.setTheme(.night)
      }
    } else {
      FrameworkHelper.setTheme(actualTheme)
    }
    if nightMode != newNightMode || StyleManager.shared.hasTheme() == false{
      UIColor.setNightMode(newNightMode)
      if newNightMode {
        StyleManager.shared.setTheme(MainTheme(type: .dark, colors: NightColors(), fonts: Fonts()))
      } else {
        StyleManager.shared.setTheme(MainTheme(type: .light, colors: DayColors(), fonts: Fonts()))
      }
    }
  }

  @objc static func invalidate() {
    instance.update(theme: SettingsBridge.theme())
  }

  @available(iOS 13.0, *)
  private func updateSystemUserInterfaceStyle(_ theme: MWMTheme) {
    let userInterfaceStyle: UIUserInterfaceStyle = { theme in
      switch theme {
      case .day: fallthrough
      case .vehicleDay: return .light
      case .night: fallthrough
      case .vehicleNight: return .dark
      case .auto: return .unspecified
      @unknown default:
        fatalError()
      }
    }(theme)
    UIApplication.shared.delegate?.window??.overrideUserInterfaceStyle = userInterfaceStyle
  }

  @available(iOS, deprecated:13.0)
  @objc static var autoUpdates: Bool {
    get {
      return instance.timer != nil
    }
    set {
      if newValue {
        instance.timer = Timer.scheduledTimer(timeInterval: autoUpdatesInterval, target: self, selector: #selector(invalidate), userInfo: nil, repeats: true)
      } else {
        instance.timer?.invalidate()
      }
      invalidate()
    }
  }
}
