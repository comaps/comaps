extension Settings {
    /// How to handle alternative map languages
    enum AlternativeMapLanguageHandling: Int, Codable, CaseIterable, Identifiable {
        case ignoreAlternatives = 0
        case systemOrder = 1
        case localOnly = 2
        
        
        
        // MARK: Properties
        
        /// The id
        var id: Self { self }
        
        
        /// The description text
        var description: String {
            switch self {
                case .ignoreAlternatives:
                    return String(localized: "pref_alt_map_lang_handling_ignore_alternatives")
                case .systemOrder:
                    return String(localized: "pref_alt_map_lang_handling_system_order")
                case .localOnly:
                    return String(localized: "pref_alt_map_lang_handling_local_only")
            }
        }
    }
}
