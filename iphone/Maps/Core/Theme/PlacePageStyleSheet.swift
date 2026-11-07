enum PlacePageStyleSheet: String, CaseIterable {
  case ppRouteBasePreview = "RouteBasePreview"
  case ppRoutePreview = "RoutePreview"
  case ppChartView = "ChartView"
}

extension PlacePageStyleSheet: IStyleSheet {
  func styleResolverFor(colors: IColors, fonts: IFonts) -> Theme.StyleResolver {
    switch self {
    case .ppRouteBasePreview:
      return .add { s in
        s.borderColor = colors.blackDividers
        s.borderWidth = 1
        s.backgroundColor = colors.white
      }
    case .ppRoutePreview:
      return .add { s in
        s.shadowRadius = 2
        s.shadowColor = colors.blackDividers
        s.shadowOpacity = 1
        s.shadowOffset = CGSize(width: 3, height: 0)
        s.backgroundColor = colors.pressBackground
      }
    case .ppChartView:
      return .add { s in
        s.backgroundColor = colors.white
        s.fontColor = colors.blackSecondaryText
        s.font = fonts.regular12
        s.gridColor = colors.blackDividers
        s.previewSelectorColor = colors.elevationPreviewSelector
        s.previewTintColor = colors.elevationPreviewTint
        s.shadowOpacity = 0.25
        s.shadowColor = colors.shadow
        s.infoBackground = colors.pressBackground
      }
    }
  }
}
