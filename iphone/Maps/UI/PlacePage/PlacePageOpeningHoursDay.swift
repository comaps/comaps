import SwiftUI

struct PlacePageOpeningHoursDay: View {
  let day: WorkingDay
  var isClosedNow = false
  var title: String?

  var body: some View {
    VStack(alignment: .leading, spacing: 2) {
      HStack(alignment: .firstTextBaseline) {
        Text(day.workingDays)
        Spacer()
        Text(day.workingTimes)
          .multilineTextAlignment(.trailing)
      }
      if let breaks = day.breaks, !breaks.isEmpty {
        Text(breaks)
          .font(.subheadline)
          .foregroundStyle(.secondary)
      }
      if isClosedNow {
        Text(L("closed_now"))
          .font(.subheadline)
          .foregroundStyle(.red)
      }
    }
    .foregroundStyle(.primary)
    .accessibilityElement(children: .ignore)
    .accessibilityLabel(title ?? spokenText)
    .accessibilityValue(title == nil ? "" : spokenText)
  }

  private var spokenText: String {
    [day.workingDays, day.workingTimes, day.breaks, isClosedNow ? L("closed_now") : nil]
      .compactMap { $0 }
      .filter { !$0.isEmpty }
      .joined(separator: ", ")
  }
}
