import SwiftUI

struct PlacePageInfoSection: View {
  let content: PlacePageContent
  let info: PlacePageInfoData

  @AppStorage("PlacePageInfoViewController_coordFormatIdKey") private var coordinatesFormat = 0

  var body: some View {
    VStack(alignment: .leading, spacing: 0) {
      PlacePageDividedStack(inset: PlacePageInfoRowLayout.textInset) {
        openingHours
        row(info.cuisine, title: L("cuisine"), icon: .system("fork.knife"))
        row(info.organic, icon: .system("leaf.fill"))
        row(info.secondHand, icon: .system("arrow.trianglehead.2.clockwise"))
        ForEach(info.phones, id: \.phone) { phone in
          let canCall = phone.url.map { UIApplication.shared.canOpenURL($0) } ?? false
          PlacePageInfoRow(item: PlacePageInfoItem(icon: .system("phone.fill"), text: phone.phone, title: L("phone"),
                                                   isLink: canCall, copyText: phone.phone) { [content] in
            content.call(phone)
          }, copy: content.copy)
        }
        row(info.ppOperator, icon: .system("person.text.rectangle"))
        row(info.network, icon: .system("point.3.filled.connected.trianglepath.dotted"))
        row(info.population, icon: .system("person.2.fill"))
        link(info.website, text: info.website.map(Self.strippedURL), title: L("website"), icon: .system("globe"),
             action: MWMPlacePageManagerHelper.openWebsite)
        link(info.websiteMenu, text: L("website_menu"), icon: .system("menucard"),
             action: MWMPlacePageManagerHelper.openWebsiteMenu)
        link(info.wikipedia, text: L("read_in_wikipedia"), icon: .brand(.Brands.wikipedia),
             action: MWMPlacePageManagerHelper.openWikipedia)
        link(info.wikimediaCommons, text: L("wikimedia_commons"), icon: .brand(.Brands.wikimediaCommons),
             action: MWMPlacePageManagerHelper.openWikimediaCommons)
        row(info.wifiAvailable, title: L("category_wifi"), icon: .system("wifi"))
        row(info.atm, icon: .system("creditcard"))
        row(info.level, title: L("level"), icon: .symbol("level"))
        row(info.capacity, icon: .system("viewfinder"))
        row(info.capacityCharging, icon: .symbol("powerplug.portrait.viewfinder"))
        row(info.capacityDisabled, icon: .symbol("figure.roll.viewfinder"))
        row(info.rooms, icon: .system("bed.double.fill"))
        row(info.charge, title: L("fee"), icon: .system("tag.fill"))
        row(info.wheelchair, icon: .system("figure.roll"))
        row(info.selfService, icon: .symbol("service.slash"))
        row(info.outdoorSeating, icon: .symbol("outdoorseating"))
        row(info.driveThrough, icon: .symbol("drivethrough"))
        link(info.email, title: L("email"), icon: .system("envelope.fill"), action: MWMPlacePageManagerHelper.openEmail)
        link(info.fediverse, title: L("fediverse"), icon: .brand(.Brands.mastodon),
             action: MWMPlacePageManagerHelper.openFediverse)
        link(info.facebook, title: L("facebook"), icon: .brand(.Brands.facebook),
             action: MWMPlacePageManagerHelper.openFacebook)
        link(info.instagram, title: L("instagram"), icon: .brand(.Brands.instagram),
             action: MWMPlacePageManagerHelper.openInstagram)
        link(info.twitter, title: L("twitter"), icon: .brand(.Brands.X),
             action: MWMPlacePageManagerHelper.openTwitter)
        link(info.vk, title: L("vk"), icon: .brand(.Brands.VK),
             action: MWMPlacePageManagerHelper.openVk)
        link(info.line, title: L("line"), icon: .brand(.Brands.line),
             action: MWMPlacePageManagerHelper.openLine)
        link(info.bluesky, title: L("bluesky"), icon: .brand(.Brands.bluesky),
             action: MWMPlacePageManagerHelper.openBluesky)
        link(info.panoramax, text: L("panoramax_picture"), icon: .brand(.Brands.panoramax),
             action: MWMPlacePageManagerHelper.openPanoramax)
        if let address = info.address {
          PlacePageInfoRow(item: PlacePageInfoItem(icon: .symbol("address"), text: address, title: L("address"),
                                                   copyText: address),
                           copy: content.copy)
        }
        if let formats = info.coordFormats as? [String], !formats.isEmpty {
          coordinatesRow(formats)
        }
        openInApp
      }
      if let checkDate = info.checkDate {
        Divider()
        Text(String(format: L("existence_confirmed_time_ago"), checkDate.placePageRelativeString))
          .font(.footnote)
          .foregroundStyle(.secondary)
          .padding(.horizontal)
          .padding(.vertical, 12)
      }
    }
  }

