import CoreLocation
import SwiftUI
import WatchKit

@main
struct CoMapsWatchApp: App {
  @StateObject private var routeStore = RouteStore()
  @StateObject private var locationProvider = WatchLocationProvider()
  @StateObject private var workout = WorkoutSessionManager()

  var body: some Scene {
    WindowGroup {
      RootView()
        .environmentObject(routeStore)
        .environmentObject(locationProvider)
        .environmentObject(workout)
    }
  }
}

/// With a route: swipeable pages — map glance, next turn, compass guidance,
/// and (when the route has altitudes) the elevation profile. Without one:
/// the phone's bookmarks as tappable destinations. Location updates run only
/// while the app is on screen, unless a workout session keeps it alive.
private struct RootView: View {
  @EnvironmentObject private var routeStore: RouteStore
  @EnvironmentObject private var locationProvider: WatchLocationProvider
  @EnvironmentObject private var workout: WorkoutSessionManager
  @Environment(\.scenePhase) private var scenePhase

  @State private var wasOffRoute = false
  @State private var lastOffRouteAlert: Date?
  @AppStorage("watchPageOrder") private var rawPageOrder = ""

  var body: some View {
    Group {
      if let route = routeStore.route {
        TabView {
          ForEach(WatchPage.order(from: rawPageOrder)) { page in
            switch page {
            case .map:
              RouteGlanceView()
            case .guidance:
              CompassGuidanceView()
            case .elevation:
              if route.altitudeSamples.count > 1 {
                ElevationProfileView()
              }
            }
          }
        }
      } else if routeStore.isRequestingRoute {
        ProgressView("Building route…")
      } else {
        DestinationsView()
      }
    }
    .onChange(of: scenePhase) { phase in
      if phase == .active {
        locationProvider.start()
        routeStore.requestRefresh()
        if routeStore.route == nil {
          routeStore.requestDestinations()
        }
      } else if !workout.isRunning {
        locationProvider.stop()
      }
    }
    .onChange(of: routeStore.route == nil) { routeCleared in
      if routeCleared {
        workout.stop()
      }
    }
    .onChange(of: locationProvider.location) { location in
      alertIfOffRoute(location)
    }
  }

  /// One wrist tap when drifting off the route (repeated at most per minute).
  private func alertIfOffRoute(_ location: CLLocation?) {
    guard let location, let route = routeStore.route, route.coordinates.count >= 2 else { return }
    let projection = RouteProjection(origin: route.coordinates[route.coordinates.count / 2])
    let points = route.coordinates.map(projection.point)
    let progress = RouteMath.progress(points: points,
                                      cumulativeLengths: RouteMath.cumulativeLengths(points),
                                      user: projection.point(location.coordinate))
    let offRoute = progress.distanceFromRoute > 100
    if offRoute, !wasOffRoute,
       lastOffRouteAlert.map({ Date().timeIntervalSince($0) > 60 }) ?? true {
      WKInterfaceDevice.current().play(.notification)
      lastOffRouteAlert = Date()
    }
    wasOffRoute = offRoute
  }
}
