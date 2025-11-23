import CarPlay

final class MapTemplateBuilder {
  enum MapButtonType {
    case startPanning
    case zoomIn
    case zoomOut
  }
  enum BarButtonType {
    case dismissPaning
    case destination
    case recenter
    case settings
    case mute
    case unmute
    case redirectRoute
    case endRoute
  }
  
  private enum Constants {
    static let carPlayGuidanceBackgroundColor = UIColor(46, 100, 51, 1.0)
  }
  
  // MARK: - CPMapTemplate builders
  class func buildBaseTemplate(positionMode: MWMMyPositionMode) -> CPMapTemplate {
    let mapTemplate = CPMapTemplate()
    mapTemplate.hidesButtonsWithNavigationBar = false
    configureBaseUI(mapTemplate: mapTemplate)
    if positionMode == .pendingPosition {
      mapTemplate.leadingNavigationBarButtons = []
    } else if positionMode == .follow || positionMode == .followAndRotate {
      setupDestinationButton(mapTemplate: mapTemplate)
    } else {
      setupRecenterButton(mapTemplate: mapTemplate)
    }
    return mapTemplate
  }
  
  class func buildNavigationTemplate() -> CPMapTemplate {
    let mapTemplate = CPMapTemplate()
    mapTemplate.hidesButtonsWithNavigationBar = false
    configureNavigationUI(mapTemplate: mapTemplate)
    return mapTemplate
  }
  
  class func buildTripPreviewTemplate(forTrips trips: [CPTrip]) -> CPMapTemplate {
    let mapTemplate = CPMapTemplate()
    mapTemplate.userInfo = MapInfo(type: CPConstants.TemplateType.preview, trips: trips)
    mapTemplate.mapButtons = []
    mapTemplate.leadingNavigationBarButtons = []
    let settingsButton = buildBarButton(type: .settings) { _ in
      mapTemplate.userInfo = MapInfo(type: CPConstants.TemplateType.previewSettings)
      let gridTemplate = SettingsTemplateBuilder.buildGridTemplate()
      CarPlayService.shared.pushTemplate(gridTemplate, animated: true)
    }
    mapTemplate.trailingNavigationBarButtons = [settingsButton]
    return mapTemplate
  }
  
  // MARK: - MapTemplate UI configs
  class func configureBaseUI(mapTemplate: CPMapTemplate) {
    mapTemplate.userInfo = MapInfo(type: CPConstants.TemplateType.main)
    let panningButton = buildMapButton(type: .startPanning) { _ in
      mapTemplate.showPanningInterface(animated: true)
    }
    let zoomInButton = buildMapButton(type: .zoomIn) { _ in
      FrameworkHelper.zoomMap(.in)
    }
    let zoomOutButton = buildMapButton(type: .zoomOut) { _ in
      FrameworkHelper.zoomMap(.out)
    }
    mapTemplate.mapButtons = [panningButton, zoomInButton, zoomOutButton]
    
    let settingsButton = buildBarButton(type: .settings) { _ in
      let gridTemplate = SettingsTemplateBuilder.buildGridTemplate()
      CarPlayService.shared.pushTemplate(gridTemplate, animated: true)
    }
    mapTemplate.trailingNavigationBarButtons = [settingsButton]
  }
  
  class func configurePanUI(mapTemplate: CPMapTemplate) {
    let zoomInButton = buildMapButton(type: .zoomIn) { _ in
      FrameworkHelper.zoomMap(.in)
    }
    let zoomOutButton = buildMapButton(type: .zoomOut) { _ in
      FrameworkHelper.zoomMap(.out)
    }
    mapTemplate.mapButtons = [zoomInButton, zoomOutButton]
    
    let doneButton = buildBarButton(type: .dismissPaning) { _ in
      mapTemplate.dismissPanningInterface(animated: true)
    }
    mapTemplate.leadingNavigationBarButtons = []
    mapTemplate.trailingNavigationBarButtons = [doneButton]
  }
  
  class func configureNavigationUI(mapTemplate: CPMapTemplate) {
    mapTemplate.userInfo = MapInfo(type: CPConstants.TemplateType.navigation)
    let panningButton = buildMapButton(type: .startPanning) { _ in
      mapTemplate.showPanningInterface(animated: true)
    }
    mapTemplate.mapButtons = [panningButton]
    setupMuteAndRedirectButtons(template: mapTemplate)
    let endButton = buildBarButton(type: .endRoute) { _ in
      CarPlayService.shared.cancelCurrentTrip()
    }
    mapTemplate.trailingNavigationBarButtons = [endButton]
    mapTemplate.guidanceBackgroundColor = Constants.carPlayGuidanceBackgroundColor
  }
  
