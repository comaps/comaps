import Foundation

/// API client for location sharing server
class LocationSharingApiClient {

  private let serverBaseUrl: String
  private let sessionId: String
  private let session: URLSession

  private static let connectTimeout: TimeInterval = 10
  private static let requestTimeout: TimeInterval = 10

  init(serverBaseUrl: String, sessionId: String) {
    self.serverBaseUrl = serverBaseUrl.hasSuffix("/") ? serverBaseUrl : serverBaseUrl + "/"
    self.sessionId = sessionId

    let config = URLSessionConfiguration.default
    config.timeoutIntervalForRequest = Self.requestTimeout
    config.timeoutIntervalForResource = Self.connectTimeout
    self.session = URLSession(configuration: config)
  }

  // MARK: - API Methods

  /// Create session on server
  func createSession(completion: ((Bool, String?) -> Void)? = nil) {
    let urlString = "\(serverBaseUrl)api/v1/session"
    guard let url = URL(string: urlString) else {
      completion?(false, "Invalid URL")
      return
    }

    var request = URLRequest(url: url)
    request.httpMethod = "POST"
    request.setValue("application/json", forHTTPHeaderField: "Content-Type")

    let body: [String: Any] = ["sessionId": sessionId]
    guard let jsonData = try? JSONSerialization.data(withJSONObject: body) else {
      completion?(false, "Failed to serialize JSON")
      return
    }

    request.httpBody = jsonData

    let task = session.dataTask(with: request) { data, response, error in
      if let error = error {
        NSLog("Create session error: \(error.localizedDescription)")
        completion?(false, error.localizedDescription)
        return
      }

      guard let httpResponse = response as? HTTPURLResponse else {
        completion?(false, "Invalid response")
        return
      }

      if (200..<300).contains(httpResponse.statusCode) {
        NSLog("Session created: \(self.sessionId)")
        completion?(true, nil)
      } else {
        completion?(false, "Server error: \(httpResponse.statusCode)")
      }
    }

    task.resume()
  }

  /// Update location on server with encrypted payload
  func updateLocation(encryptedPayload: String, completion: ((Bool, String?) -> Void)? = nil) {
    let urlString = "\(serverBaseUrl)api/v1/location/\(sessionId)"
    guard let url = URL(string: urlString) else {
      completion?(false, "Invalid URL")
      return
    }

    var request = URLRequest(url: url)
    request.httpMethod = "POST"
    request.setValue("application/json", forHTTPHeaderField: "Content-Type")
    request.httpBody = encryptedPayload.data(using: .utf8)

    let task = session.dataTask(with: request) { data, response, error in
      if let error = error {
        NSLog("Update location error: \(error.localizedDescription)")
        completion?(false, error.localizedDescription)
        return
      }

      guard let httpResponse = response as? HTTPURLResponse else {
        completion?(false, "Invalid response")
        return
      }

      if (200..<300).contains(httpResponse.statusCode) {
        completion?(true, nil)
      } else {
        completion?(false, "Server error: \(httpResponse.statusCode)")
      }
    }

    task.resume()
  }

  /// End session on server
  func endSession(completion: ((Bool, String?) -> Void)? = nil) {
    let urlString = "\(serverBaseUrl)api/v1/session/\(sessionId)"
    guard let url = URL(string: urlString) else {
      completion?(false, "Invalid URL")
      return
    }

    var request = URLRequest(url: url)
    request.httpMethod = "DELETE"

    let task = session.dataTask(with: request) { data, response, error in
      if let error = error {
        NSLog("End session error: \(error.localizedDescription)")
        completion?(false, error.localizedDescription)
        return
      }

      guard let httpResponse = response as? HTTPURLResponse else {
        completion?(false, "Invalid response")
        return
      }

      if (200..<300).contains(httpResponse.statusCode) {
        NSLog("Session ended: \(self.sessionId)")
        completion?(true, nil)
      } else {
        completion?(false, "Server error: \(httpResponse.statusCode)")
      }
    }

    task.resume()
  }
}
