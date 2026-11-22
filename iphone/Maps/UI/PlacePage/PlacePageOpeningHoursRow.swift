import SwiftUI

struct PlacePageOpeningHoursRow: View {
  let openingHours: OpeningHours
  let checkDate: Date?

  @State private var isExpanded = false

  var body: some View {
    if let today = openingHours.days.first {
      HStack(alignment: .firstTextBaseline, spacing: PlacePageInfoRowLayout.spacing) {
        Image(systemName: "clock.fill")
          .foregroundStyle(.primary)
          .frame(width: PlacePageInfoRowLayout.iconWidth)
          .accessibilityHidden(true)
        if openingHours.days.count > 1 {
          DisclosureGroup(isExpanded: $isExpanded) {
            VStack(alignment: .leading, spacing: 8) {
              ForEach(Array(openingHours.days.dropFirst().enumerated()), id: \.offset) { _, day in
                PlacePageOpeningHoursDay(day: day)
              }
              if let checkDate {
                Text(String(format: L("hours_confirmed_time_ago"), checkDate.placePageRelativeString))
                  .font(.footnote)
                  .foregroundStyle(.secondary)
              }
            }
            .padding(.top, 8)
          } label: {
            PlacePageOpeningHoursDay(day: today, isClosedNow: openingHours.isClosedNow, title: L("editor_time_title"))
          }
          .disclosureGroupStyle(PlacePageDisclosureGroupStyle())
        } else {
          PlacePageOpeningHoursDay(day: today, isClosedNow: openingHours.isClosedNow, title: L("editor_time_title"))
        }
      }
      .padding(.horizontal)
      .padding(.vertical, 12)
    }
  }
}
