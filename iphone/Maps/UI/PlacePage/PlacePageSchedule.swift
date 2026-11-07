import SwiftUI

struct PlacePageSchedule {
  let state: String
  let color: Color
  let details: String?

  init?(_ schedule: PlacePageDataSchedule) {
    let now = time_t(Date().timeIntervalSince1970)
    switch schedule.state {
    case .unknown:
      return nil
    case .allDay:
      state = L("twentyfour_seven")
      color = Color.BaseColors.green
      details = nil
    case .open:
      let minutesUntilClosed = (schedule.nextTimeClosed - now) / 60
      let interval = Self.intervalString(minutes: minutesUntilClosed)
      let time = Self.timeString(schedule.nextTimeClosed)
      if minutesUntilClosed < 60 {
        state = String(format: L("closes_in"), interval)
        color = Color.BaseColors.yellow
        details = time
      } else {
        state = L("editor_time_open")
        color = Color.BaseColors.green
        if minutesUntilClosed < 3 * 60 {
          details = String(format: L("closes_in"), interval) + " • " + time
        } else if minutesUntilClosed < 24 * 60 {
          details = String(format: L("closes_at"), time)
        } else {
          details = nil
        }
      }
    case .closed:
      let minutesUntilOpen = (schedule.nextTimeOpen - now) / 60
      let interval = Self.intervalString(minutes: minutesUntilOpen)
      let time = Self.timeString(schedule.nextTimeOpen)
      let nextTimeOpen = Date(timeIntervalSince1970: TimeInterval(schedule.nextTimeOpen))
      if minutesUntilOpen < 15 {
        state = String(format: L("opens_in"), interval)
        color = Color.BaseColors.yellow
        details = time
      } else {
        state = L("closed_now")
        color = Color.BaseColors.red
        if minutesUntilOpen < 3 * 60 {
          details = String(format: L("opens_in"), interval) + " • " + time
        } else if Calendar.current.isDateInToday(nextTimeOpen) {
          details = String(format: L("opens_at"), time)
        } else if minutesUntilOpen < 24 * 60 {
          details = String(format: L("opens_tomorrow_at"), time)
        } else if minutesUntilOpen < 7 * 24 * 60 {
          let weekday = DateTimeFormatter.dateString(from: nextTimeOpen, format: "EEEE")
          details = String(format: L("opens_dayoftheweek_at"), weekday, time)
        } else {
          details = nil
        }
      }
    @unknown default:
      return nil
    }
  }

  var text: Text {
    var text = AttributedString(state)
    text.foregroundColor = color
    text.font = .subheadline.weight(.semibold)
    if let details {
      var detailsText = AttributedString(" • " + details)
      detailsText.foregroundColor = .secondary
      text += detailsText
    }
    return Text(text)
  }

  private static func timeString(_ time: time_t) -> String {
    Date(timeIntervalSince1970: TimeInterval(time)).formatted(date: .omitted, time: .shortened)
  }

  private static func intervalString(minutes: Int) -> String {
    Duration.seconds(minutes * 60).formatted(.units(allowed: [.hours, .minutes], width: .wide))
  }
}
