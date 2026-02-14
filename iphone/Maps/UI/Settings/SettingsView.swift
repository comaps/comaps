import SwiftUI

/// View for the settings
struct SettingsView: View {
    // MARK: Properties
    
    /// The dismiss action of the environment
    @Environment(\.dismiss) private var dismiss
    
    
    /// The selected distance unit
    @State private var selectedDistanceUnit: Settings.DistanceUnit = .metric
    
    
    /// If zoom buttons should be displayed
    @State private var hasZoomButtons: Bool = true
    
    
    /// The selected left button type
    @State private var selectedLeftButtonType: Settings.LeftButtonType = .help
    
    
    /// If 3D buildings should be displayed
    @State private var has3dBuildings: Bool = true
    
    
    /// If automatic map downloads should be enabled
    @State private var hasAutomaticDownload: Bool = true
    
    
    /// If an increased font size should be used for map labels
    @State private var hasIncreasedFontsize: Bool = false
    
    
    /// The selected language for the map
    @State var selectedLanguageForMap: Settings.MapLanguage.ID? = nil
    
    
    /// The selected way to handle alternatve map languages
    @State private var alternativeMapLanguageHandling: Settings.AlternativeMapLanguageHandling = .localOnly
    
    
    /// If names should be transliterated to Latin
    @State private var shouldTransliterateToLatin: Bool = true
    
    
    /// The selected map appearance
    @State private var selectedMapAppearance: Settings.Appearance = .auto
    
    
    /// The selected appearance
    @State private var selectedAppearance: Settings.Appearance = .auto
    
    
    /// If the bookmarks should be synced via iCloud
    @State private var shouldSync: Bool = false
    
    
    /// If the sync beta alert should be shown
    @State private var showSyncBetaAlert: Bool = false
    
    
    /// If the sync is possible
    @State private var isSyncPossible: Bool = true
    
    
    /// The selected power saving mode
    @State private var selectedPowerSavingMode: Settings.PowerSavingMode = .never
    
    
    /// The selected mobile data policy
    @State private var selectedMobileDataPolicy: Settings.MobileDataPolicy = .always
    
    
    /// If our custom logging is enabled
    @State private var isLogging: Bool = false
    
    
    /// The actual view
    var body: some View {
        NavigationView {
            List {
                Section {
                    NavigationLink {
                        ProfileView()
                    } label: {
                        HStack {
                            Text("osm_profile")
                                .lineLimit(1)
                                .layoutPriority(2)
                            
                            Spacer(minLength: 0)
                                .layoutPriority(0)
                            
                            Text(Profile.name ?? String())
                                .lineLimit(1)
                                .foregroundStyle(.secondary)
                                .layoutPriority(1)
                        }
                    }
                }
                
                Section {
                    Picker(selection: $selectedDistanceUnit) {
                        ForEach(Settings.DistanceUnit.allCases) { distanceUnit in
                            Text(distanceUnit.description)
                        }
                    } label: {
                        Text("measurement_units")
                    }
                    
                    Toggle("pref_zoom_title", isOn: $hasZoomButtons)
                        .tint(.accent)
                    
                    Picker(selection: $selectedLeftButtonType) {
                        ForEach(Settings.LeftButtonType.allCases) { leftButtonType in
                            Text(leftButtonType.description)
                        }
                    } label: {
                        Text("pref_left_button_type")
                    }
                    
                    Toggle(isOn: $has3dBuildings) {
                        VStack(alignment: .leading) {
                            Text("pref_map_3d_buildings_title")
                            
                            if selectedPowerSavingMode == .maximum {
                                Text("pref_map_3d_buildings_disabled_summary")
                                    .font(.footnote)
                                    .foregroundStyle(.secondary)
                            }
                        }
                    }
                    .tint(.accent)
                    .disabled(selectedPowerSavingMode == .maximum)
                    
                    Toggle("autodownload", isOn: $hasAutomaticDownload)
                        .tint(.accent)
                    
                    Toggle("big_font", isOn: $hasIncreasedFontsize)
                        .tint(.accent)
                    
                    Picker(selection: $selectedLanguageForMap) {
                        ForEach(Settings.availableLanguagesForMap) { languageForMap in
                            Text(languageForMap.localizedName)
                                .tag(languageForMap.id)
                            
                            if languageForMap.id == "auto" || languageForMap.id == "default" {
                                Divider()
                            }
                        }
                    } label: {
                        Text("pref_maplanguage_title")
                    }
                    
                    Picker(selection: $alternativeMapLanguageHandling) {
                        ForEach(Settings.AlternativeMapLanguageHandling.allCases) { alternativeMapLanguageHandling in
                            Text(alternativeMapLanguageHandling.description)
                        }
                    } label: {
                        VStack(alignment: .leading) {
                            Text("pref_alt_map_lang_handling")
                            
                            if selectedLanguageForMap == "default" {
                                Text("transliteration_title_disabled_summary")
                                    .font(.footnote)
                                    .foregroundStyle(.secondary)
                            }
                        }
                    }
                    .disabled(selectedLanguageForMap == "default")
                    
                    Toggle(isOn: $shouldTransliterateToLatin) {
                        VStack(alignment: .leading) {
                            Text("transliteration_title")
                            
                            if selectedLanguageForMap == "default" {
                                Text("transliteration_title_disabled_summary")
                                    .font(.footnote)
                                    .foregroundStyle(.secondary)
                            }
                        }
                    }
                    .tint(.accent)
                    .disabled(selectedLanguageForMap == "default")
                    
                    Picker(selection: $selectedMapAppearance) {
                        ForEach(Settings.Appearance.allCases) { mapAppearance in
                            Text(mapAppearance.description)
                        }
                    } label: {
                        Text("pref_mapappearance_title")
                    }
                }
                
                NavigationLink("prefs_group_route") {
                    SettingsNavigationView()
                }
                
                Section {
                    Toggle(isOn: $shouldSync) {
                        VStack(alignment: .leading) {
                            Text("icloud_sync")
                            
                            if !isSyncPossible {
                                Text("icloud_disabled_message")
                                    .font(.footnote)
                                    .foregroundStyle(.secondary)
                            }
                        }
                    }
                    .tint(.accent)
                    .disabled(!isSyncPossible)
                    .alert("enable_icloud_synchronization_title", isPresented: $showSyncBetaAlert) {
                        Button {
                            Settings.hasShownSyncBetaAlert = true
                            Settings.shouldSync = true
                            shouldSync = true
                        } label: {
                            Text("enable")
                        }
                        
                        Button {
                            Settings.createBookmarksBackupBecauseOfSyncBeta { hasCreatedBackup in
                                if hasCreatedBackup {
                                    Settings.hasShownSyncBetaAlert = true
                                    Settings.shouldSync = true
                                    shouldSync = true
                                } else {
                                    Settings.shouldSync = false
                                    shouldSync = false
                                }
                            }
                        } label: {
                            Text("backup")
                        }
                        
                        Button(role: .cancel) {
                            // Do nothing
                        } label: {
                            Text("cancel")
                        }
                    } message: {
                        Text("enable_icloud_synchronization_message")
                    }
                    
                }
                
                Section {
                    Picker(selection: $selectedAppearance) {
                        ForEach(Settings.Appearance.allCases) { appearance in
                            Text(appearance.description)
                        }
                    } label: {
                        Text("pref_appearance_title")
                    }
                    
                    Picker(selection: $selectedPowerSavingMode) {
                        ForEach(Settings.PowerSavingMode.allCases) { powerSavingMode in
                            Text(powerSavingMode.description)
                        }
                    } label: {
                        Text("power_managment_title")
                    }
                    
                    Picker(selection: $selectedMobileDataPolicy) {
                        ForEach(Settings.MobileDataPolicy.allCases) { mobileDataPolicy in
                            Text(mobileDataPolicy.description)
                        }
                    } label: {
                        Text("mobile_data")
                    }
                }
                
                Section {
                    Toggle(isOn: $isLogging) {
                        VStack(alignment: .leading) {
                            Text("enable_logging")
                            
                            if isLogging {
                                Text(Settings.logSize, formatter: Settings.logSizeFormatter)
                                    .font(.footnote)
                                    .foregroundStyle(.secondary)
                            }
                        }
                    }
                    .tint(.accent)
                } footer: {
                    Text("enable_logging_warning_message")
                }
            }
            .navigationTitle("settings")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .confirmationAction) {
                    Button {
                        dismiss()
                    } label: {
                        Text("close")
                    }
                }
            }
        }
        .navigationViewStyle(StackNavigationViewStyle())
        .onAppear {
            selectedDistanceUnit = Settings.distanceUnit
            hasZoomButtons = Settings.hasZoomButtons
            selectedLeftButtonType = Settings.leftButtonType
            has3dBuildings = Settings.has3dBuildings
            hasAutomaticDownload = Settings.hasAutomaticDownload
            hasIncreasedFontsize = Settings.hasIncreasedFontsize
            selectedLanguageForMap = Settings.languageForMap
            alternativeMapLanguageHandling = Settings.alternativeMapLanguageHandling
            shouldTransliterateToLatin = Settings.shouldTransliterateToLatin
            selectedMapAppearance = Settings.mapAppearance
            selectedAppearance = Settings.appearance
            shouldSync = Settings.shouldSync
            selectedPowerSavingMode = Settings.powerSavingMode
            selectedMobileDataPolicy = Settings.mobileDataPolicy
            isLogging = Settings.isLogging
        }
        .onChange(of: selectedDistanceUnit) { changedSelectedDistanceUnit in
            Settings.distanceUnit = changedSelectedDistanceUnit
        }
        .onChange(of: hasZoomButtons) { changedHasZoomButtons in
            Settings.hasZoomButtons = changedHasZoomButtons
        }
        .onChange(of: selectedLeftButtonType) { changedSelectedLeftButtonType in
            Settings.leftButtonType = changedSelectedLeftButtonType
        }
        .onChange(of: has3dBuildings) { changedHas3dBuildings in
            Settings.has3dBuildings = changedHas3dBuildings
        }
        .onChange(of: hasAutomaticDownload) { changedHasAutomaticDownload in
            Settings.hasAutomaticDownload = changedHasAutomaticDownload
        }
        .onChange(of: hasIncreasedFontsize) { changedHasIncreasedFontsize in
            Settings.hasIncreasedFontsize = changedHasIncreasedFontsize
        }
        .onChange(of: selectedLanguageForMap) { changedSelectedLanguageForMap in
            if let changedSelectedLanguageForMap {
                Settings.languageForMap = changedSelectedLanguageForMap
            }
        }
        .onChange(of: alternativeMapLanguageHandling) { changedAlternativeMapLanguageHandling in
            Settings.alternativeMapLanguageHandling = changedAlternativeMapLanguageHandling
        }
        .onChange(of: shouldTransliterateToLatin) { changedShouldTransliterateToLatin in
            Settings.shouldTransliterateToLatin = changedShouldTransliterateToLatin
        }
        .onChange(of: selectedMapAppearance) { changedSelectedMapAppearance in
            Settings.mapAppearance = changedSelectedMapAppearance
        }
        .onChange(of: selectedAppearance) { changedSelectedAppearance in
            Settings.appearance = changedSelectedAppearance
        }
        .onChange(of: shouldSync) { changedShouldSync in
            if changedShouldSync, !Settings.hasShownSyncBetaAlert {
                showSyncBetaAlert = true
                shouldSync = false
            } else {
                Settings.shouldSync = changedShouldSync
            }
        }
        .onChange(of: selectedPowerSavingMode) { changedSelectedPowerSavingMode in
            Settings.powerSavingMode = changedSelectedPowerSavingMode
        }
        .onChange(of: selectedMobileDataPolicy) { changedSelectedMobileDataPolicy in
            Settings.mobileDataPolicy = changedSelectedMobileDataPolicy
        }
        .onChange(of: isLogging) { changedIsLogging in
            Settings.isLogging = changedIsLogging
        }
        .onReceive(Settings.syncStatePublisher) { syncState in
            isSyncPossible = syncState.isAvailable
        }
        .accentColor(.toolbarAccent)
    }
}
