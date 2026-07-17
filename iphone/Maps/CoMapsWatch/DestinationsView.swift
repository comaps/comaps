import SwiftUI
import WatchKit

/// Start screen when no route is active: a Home chip, a "+" chip that saves
/// the current location as a bookmark, and the phone's bookmarks as tappable
/// destinations ("take me there"). Tapping one asks the phone to build a
/// route; the app switches to the glance views once the route arrives.
struct DestinationsView: View {
  @EnvironmentObject private var routeStore: RouteStore
  @EnvironmentObject private var locationProvider: WatchLocationProvider

  private var home: WatchDestination? { routeStore.destinations.first(where: \.isHome) }
  private var places: [WatchDestination] { routeStore.destinations.filter { !$0.isHome } }

  var body: some View {
    NavigationStack {
      List {
        Section {
          HStack(spacing: 8) {
            homeChip
            addChip
          }
          .listRowBackground(Color.clear)
          .listRowInsets(EdgeInsets())
          if home == nil {
            Text("Name a bookmark “Home” to pin it here.")
              .font(.system(size: 11))
              .foregroundStyle(.secondary)
              .listRowBackground(Color.clear)
          }
        }
        ForEach(places) { destination in
          Button {
            routeStore.requestRoute(to: destination)
          } label: {
            Label {
              Text(destination.name.isEmpty ? "Unnamed place" : destination.name)
                .lineLimit(1)
            } icon: {
              Image(systemName: "star.circle.fill")
                .foregroundStyle(.green)
            }
          }
        }
        if routeStore.destinations.isEmpty {
          Text("Bookmark places in CoMaps on your iPhone to route to them from here.")
            .font(.footnote)
            .foregroundStyle(.secondary)
        }
        Button {
          routeStore.requestDestinations()
          routeStore.requestRefresh()
        } label: {
          Label("Refresh", systemImage: "arrow.clockwise")
            .font(.footnote)
        }
        NavigationLink {
          PageOrderView()
        } label: {
          Label("Page order", systemImage: "arrow.up.arrow.down")
            .font(.footnote)
        }
        if !routeStore.isPhoneReachable {
          Text("iPhone not reachable — routing needs CoMaps on the phone.")
            .font(.footnote)
            .foregroundStyle(.secondary)
        }
      }
      .navigationTitle("CoMaps")
    }
    .onAppear { routeStore.requestDestinations() }
  }

  /// Routes home in one tap, like the mockup's house chip.
  private var homeChip: some View {
    Button {
      if let home { routeStore.requestRoute(to: home) }
    } label: {
      Image(systemName: "house.fill")
        .font(.title3)
        .foregroundStyle(home != nil ? .green : .secondary)
        .frame(maxWidth: .infinity, minHeight: 44)
        .background(RoundedRectangle(cornerRadius: 12).fill(.white.opacity(0.12)))
    }
    .buttonStyle(.plain)
    .disabled(home == nil)
  }

  /// Saves the current location as a bookmark ("where did I park?").
  private var addChip: some View {
    Button {
      guard let coordinate = locationProvider.location?.coordinate else { return }
      routeStore.addBookmark(at: coordinate)
      WKInterfaceDevice.current().play(.success)
    } label: {
      Image(systemName: "plus")
        .font(.title3)
        .foregroundStyle(.secondary)
        .frame(maxWidth: .infinity, minHeight: 44)
        .background(
          RoundedRectangle(cornerRadius: 12)
            .strokeBorder(style: StrokeStyle(lineWidth: 1, dash: [4, 3]))
            .foregroundStyle(.gray)
        )
    }
    .buttonStyle(.plain)
    .disabled(locationProvider.location == nil)
  }
}
