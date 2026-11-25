import CryptoKit
import Network
import OSMEditor

/// The OpenStreetMap profile
@objc class Profile: NSObject {
    // MARK: Properties
    
    /// Key for storing the current authorization token in the user defaults
    static private let userDefaultsKeyAuthorizationToken = "OSMAuthToken"
    
    
    /// Key for storing the name of the OpenStreetMap profile in the user defaults
    static private let userDefaultsKeyName = "UDOsmUserName"
    
    
    /// Key for storing the number of edits of the OpenStreetMap profile in the user defaults
    static private let userDefaultsKeyNumberOfEdits = "OSMUserChangesetsCount"
    
    
    /// Key for storing iff the OpenStreetMap profile needs to be reauthorized in the user defaults
    static private let userDefaultsNeedsReauthorization = "AuthNeedCheck"
    
    
    static private let userDefaultsKeyOAuth2State = "OSMOAuth2State"
    
    
    static private let userDefaultsKeyOAuth2CodeVerifier = "OSMOAuth2CodeVerifier"
    
    
    /// The URL for registering an OpenStreetMap profile
    static var registrationUrl: URL {
        return URL(string: String(describing: osm.OsmOAuth.ServerAuth().GetRegistrationURL()))!
    }
    
    
    /// The URL for letting an OpenStreetMap profile authorize this app
    static var authorizationUrl: URL {
        let userDefaults = UserDefaults.standard
        let state: String
        let codeVerifier: String
        if let storedState = userDefaults.string(forKey: userDefaultsKeyOAuth2State),
           let storedCodeVerifier = userDefaults.string(forKey: userDefaultsKeyOAuth2CodeVerifier) {
            state = storedState
            codeVerifier = storedCodeVerifier
        } else {
            state = randomUrlSafeString()
            codeVerifier = randomUrlSafeString()
            userDefaults.set(state, forKey: userDefaultsKeyOAuth2State)
            userDefaults.set(codeVerifier, forKey: userDefaultsKeyOAuth2CodeVerifier)
        }
        let codeChallenge = base64UrlEncoded(Data(SHA256.hash(data: Data(codeVerifier.utf8))))
        return URL(string: String(describing: osm.OsmOAuth.ServerAuth().BuildOAuth2Url(std.string(state), std.string(codeChallenge))))!
    }
    
    
    /// The optional current authorization token (can be empty)
    @objc static var authorizationToken: String? {
        if let authorizationToken = UserDefaults.standard.string(forKey: userDefaultsKeyAuthorizationToken), !authorizationToken.isEmpty {
            return authorizationToken
        }
        
        return nil
    }
    
    
    /// If the OpenStreetMap profile needs to be reauthorized
    @objc static var needsReauthorization: Bool {
        return UserDefaults.standard.bool(forKey: userDefaultsNeedsReauthorization)
    }
    
    
    /// If there is an OpenStreetMap profile existing in the app
    @objc static var isExisting: Bool {
        return authorizationToken != nil
    }
    
    
    /// The optional name of the OpenStreetMap profile
    @objc static var name: String? {
        if isExisting {
            return UserDefaults.standard.string(forKey: userDefaultsKeyName)
        }
        
        return nil
    }
    
    
    /// The optional number of edits of the OpenStreetMap profile
    static var numberOfEdits: Int? {
        if isExisting {
            return UserDefaults.standard.integer(forKey: userDefaultsKeyNumberOfEdits)
        }
        
        return nil
    }
    
    
    /// The optional URL for the edit history of the OpenStreetMap profile
    static var editHistoryUrl: URL? {
        if let name, let url = URL(string: String(describing: osm.OsmOAuth.ServerAuth().GetHistoryURL(std.string(name)))) {
            return url
        }
        
        return nil
    }
    
    
    /// The optional URL for the map notes of the OpenStreetMap profile
    static var notesUrl: URL? {
        if let name, let url = URL(string: String(describing: osm.OsmOAuth.ServerAuth().GetNotesURL(std.string(name)))) {
            return url
        }
        
        return nil
    }
    
    
    /// The URL for deleting an OpenStreetMap profile
    static var deleteUrl: URL {
        return URL(string: String(describing: osm.OsmOAuth.ServerAuth().GetDeleteURL()))!
    }
    
    
    
    // MARK: Methods
    