  @ViewBuilder
  private func row(_ text: String?, title: String? = nil, icon: PlacePageInfoItem.Icon) -> some View {
    if let text {
      PlacePageInfoRow(item: PlacePageInfoItem(icon: icon, text: text, title: title), copy: content.copy)
    }
  }

  @ViewBuilder
  private func link(_ value: String?, text: String? = nil, title: String? = nil, icon: PlacePageInfoItem.Icon,
                    action: @escaping @MainActor (PlacePageData) -> Void) -> some View {
    if let value {
      let data = content.data
      PlacePageInfoRow(item: PlacePageInfoItem(icon: icon, text: text ?? value, title: title, isLink: true,
                                               copyText: value) {
        action(data)
      }, copy: content.copy)
    }
  }

  @ViewBuilder
  private var openingHours: some View {
    if let openingHours = info.openingHours {
      PlacePageOpeningHoursRow(openingHours: openingHours, checkDate: info.checkDateOpeningHours)
    } else if let openingHours = info.openingHoursString {
      let clock = PlacePageInfoItem.Icon.system("clock.fill")
      let title = L("editor_time_title")
      switch openingHours.lowercased() {
      case "off", "closed":
        PlacePageInfoRow(item: PlacePageInfoItem(icon: clock, text: L("day_off"), title: title, color: .red),
                         copy: content.copy)
      case "unknown":
        PlacePageInfoRow(item: PlacePageInfoItem(icon: clock, text: L("oh_unknown"), title: title), copy: content.copy)
      default:
        PlacePageInfoRow(item: PlacePageInfoItem(icon: clock, text: openingHours, title: title), copy: content.copy)
      }
    }
  }

  private func coordinatesRow(_ formats: [String]) -> some View {
    let index = formats.indices.contains(coordinatesFormat) ? coordinatesFormat : 0
    let item = PlacePageInfoItem(icon: .system("dot.scope"), text: formats[index], title: L("coordinates"),
                                 isLink: true, copyText: formats[index]) {
      coordinatesFormat = (index + 1) % formats.count
    }
    return PlacePageInfoRow(item: item, copy: content.copy) {
      Menu {
        ForEach(Array(formats.enumerated()), id: \.offset) { formatIndex, format in
          Button(format) {
            coordinatesFormat = formatIndex
            content.copy(format)
          }
        }
      } label: {
        Image(systemName: "chevron.up.chevron.down")
          .frame(minWidth: 44, minHeight: 44)
          .contentShape(.rect)
      }
      .accessibilityLabel(L("copy_to_clipboard"))
    }
  }

  @ViewBuilder
  private var openInApp: some View {
    let apps = OpenInApplication.availableApps
    if !apps.isEmpty {
      Menu {
        ForEach(apps, id: \.self) { app in
          Button(app.name) {
            content.open(in: app)
          }
        }
      } label: {
        PlacePageInfoRowLabel(item: PlacePageInfoItem(icon: .system("arrow.up.forward.app"), text: L("open_in_app"), isLink: true))
      }
      .buttonStyle(PlacePageInfoRowButtonStyle())
    }
  }

  private static func strippedURL(_ url: String) -> String {
    var url = Substring(url)
    for scheme in ["https://", "http://"] where url.hasPrefix(scheme) {
      url = url.dropFirst(scheme.count)
      break
    }
    if url.hasSuffix("/") {
      url = url.dropLast()
    }
    return String(url)
  }
}

extension Date {
  var placePageRelativeString: String {
    if Calendar.current.isDateInToday(self) || Calendar.current.isDateInYesterday(self) {
      let formatter = DateFormatter()
      formatter.dateStyle = .medium
      formatter.timeStyle = .none
      formatter.doesRelativeDateFormatting = true
      let string = formatter.string(from: self)
      return string.prefix(1).lowercased() + string.dropFirst()
    }
    let formatter = RelativeDateTimeFormatter()
    formatter.unitsStyle = .spellOut
    return formatter.localizedString(for: self, relativeTo: Date())
  }
}
