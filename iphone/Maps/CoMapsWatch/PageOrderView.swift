import SwiftUI

/// The swipeable pages shown while a route is active, in user-chosen order.
enum WatchPage: String, CaseIterable, Identifiable {
  case map
  case guidance
  case elevation

  var id: String { rawValue }

  var title: LocalizedStringKey {
    switch self {
    case .map: return "Map"
    case .guidance: return "Guidance"
    case .elevation: return "Elevation profile"
    }
  }

  var symbolName: String {
    switch self {
    case .map: return "map"
    case .guidance: return "location.north.circle"
    case .elevation: return "chart.xyaxis.line"
    }
  }

  /// Parses a stored order, appending any pages missing from it so new
  /// pages appear even with an old stored value.
  static func order(from raw: String) -> [WatchPage] {
    var pages = raw.split(separator: ",").compactMap { WatchPage(rawValue: String($0)) }
    for page in WatchPage.allCases where !pages.contains(page) {
      pages.append(page)
    }
    return pages
  }
}

/// Settings screen: tapping a page moves it up one position; the first page
/// is what opens when a route becomes active.
struct PageOrderView: View {
  @AppStorage("watchPageOrder") private var rawOrder = ""

  var body: some View {
    let order = WatchPage.order(from: rawOrder)
    List {
      ForEach(Array(order.enumerated()), id: \.element) { index, page in
        Button {
          moveUp(page)
        } label: {
          HStack {
            Label {
              Text(page.title)
            } icon: {
              Image(systemName: page.symbolName)
                .foregroundStyle(.green)
            }
            Spacer()
            if index > 0 {
              Image(systemName: "arrow.up")
                .foregroundStyle(.secondary)
            }
          }
        }
        .disabled(index == 0)
      }
      Text("Tap a page to move it up. The first page shows when a route opens.")
        .font(.footnote)
        .foregroundStyle(.secondary)
    }
    .navigationTitle("Page order")
  }

  private func moveUp(_ page: WatchPage) {
    var order = WatchPage.order(from: rawOrder)
    guard let index = order.firstIndex(of: page), index > 0 else { return }
    order.swapAt(index, index - 1)
    rawOrder = order.map(\.rawValue).joined(separator: ",")
  }
}
