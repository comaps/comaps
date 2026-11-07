enum PlacePageStyleSheet: String, CaseIterable {
  case ppRouteBasePreview = "RouteBasePreview"
  case ppRoutePreview = "RoutePreview"
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
    }
  }
}