    /// Save the authorization token based on a code during the Oauth process
    /// - Parameter authorizationCode: The code
    static func saveAuthorizationToken(from authorizationCode: String, state: String?) async {
        guard let codeVerifier = consumeCodeVerifier(for: state) else {
            return
        }
        
        var serverAuth = osm.OsmOAuth.ServerAuth()
        
        let authorizationToken = String(describing: serverAuth.FinishAuthorization(std.string(authorizationCode), std.string(codeVerifier)))
        guard !authorizationToken.isEmpty else {
            return
        }
        
        serverAuth.SetAuthToken(std.string(authorizationToken))
        
        let userDefaults = UserDefaults.standard
        userDefaults.set(authorizationToken, forKey: userDefaultsKeyAuthorizationToken)
        if let userPreferences = await reloadUserPreferences() {
            userDefaults.set(String(describing: userPreferences.m_displayName), forKey: userDefaultsKeyName)
            userDefaults.set(Int(userPreferences.m_changesets), forKey: userDefaultsKeyNumberOfEdits)
            userDefaults.set(false, forKey: userDefaultsNeedsReauthorization)
        }
    }
    
    
    static private func consumeCodeVerifier(for state: String?) -> String? {
        let userDefaults = UserDefaults.standard
        guard let state,
              let expectedState = userDefaults.string(forKey: userDefaultsKeyOAuth2State),
              let codeVerifier = userDefaults.string(forKey: userDefaultsKeyOAuth2CodeVerifier),
              Data(state.utf8) == Data(expectedState.utf8) else {
            return nil
        }
        userDefaults.removeObject(forKey: userDefaultsKeyOAuth2State)
        userDefaults.removeObject(forKey: userDefaultsKeyOAuth2CodeVerifier)
        return codeVerifier
    }
    
    
    static private func randomUrlSafeString() -> String {
        var generator = SystemRandomNumberGenerator()
        let bytes = (0..<32).map { _ in UInt8.random(in: UInt8.min...UInt8.max, using: &generator) }
        return base64UrlEncoded(Data(bytes))
    }
    
    
    static private func base64UrlEncoded(_ data: Data) -> String {
        return data.base64EncodedString()
            .replacingOccurrences(of: "+", with: "-")
            .replacingOccurrences(of: "/", with: "_")
            .replacingOccurrences(of: "=", with: "")
    }
    
    
    /// Reload the OpenStreetMap profile data
    /// - Returns: Optional profile data
    static private func reloadUserPreferences() async -> osm.UserPreferences? {
        var userPreferences: osm.UserPreferences? = nil
        userPreferences = osm.ServerApi06(osm.OsmOAuth.ServerAuth(std.string(authorizationToken ?? ""))).GetUserPreferences()
        if let userPreferences, userPreferences.m_id == 0 {
            return nil
        }
        return userPreferences
    }
    
    
    /// Reload the OpenStreetMap profile
    static func reload() async {
        if isExisting {
            // Could be done in nicer way, but that would require iOS 17+
            await withCheckedContinuation { continuation in
                let networkPathMonitor: NWPathMonitor = NWPathMonitor()
                networkPathMonitor.pathUpdateHandler = { path in
                    Task {
                        if path.status != .unsatisfied {
                            let userDefaults = UserDefaults.standard
                            if let userPreferences = await reloadUserPreferences() {
                                userDefaults.set(String(describing: userPreferences.m_displayName), forKey: userDefaultsKeyName)
                                userDefaults.set(Int(userPreferences.m_changesets), forKey: userDefaultsKeyNumberOfEdits)
                            } else if path.status == .satisfied {
                                userDefaults.set(true, forKey: userDefaultsNeedsReauthorization)
                            }
                        }
                        networkPathMonitor.cancel()
                        continuation.resume()
                    }
                }
                networkPathMonitor.start(queue: .main)
            }
        }
    }
    
    
    /// Logout of the OpenStreetMap profile
    static func logout() {
        let userDefaults = UserDefaults.standard
        userDefaults.removeObject(forKey: userDefaultsKeyAuthorizationToken)
        userDefaults.removeObject(forKey: userDefaultsKeyName)
        userDefaults.removeObject(forKey: userDefaultsKeyNumberOfEdits)
        userDefaults.removeObject(forKey: userDefaultsNeedsReauthorization)
    }
}
