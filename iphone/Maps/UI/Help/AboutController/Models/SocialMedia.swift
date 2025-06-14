enum SocialMedia {
  case telegram
  case bluesky
  case instagram
  case facebook
  case lemmy
  case matrix
  case fosstodon
  case linkedin
  case email
  case codeberg

  var link: String {
    switch self {
    case .telegram:
      return L("telegram_url")
    case .codeberg:
      return "https://codeberg.org/comaps/comaps/"
    case .linkedin:
      return "https://www.linkedin.com/company/comaps-community/"
    case .email:
      return "ios@comaps.app"
    case .matrix:
      return "https://matrix.to/#/#comaps:matrix.org"
    case .fosstodon:
      return "https://floss.social/@CoMaps"
    case .facebook:
      return "https://www.facebook.com/CoMapsCommunity/"
    case .bluesky:
      return "https://bsky.app/profile/comaps.app"
    case .instagram:
      return L("instagram_url")
    case .lemmy:
      return "https://sopuli.xyz/c/CoMaps/"
    }
  }

  var image: UIImage {
    switch self {
    case .telegram:
      return UIImage(named: "ic_social_media_telegram")!
    case .codeberg:
      return UIImage(named: "ic_social_media_codeberg")!
    case .linkedin:
      return UIImage(named: "ic_social_media_linkedin")!
    case .email:
      return UIImage(named: "ic_social_media_mail")!
    case .matrix:
      return UIImage(named: "ic_social_media_matrix")!
    case .fosstodon:
      return UIImage(named: "ic_social_media_fosstodon")!
    case .facebook:
      return UIImage(named: "ic_social_media_facebook")!
    case .bluesky:
      return UIImage(named: "ic_social_media_bluesky")!
    case .instagram:
      return UIImage(named: "ic_social_media_instagram")!
    case .lemmy:
      return UIImage(named: "ic_social_media_lemmy")!
    }
  }
}
