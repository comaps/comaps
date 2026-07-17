import Combine
import CoreLocation
import Foundation
import WatchConnectivity

/// Receives the route pushed by the phone and caches it on disk, so the last
/// route survives app relaunches and works with the phone out of reach.
final class RouteStore: NSObject, ObservableObject {
  @Published private(set) var route: WatchRoutePayload?
  @Published private(set) var mapContext: WatchMapContext?
  @Published private(set) var destinations: [WatchDestination] = []
  /// Next turn streamed by the phone during navigation; nil when idle.
  @Published private(set) var turnInfo: WatchTurnInfo?
  /// True between tapping a destination and the requested route arriving.
  @Published private(set) var isRequestingRoute = false
  @Published private(set) var isPhoneReachable = false

  private var cacheDirectory: URL {
    FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
  }
  private var cacheURL: URL { cacheDirectory.appendingPathComponent("current-route.plist") }
  private var contextCacheURL: URL { cacheDirectory.appendingPathComponent("route-context.bin") }
  private var destinationsCacheURL: URL { cacheDirectory.appendingPathComponent("destinations.plist") }

  override init() {
    super.init()
    // Application Support does not exist until someone creates it.
    try? FileManager.default.createDirectory(at: cacheDirectory, withIntermediateDirectories: true)
    loadCache()
    guard WCSession.isSupported() else { return }
    WCSession.default.delegate = self
    WCSession.default.activate()
  }

  /// Asks the phone for the current route; useful when the last application
  /// context predates the navigation session running on the phone right now.
  func requestRefresh() {
    guard let session = reachableSession else { return }
    session.sendMessage([WatchRoutePayload.refreshRequestKey: true]) { [weak self] reply in
      guard !reply.isEmpty else { return }
      DispatchQueue.main.async { self?.apply(reply) }
    } errorHandler: { _ in }
  }

  /// Asks the phone for its bookmarks to offer as tappable destinations.
  func requestDestinations() {
    guard let session = reachableSession else { return }
    session.sendMessage([WatchRoutePayload.destinationsRequestKey: true]) { [weak self] reply in
      self?.applyDestinationsReply(reply)
    } errorHandler: { _ in }
  }

  /// Saves the given spot (the watch's current location) as a bookmark on the
  /// phone; the reply carries the refreshed destination list.
  func addBookmark(at coordinate: CLLocationCoordinate2D) {
    guard let session = reachableSession else { return }
    let name = String(format: String(localized: "Pin %@"),
                      Date().formatted(date: .omitted, time: .shortened))
    let destination: [String: Any] = [WatchDestination.nameKey: name,
                                      WatchDestination.latitudeKey: coordinate.latitude,
                                      WatchDestination.longitudeKey: coordinate.longitude]
    session.sendMessage([WatchRoutePayload.addBookmarkKey: destination]) { [weak self] reply in
      self?.applyDestinationsReply(reply)
    } errorHandler: { _ in }
  }

  private func applyDestinationsReply(_ reply: [String: Any]) {
    guard let dictionaries = reply[WatchRoutePayload.destinationsKey] as? [[String: Any]] else { return }
    let parsed = dictionaries.compactMap(WatchDestination.init)
    // Home first, then the phone's bookmark order.
    let sorted = parsed.filter(\.isHome) + parsed.filter { !$0.isHome }
    DispatchQueue.main.async { [weak self] in
      self?.destinations = sorted
      self?.persistDestinations(dictionaries)
    }
  }

  /// Asks the phone to build a route to the destination; the route arrives
  /// through the normal sync once built.
  func requestRoute(to destination: WatchDestination) {
    guard let session = reachableSession else { return }
    isRequestingRoute = true
    session.sendMessage([WatchRoutePayload.routeRequestKey: destination.dictionary]) { _ in
    } errorHandler: { [weak self] _ in
      DispatchQueue.main.async { self?.isRequestingRoute = false }
    }
    // Routing can fail silently on the phone (no location, missing maps);
    // fall back to the destination list instead of spinning forever.
    DispatchQueue.main.asyncAfter(deadline: .now() + 30) { [weak self] in
      self?.isRequestingRoute = false
    }
  }

