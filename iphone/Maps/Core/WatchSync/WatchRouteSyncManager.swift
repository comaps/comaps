import CoreLocation
import WatchConnectivity

/// Pushes the current route geometry to the paired watch so the watch app can
/// draw a glanceable "where am I on my route" view. The route is transferred
/// once per (re)build via the application context; no live streaming happens,
/// so there is no battery cost beyond the transfer itself.
@objc(MWMWatchRouteSync)
final class WatchRouteSyncManager: NSObject {
  @objc static let shared = WatchRouteSyncManager()

  /// Roughly one point per 5 m is far more than a watch screen can show.
  private let simplificationToleranceMeters: CLLocationDistance = 5
  /// Hard cap keeping the payload a few dozen KB at most.
  private let maxTransferredPoints = 2000
  /// Elevation chart resolution; the watch screen is under 300 px wide.
  private let maxAltitudeSamples: UInt = 512
  /// Corridor re-extraction is skipped when a fresher transfer is in flight.
  private let corridorMinimumInterval: TimeInterval = 30

  /// Turn updates stream only this often while navigating.
  private let turnPushInterval: TimeInterval = 3

  private var pendingContext: [String: Any]?
  private var lastCorridorPush: Date?
  private var lastTurnPush: Date?

  private var session: WCSession? {
    WCSession.isSupported() ? WCSession.default : nil
  }

  private override init() { super.init() }

  /// Call once at app launch so the watch can reach us even in background.
  @objc func activate() {
    guard let session else { return }
    session.delegate = self
    session.activate()
  }

  /// Call on the main thread whenever a route was (re)built.
  @objc func routeDidChange() {
    push(currentContext())
    transferCorridorIfNeeded(force: true)
  }

  /// Call when routing is closed.
  @objc func routeDidStop() {
    push(WatchRoutePayload.inactiveContext())
  }

  /// Call on the main thread on location updates while navigating; streams
  /// the next turn to a reachable watch, throttled and fire-and-forget.
  @objc func navigationDidUpdate() {
    guard let session, session.activationState == .activated, session.isReachable else { return }
    if let last = lastTurnPush, Date().timeIntervalSince(last) < turnPushInterval { return }
    guard let turnInfo = MWMWatchMapExtractor.currentTurnInfo() else { return }
    lastTurnPush = Date()
    session.sendMessage([WatchRoutePayload.turnInfoKey: turnInfo], replyHandler: nil)
  }

  // MARK: - Payload assembly (main thread: touches Framework via MWMRouter)

  private func currentContext() -> [String: Any] {
    guard let polyline = MWMRouter.routePolyline(), polyline.count >= 2 else {
      return WatchRoutePayload.inactiveContext()
    }
    let coordinates = Self.simplify(polyline.map { $0.coordinate },
                                    toleranceMeters: simplificationToleranceMeters,
                                    maxPoints: maxTransferredPoints)
    var ascent = 0.0
    var descent = 0.0
    let altitudeData = MWMWatchMapExtractor.routeAltitudes(withMaxPoints: maxAltitudeSamples,
                                                           ascent: &ascent, descent: &descent)
    let payload = WatchRoutePayload(coordinates: coordinates,
                                    routerType: Self.routerTypeString(MWMRouter.type()),
                                    destinationName: MWMRouter.finishPoint()?.title ?? "",
                                    altitudeData: altitudeData,
                                    ascent: ascent,
                                    descent: descent)
    return payload.contextDictionary()
  }

  /// Extracts the map corridor on a background queue and ships it as a file
  /// transfer; WatchConnectivity delivers it in background on its own pace.
  private func transferCorridorIfNeeded(force: Bool) {
    guard let session, session.activationState == .activated,
          session.isPaired, session.isWatchAppInstalled,
          MWMRouter.isRoutingActive()
    else { return }
    if !force, let last = lastCorridorPush, Date().timeIntervalSince(last) < corridorMinimumInterval {
      return
    }
    lastCorridorPush = Date()
    session.outstandingFileTransfers.forEach { $0.cancel() }
    MWMWatchMapExtractor.extractCorridor(withTimestamp: Date().timeIntervalSince1970) { data in
      guard let data, !data.isEmpty else { return }
      let url = FileManager.default.temporaryDirectory.appendingPathComponent("watch-route-context.bin")
      do {
        try data.write(to: url, options: .atomic)
      } catch {
        LOG(.warning, "WatchRouteSync: failed to stage corridor file: \(error)")
        return
      }
      session.transferFile(url, metadata: nil)
      LOG(.info, "WatchRouteSync: corridor transfer queued, \(data.count) bytes")
    }
  }

  private static func routerTypeString(_ type: MWMRouterType) -> String {
    switch type {
    case .vehicle: return "vehicle"
    case .pedestrian: return "pedestrian"
    case .bicycle: return "bicycle"
    case .publicTransport: return "transit"
    case .ruler: return "ruler"
    @unknown default: return ""
    }
  }

  private func push(_ context: [String: Any]) {
    guard let session else { return }
    guard session.activationState == .activated else {
      pendingContext = context
      activate()
      return
    }
    guard session.isPaired, session.isWatchAppInstalled else { return }
    do {
      try session.updateApplicationContext(context)
    } catch {
      LOG(.warning, "WatchRouteSync: failed to update application context: \(error)")
    }
  }

