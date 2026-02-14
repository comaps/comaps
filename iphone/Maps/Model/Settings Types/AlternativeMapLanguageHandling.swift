extension Settings {
    /// How to handle alternative map languages
    enum AlternativeMapLanguageHandling: String, Codable, CaseIterable, Identifiable {
        case systemOrder = "SystemOrder"
        case localOnly = "LocalOnly"
        
        
        
        // MARK: Properties
        
        /// The id
        var id: Self { self }
        
        
        /// The description text
        var description: String {
            switch self {
                case .systemOrder:
                    return String(localized: "pref_alt_map_lang_handling_system_order")
                case .localOnly:
                    return String(localized: "pref_alt_map_lang_handling_local_only")
            }
        }
    }
}