  // MARK: - Conditional navigation buttons
  class func setupDestinationButton(mapTemplate: CPMapTemplate) {
    let destinationButton = buildBarButton(type: .destination) { _ in
      let listTemplate = ListTemplateBuilder.buildListTemplate(for: .history)
      CarPlayService.shared.pushTemplate(listTemplate, animated: true)
    }
    mapTemplate.leadingNavigationBarButtons = [destinationButton]
  }
  
  class func setupRecenterButton(mapTemplate: CPMapTemplate) {
    let recenterButton = buildBarButton(type: .recenter) { _ in
      FrameworkHelper.switchMyPositionMode()
    }
    mapTemplate.leadingNavigationBarButtons = [recenterButton]
  }
  
  private class func setupMuteAndRedirectButtons(template: CPMapTemplate) {
    let redirectButton = buildBarButton(type: .redirectRoute) { _ in
      let listTemplate = ListTemplateBuilder.buildListTemplate(for: .history)
      CarPlayService.shared.pushTemplate(listTemplate, animated: true)
    }
    if MWMTextToSpeech.isTTSEnabled() {
      let muteButton = buildBarButton(type: .mute) { _ in
        MWMTextToSpeech.tts().active = false
        setupUnmuteAndRedirectButtons(template: template)
      }
      template.leadingNavigationBarButtons = [muteButton, redirectButton]
    } else {
      template.leadingNavigationBarButtons = [redirectButton]
    }
  }
  
  private class func setupUnmuteAndRedirectButtons(template: CPMapTemplate) {
    let redirectButton = buildBarButton(type: .redirectRoute) { _ in
      let listTemplate = ListTemplateBuilder.buildListTemplate(for: .history)
      CarPlayService.shared.pushTemplate(listTemplate, animated: true)
    }
    if MWMTextToSpeech.isTTSEnabled() {
      let unmuteButton = buildBarButton(type: .unmute) { _ in
        MWMTextToSpeech.tts().active = true
        setupMuteAndRedirectButtons(template: template)
      }
      template.leadingNavigationBarButtons = [unmuteButton, redirectButton]
    } else {
      template.leadingNavigationBarButtons = [redirectButton]
    }
  }
  
  // MARK: - CPMapButton builder
  private class func buildMapButton(type: MapButtonType, action: ((CPMapButton) -> Void)?) -> CPMapButton {
    let button = CPMapButton(handler: action)
    switch type {
    case .startPanning:
      button.image = UIImage(systemName: "arrow.up.and.down.and.arrow.left.and.right")
    case .zoomIn:
      button.image = UIImage(systemName: "plus")
    case .zoomOut:
      button.image = UIImage(systemName: "minus")
    }
    // Remove code below once Apple has fixed its issue with the button background
    if #unavailable(iOS 26) {
      switch type {
      case .startPanning:
        button.focusedImage = UIImage(systemName: "smallcircle.filled.circle.fill")
      case .zoomIn:
        button.focusedImage = UIImage(systemName: "plus.circle.fill")
      case .zoomOut:
        button.focusedImage = UIImage(systemName: "minus.circle.fill")
      }
    }
    return button
  }
  
  // MARK: - CPBarButton builder
  private class func buildBarButton(type: BarButtonType, action: ((CPBarButton) -> Void)?) -> CPBarButton {
    switch type {
    case .dismissPaning:
      return CPBarButton(title: L("done"), handler: action)
    case .destination:
      return CPBarButton(title: L("pick_destination"), handler: action)
    case .recenter:
      return CPBarButton(title: L("follow_my_position"), handler: action)
    case .settings:
      return CPBarButton(image: UIImage(systemName: "gearshape.fill")!, handler: action)
    case .mute:
      return CPBarButton(image: UIImage(systemName: "speaker.wave.3")!, handler: action)
    case .unmute:
      return CPBarButton(image: UIImage(systemName: "speaker.slash")!, handler: action)
    case .redirectRoute:
      return CPBarButton(image: UIImage(named: "ic_carplay_redirect_route")!, handler: action)
    case .endRoute:
      return CPBarButton(title: L("navigation_stop_button").capitalized, handler: action)
    }
  }
}