  // MARK: - Simplification

  /// Iterative Douglas-Peucker on a local equirectangular projection.
  static func simplify(_ coordinates: [CLLocationCoordinate2D],
                       toleranceMeters: CLLocationDistance,
                       maxPoints: Int) -> [CLLocationCoordinate2D] {
    guard coordinates.count > 2 else { return coordinates }

    let metersPerDegreeLat = 111_320.0
    let midLat = coordinates[coordinates.count / 2].latitude
    let metersPerDegreeLon = metersPerDegreeLat * cos(midLat * .pi / 180)
    let points = coordinates.map { CGPoint(x: $0.longitude * metersPerDegreeLon,
                                           y: $0.latitude * metersPerDegreeLat) }

    var tolerance = toleranceMeters
    while true {
      var keep = [Bool](repeating: false, count: points.count)
      keep[0] = true
      keep[points.count - 1] = true
      var stack = [(0, points.count - 1)]
      let toleranceSquared = tolerance * tolerance
      while let (first, last) = stack.popLast() {
        guard last > first + 1 else { continue }
        var maxDistanceSquared = 0.0
        var maxIndex = first
        for i in (first + 1)..<last {
          let d = squaredSegmentDistance(points[i], points[first], points[last])
          if d > maxDistanceSquared {
            maxDistanceSquared = d
            maxIndex = i
          }
        }
        if maxDistanceSquared > toleranceSquared {
          keep[maxIndex] = true
          stack.append((first, maxIndex))
          stack.append((maxIndex, last))
        }
      }
      let kept = keep.filter { $0 }.count
      if kept <= maxPoints || tolerance > 10_000 {
        return zip(coordinates, keep).compactMap { $1 ? $0 : nil }
      }
      tolerance *= 2
    }
  }

  private static func squaredSegmentDistance(_ p: CGPoint, _ a: CGPoint, _ b: CGPoint) -> Double {
    var x = a.x, y = a.y
    var dx = b.x - x, dy = b.y - y
    if dx != 0 || dy != 0 {
      let t = ((p.x - x) * dx + (p.y - y) * dy) / (dx * dx + dy * dy)
      if t > 1 {
        x = b.x
        y = b.y
      } else if t > 0 {
        x += dx * t
        y += dy * t
      }
    }
    dx = p.x - x
    dy = p.y - y
    return dx * dx + dy * dy
  }
}

// MARK: - WCSessionDelegate

extension WatchRouteSyncManager: WCSessionDelegate {
  func session(_ session: WCSession, activationDidCompleteWith activationState: WCSessionActivationState, error: Error?) {
    guard activationState == .activated, let context = pendingContext else { return }
    pendingContext = nil
    DispatchQueue.main.async { self.push(context) }
  }

  func sessionDidBecomeInactive(_ session: WCSession) {}

  func sessionDidDeactivate(_ session: WCSession) {
    // Called after switching to a new watch; reactivate for the new one.
    session.activate()
  }

  func session(_ session: WCSession,
               didReceiveMessage message: [String: Any],
               replyHandler: @escaping ([String: Any]) -> Void) {
    // The watch asks for the current route when it launches, in case the last
    // application context predates this navigation session.
    if message[WatchRoutePayload.refreshRequestKey] != nil {
      DispatchQueue.main.async {
        replyHandler(MWMRouter.isRoutingActive() ? self.currentContext() : WatchRoutePayload.inactiveContext())
        // The watch may have lost the corridor (fresh install, cleared cache).
        self.transferCorridorIfNeeded(force: false)
      }
      return
    }
    // The watch shows bookmarks as tappable destinations when idle.
    if message[WatchRoutePayload.destinationsRequestKey] != nil {
      DispatchQueue.main.async {
        replyHandler([WatchRoutePayload.destinationsKey: MWMWatchMapExtractor.watchDestinations(withLimit: 60)])
      }
      return
    }
    // "Take me there" tapped on the watch: build the route on the phone; the
    // result reaches the watch through the normal route sync.
    if let request = message[WatchRoutePayload.routeRequestKey] as? [String: Any],
       let destination = WatchDestination(dictionary: request) {
      DispatchQueue.main.async {
        MWMWatchMapExtractor.buildRoute(toLatitude: destination.coordinate.latitude,
                                        longitude: destination.coordinate.longitude,
                                        name: destination.name)
        replyHandler(["ok": true])
      }
      return
    }
    // "End route" tapped on the watch.
    if message[WatchRoutePayload.stopRouteKey] != nil {
      DispatchQueue.main.async {
        if MWMRouter.isRoutingActive() {
          MWMRouter.stopRouting()
        }
        replyHandler(WatchRoutePayload.inactiveContext())
      }
      return
    }
    // "+" tapped on the watch: save its location as a bookmark and reply
    // with the refreshed destination list.
    if let request = message[WatchRoutePayload.addBookmarkKey] as? [String: Any],
       let destination = WatchDestination(dictionary: request) {
      DispatchQueue.main.async {
        MWMWatchMapExtractor.addBookmark(atLatitude: destination.coordinate.latitude,
                                         longitude: destination.coordinate.longitude,
                                         name: destination.name)
        replyHandler([WatchRoutePayload.destinationsKey: MWMWatchMapExtractor.watchDestinations(withLimit: 60)])
      }
      return
    }
    replyHandler([:])
  }
}