  /// Ends the route on the phone; the reply is the inactive context, which
  /// clears the watch state through the normal path.
  func stopRoute() {
    guard let session = reachableSession else { return }
    session.sendMessage([WatchRoutePayload.stopRouteKey: true]) { [weak self] reply in
      guard !reply.isEmpty else { return }
      DispatchQueue.main.async { self?.apply(reply) }
    } errorHandler: { _ in }
  }

  private var reachableSession: WCSession? {
    let session = WCSession.default
    guard WCSession.isSupported(), session.activationState == .activated, session.isReachable else { return nil }
    return session
  }

  private func apply(_ context: [String: Any]) {
    guard !context.isEmpty else { return }
    route = WatchRoutePayload(contextDictionary: context)
    if route != nil {
      isRequestingRoute = false
    }
    persist(context)
    if route == nil {
      mapContext = nil
      try? FileManager.default.removeItem(at: contextCacheURL)
    }
  }

  // MARK: - Disk cache

  private func persist(_ context: [String: Any]) {
    guard let data = try? PropertyListSerialization.data(fromPropertyList: context, format: .binary, options: 0)
    else { return }
    try? data.write(to: cacheURL, options: .atomic)
  }

  private func loadCache() {
    if let data = try? Data(contentsOf: cacheURL),
       let context = (try? PropertyListSerialization.propertyList(from: data, options: PropertyListSerialization.ReadOptions(), format: nil)) as? [String: Any] {
      route = WatchRoutePayload(contextDictionary: context)
    }
    if route != nil, let data = try? Data(contentsOf: contextCacheURL) {
      mapContext = WatchMapContext(data: data)
    }
    if let data = try? Data(contentsOf: destinationsCacheURL),
       let dictionaries = (try? PropertyListSerialization.propertyList(from: data, options: PropertyListSerialization.ReadOptions(), format: nil)) as? [[String: Any]] {
      let parsed = dictionaries.compactMap(WatchDestination.init)
      destinations = parsed.filter(\.isHome) + parsed.filter { !$0.isHome }
    }
  }

  private func persistDestinations(_ dictionaries: [[String: Any]]) {
    guard let data = try? PropertyListSerialization.data(fromPropertyList: dictionaries, format: .binary, options: 0)
    else { return }
    try? data.write(to: destinationsCacheURL, options: .atomic)
  }
}

// MARK: - WCSessionDelegate

extension RouteStore: WCSessionDelegate {
  func session(_ session: WCSession, activationDidCompleteWith activationState: WCSessionActivationState, error: Error?) {
    guard activationState == .activated else { return }
    DispatchQueue.main.async {
      self.isPhoneReachable = session.isReachable
      // A context delivered while the app was not running is available here.
      let received = session.receivedApplicationContext
      if !received.isEmpty {
        self.apply(received)
      }
      self.requestRefresh()
    }
  }

  func session(_ session: WCSession, didReceiveApplicationContext applicationContext: [String: Any]) {
    DispatchQueue.main.async { self.apply(applicationContext) }
  }

  func sessionReachabilityDidChange(_ session: WCSession) {
    DispatchQueue.main.async { self.isPhoneReachable = session.isReachable }
  }

  /// Fire-and-forget messages from the phone (turn streaming).
  func session(_ session: WCSession, didReceiveMessage message: [String: Any]) {
    guard let dictionary = message[WatchRoutePayload.turnInfoKey] as? [String: Any],
          let turn = WatchTurnInfo(dictionary: dictionary)
    else { return }
    DispatchQueue.main.async { self.turnInfo = turn }
  }

  /// Corridor geometry arrives as a file transfer; the source file is deleted
  /// when this callback returns, so read it synchronously.
  func session(_ session: WCSession, didReceive file: WCSessionFile) {
    guard let data = try? Data(contentsOf: file.fileURL),
          let context = WatchMapContext(data: data)
    else { return }
    try? data.write(to: contextCacheURL, options: .atomic)
    DispatchQueue.main.async { self.mapContext = context }
  }
}
