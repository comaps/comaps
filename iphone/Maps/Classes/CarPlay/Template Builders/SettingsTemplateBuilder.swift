import CarPlay

final class SettingsTemplateBuilder {
  // MARK: - CPGridTemplate builder
  class func buildGridTemplate() -> CPGridTemplate {
    let actions = SettingsTemplateBuilder.buildGridButtons()
    let gridTemplate = CPGridTemplate(title: L("settings"),
                                      gridButtons: actions)
    
    return gridTemplate
  }
  
  private class func buildGridButtons() -> [CPGridButton] {
    let options = RoutingOptions()
    return [createTollButton(options: options),
            createUnpavedButton(options: options),
            createPavedButton(options: options),
            createMotorwayButton(options: options),
            createFerryButton(options: options),
            createSpeedcamButton()]
  }
  
  // MARK: - CPGridButton builders
  private class func createTollButton(options: RoutingOptions) -> CPGridButton {
    var tollIconName = "tolls.circle"
    if options.avoidToll { tollIconName += ".slash" }
    let image = gridImage(named: tollIconName)
    let tollButton = CPGridButton(titleVariants: [L("avoid_tolls")], image: image) { _ in
                                    options.avoidToll = !options.avoidToll
                                    options.save()
                                    CarPlayService.shared.updateRouteAfterChangingSettings()
                                    CarPlayService.shared.popTemplate(animated: true)
    }
    return tollButton
  }
  
  private class func createUnpavedButton(options: RoutingOptions) -> CPGridButton {
    var unpavedIconName = "unpaved.circle"
    if options.avoidDirty && !options.avoidPaved { unpavedIconName += ".slash" }
    let image = gridImage(named: unpavedIconName)
    let unpavedButton = CPGridButton(titleVariants: [L("avoid_unpaved")], image: image) { _ in
                                      options.avoidDirty = !options.avoidDirty
                                      if options.avoidDirty {
                                        options.avoidPaved = false
                                      }
                                      options.save()
                                      CarPlayService.shared.updateRouteAfterChangingSettings()
                                      CarPlayService.shared.popTemplate(animated: true)
    }
    unpavedButton.isEnabled = !options.avoidPaved
    return unpavedButton
  }
    
  private class func createPavedButton(options: RoutingOptions) -> CPGridButton {
    var pavedIconName = "paved.circle"
    if options.avoidPaved && !options.avoidDirty { pavedIconName += ".slash" }
    let image = gridImage(named: pavedIconName)
    let pavedButton = CPGridButton(titleVariants: [L("avoid_paved")], image: image) { _ in
                                      options.avoidPaved = !options.avoidPaved
                                      if options.avoidPaved {
                                        options.avoidDirty = false
                                      }
                                      options.save()
                                      CarPlayService.shared.updateRouteAfterChangingSettings()
                                      CarPlayService.shared.popTemplate(animated: true)
    }
    pavedButton.isEnabled = !options.avoidDirty
    return pavedButton
  }
    
  private class func createMotorwayButton(options: RoutingOptions) -> CPGridButton {
    var motorwayIconName = "motorways.circle"
    if options.avoidMotorway { motorwayIconName += ".slash" }
    let image = gridImage(named: motorwayIconName)
    let motorwayButton = CPGridButton(titleVariants: [L("avoid_motorways")], image: image) { _ in
                                      options.avoidMotorway = !options.avoidMotorway
                                      options.save()
                                      CarPlayService.shared.updateRouteAfterChangingSettings()
                                      CarPlayService.shared.popTemplate(animated: true)
    }
    return motorwayButton
  }
  
  private class func createFerryButton(options: RoutingOptions) -> CPGridButton {
    var ferryIconName = "ferries.circle"
    if options.avoidFerry { ferryIconName += ".slash" }
    let image = gridImage(named: ferryIconName)
    let ferryButton = CPGridButton(titleVariants: [L("avoid_ferry")], image: image) { _ in
                                    options.avoidFerry = !options.avoidFerry
                                    options.save()
                                    CarPlayService.shared.updateRouteAfterChangingSettings()
                                    CarPlayService.shared.popTemplate(animated: true)
    }
    return ferryButton
  }
  
  private class func createSpeedcamButton() -> CPGridButton {
    var speedcamIconName = "speedcamera.circle"
    let isSpeedCamActivated = CarPlayService.shared.isSpeedCamActivated
    if !isSpeedCamActivated { speedcamIconName += ".slash" }
    let image = gridImage(named: speedcamIconName)
    let speedButton = CPGridButton(titleVariants: [L("speedcams_alert_title_carplay_1"), L("speedcams_alert_title_carplay_2")], image: image) { _ in
                                    CarPlayService.shared.isSpeedCamActivated = !isSpeedCamActivated
                                    CarPlayService.shared.popTemplate(animated: true)
    }
    return speedButton
  }
  
  // MARK: - Image helper
  private class func gridImage(named name: String) -> UIImage {
    let configuration = UIImage.SymbolConfiguration(textStyle: .title1)
    guard let symbol = UIImage(named: name, in: nil, with: configuration) else { return UIImage() }
    let scale = CarPlayService.shared.carDisplayScale
    let asset = UIImageAsset()
    asset.register(render(symbol, tint: .black, scale: scale), with: traits(for: .light, scale: scale))
    asset.register(render(symbol, tint: .white, scale: scale), with: traits(for: .dark, scale: scale))
    return asset.image(with: traits(for: .light, scale: scale))
  }

  private class func render(_ symbol: UIImage, tint: UIColor, scale: CGFloat) -> UIImage {
    let format = UIGraphicsImageRendererFormat()
    format.scale = scale
    format.opaque = false
    let bitmap = UIGraphicsImageRenderer(size: symbol.size, format: format).image { _ in
      symbol.withTintColor(tint, renderingMode: .alwaysOriginal).draw(at: .zero)
    }
    return bitmap.withRenderingMode(.alwaysOriginal)
  }

  private class func traits(for style: UIUserInterfaceStyle, scale: CGFloat) -> UITraitCollection {
    return UITraitCollection(traitsFrom: [
      UITraitCollection(userInterfaceStyle: style),
      UITraitCollection(displayScale: scale),
    ])
  }
}
