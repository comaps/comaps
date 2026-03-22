import SwiftUI

/// View for the routing options
struct RoutingOptionsView: View {
    // MARK: Properties
    
    /// The dismiss action of the environment
    @Environment(\.dismiss) private var dismiss
    
    
    /// If toll roads should be avoided during routing
    @State var shouldAvoidTollRoadsWhileRouting: Bool = false
    
    
    /// If unpaved roads should be avoided during routing
    @State var shouldAvoidUnpavedRoadsWhileRouting: Bool = false
    
    
    /// If paved roads should be avoided during routing
    @State var shouldAvoidPavedRoadsWhileRouting: Bool = false
    
    
    /// If ferries should be avoided during routing
    @State var shouldAvoidFerriesWhileRouting: Bool = false
    
    
    /// If motorways should be avoided during routing
    @State var shouldAvoidMotorwaysWhileRouting: Bool = false
    
    
    /// If steps should be avoided during routing
    @State var shouldAvoidStepsWhileRouting: Bool = false
    
    /// Border crossing avoidance mode
    @State var borderAvoidanceMode: MWMBorderAvoidanceMode = .none



    /// The actual view
    var body: some View {
        NavigationView {
            List {
                Section {
                    Toggle(isOn: $shouldAvoidTollRoadsWhileRouting) {
                        Label {
                            Text("avoid_tolls")
                        } icon: {
                            Image(shouldAvoidTollRoadsWhileRouting ? "tolls.slash" : "tolls")
                                .foregroundStyle(.secondary)
                        }
                    }
                    .tint(.accent)
                    
                    Toggle(isOn: $shouldAvoidUnpavedRoadsWhileRouting) {
                        Label {
                            Text("avoid_unpaved")
                        } icon: {
                            Image(shouldAvoidUnpavedRoadsWhileRouting ? "unpaved.slash" : "unpaved")
                                .foregroundStyle(.secondary)
                        }
                    }
                    .tint(.accent)
                    .disabled(shouldAvoidPavedRoadsWhileRouting)
                    
                    Toggle(isOn: $shouldAvoidPavedRoadsWhileRouting) {
                        Label {
                            Text("avoid_paved")
                        } icon: {
                            Image(shouldAvoidPavedRoadsWhileRouting ? "paved.slash" : "paved")
                                .foregroundStyle(.secondary)
                        }
                    }
                    .tint(.accent)
                    .disabled(shouldAvoidUnpavedRoadsWhileRouting)
                    
                    Toggle(isOn: $shouldAvoidMotorwaysWhileRouting) {
                        Label {
                            Text("avoid_motorways")
                        } icon: {
                            Image(shouldAvoidMotorwaysWhileRouting ? "motorways.slash" : "motorways")
                                .foregroundStyle(.secondary)
                        }
                    }
                    .tint(.accent)
                    
                    Toggle(isOn: $shouldAvoidFerriesWhileRouting) {
                        Label {
                            Text("avoid_ferry")
                        } icon: {
                            Image(shouldAvoidFerriesWhileRouting ? "ferries.slash" : "ferries")
                                .foregroundStyle(.secondary)
                        }
                    }
                    .tint(.accent)
                    
                    Toggle(isOn: $shouldAvoidStepsWhileRouting) {
                        Label {
                            Text("avoid_steps")
                        } icon: {
                            Image(shouldAvoidStepsWhileRouting ? "steps.slash" : "steps")
                                .foregroundStyle(.secondary)
                        }
                    }
                    .tint(.accent)

                    NavigationLink {
                        BorderAvoidanceView(mode: $borderAvoidanceMode)
                    } label: {
                        HStack {
                            Label {
                                Text("avoid_border_crossing")
                            } icon: {
                                Image(systemName: "globe")
                                    .foregroundStyle(.secondary)
                            }
                            Spacer()
                            Text(borderAvoidanceModeLabel)
                                .foregroundStyle(.secondary)
                        }
                    }
                }
            }
            .navigationTitle(String(localized: "driving_options_title"))
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .confirmationAction) {
                    if #available(iOS 26, *) {
                        Button {
                            dismiss()
                        } label: {
                            Label("close", systemImage: "xmark")
                        }
                        .buttonStyle(.glassProminent)
                    } else {
                        Button {
                            dismiss()
                        } label: {
                            Text("close")
                        }
                    }
                }
            }
        }
        .navigationViewStyle(StackNavigationViewStyle())
        .onAppear {
            shouldAvoidTollRoadsWhileRouting = Settings.shouldAvoidTollRoadsWhileRouting
            shouldAvoidUnpavedRoadsWhileRouting = Settings.shouldAvoidUnpavedRoadsWhileRouting
            shouldAvoidPavedRoadsWhileRouting = Settings.shouldAvoidPavedRoadsWhileRouting
            shouldAvoidFerriesWhileRouting = Settings.shouldAvoidFerriesWhileRouting
            shouldAvoidMotorwaysWhileRouting = Settings.shouldAvoidMotorwaysWhileRouting
            shouldAvoidStepsWhileRouting = Settings.shouldAvoidStepsWhileRouting
            borderAvoidanceMode = Settings.borderAvoidanceMode
        }
        .onChange(of: shouldAvoidTollRoadsWhileRouting) { changedShouldAvoidTollRoadsWhileRouting in
            Settings.shouldAvoidTollRoadsWhileRouting = changedShouldAvoidTollRoadsWhileRouting
        }
        .onChange(of: shouldAvoidUnpavedRoadsWhileRouting) { changedShouldAvoidUnpavedRoadsWhileRouting in
            Settings.shouldAvoidUnpavedRoadsWhileRouting = changedShouldAvoidUnpavedRoadsWhileRouting
            if changedShouldAvoidUnpavedRoadsWhileRouting {
                shouldAvoidPavedRoadsWhileRouting = false
            }
        }
        .onChange(of: shouldAvoidPavedRoadsWhileRouting) { changedShouldAvoidPavedRoadsWhileRouting in
            Settings.shouldAvoidPavedRoadsWhileRouting = changedShouldAvoidPavedRoadsWhileRouting
            if changedShouldAvoidPavedRoadsWhileRouting {
                shouldAvoidUnpavedRoadsWhileRouting = false
            }
        }
        .onChange(of: shouldAvoidFerriesWhileRouting) { changedShouldAvoidFerriesWhileRouting in
            Settings.shouldAvoidFerriesWhileRouting = changedShouldAvoidFerriesWhileRouting
        }
        .onChange(of: shouldAvoidMotorwaysWhileRouting) { changedShouldAvoidMotorwaysWhileRouting in
            Settings.shouldAvoidMotorwaysWhileRouting = changedShouldAvoidMotorwaysWhileRouting
        }
        .onChange(of: shouldAvoidStepsWhileRouting) { changedShouldAvoidStepsWhileRouting in
            Settings.shouldAvoidStepsWhileRouting = changedShouldAvoidStepsWhileRouting
        }
        .onChange(of: borderAvoidanceMode) { newMode in
            Settings.borderAvoidanceMode = newMode
        }
        .accentColor(.toolbarAccent)
    }

    private var borderAvoidanceModeLabel: String {
        switch borderAvoidanceMode {
        case .any:
            return String(localized: "border_avoidance_any")
        case .nonInternal:
            return String(localized: "border_avoidance_non_internal")
        case .specific:
            let count = Settings.avoidedBorderCountries.count
            if count > 0 {
                return String(format: L("border_avoidance_selected_count"), count)
            }
            return String(localized: "border_avoidance_specific")
        case .none, _:
            return String(localized: "border_avoidance_none")
        }
    }
}

/// View for selecting border avoidance mode
struct BorderAvoidanceView: View {
    @Binding var mode: MWMBorderAvoidanceMode
    @State private var selectedMode: MWMBorderAvoidanceMode = .none

    var body: some View {
        List {
            Section {
                ForEach([MWMBorderAvoidanceMode.none, .any, .nonInternal, .specific], id: \.self) { option in
                    Button {
                        selectedMode = option
                        mode = option
                    } label: {
                        HStack {
                            Text(labelForMode(option))
                            Spacer()
                            if selectedMode == option {
                                Image(systemName: "checkmark")
                                    .foregroundStyle(.accent)
                            }
                        }
                    }
                    .foregroundStyle(.primary)
                }
            }

            if selectedMode == .specific {
                NavigationLink {
                    BorderCountriesView()
                } label: {
                    Text("border_avoidance_select_countries")
                }
            }
        }
        .navigationTitle(String(localized: "avoid_border_crossing"))
        .navigationBarTitleDisplayMode(.inline)
        .onAppear {
            selectedMode = mode
        }
    }

    private func labelForMode(_ mode: MWMBorderAvoidanceMode) -> String {
        switch mode {
        case .any:
            return String(localized: "border_avoidance_any")
        case .nonInternal:
            return String(localized: "border_avoidance_non_internal")
        case .specific:
            return String(localized: "border_avoidance_specific")
        case .none, _:
            return String(localized: "border_avoidance_none")
        }
    }
}

/// View for selecting specific countries to avoid crossing
struct BorderCountriesView: View {
    @State private var countries: [String] = []
    @State private var selectedCountries: Set<String> = []
    @State private var searchText: String = ""

    var filteredCountries: [String] {
        if searchText.isEmpty {
            return countries
        }
        return countries.filter { $0.localizedCaseInsensitiveContains(searchText) }
    }

    var body: some View {
        List {
            ForEach(filteredCountries, id: \.self) { country in
                Button {
                    if selectedCountries.contains(country) {
                        selectedCountries.remove(country)
                    } else {
                        selectedCountries.insert(country)
                    }
                    Settings.avoidedBorderCountries = selectedCountries
                } label: {
                    HStack {
                        Text(country)
                        Spacer()
                        if selectedCountries.contains(country) {
                            Image(systemName: "checkmark")
                                .foregroundStyle(.accent)
                        }
                    }
                }
                .foregroundStyle(.primary)
            }
        }
        .searchable(text: $searchText, placement: .navigationBarDrawer(displayMode: .always))
        .navigationTitle(String(localized: "border_avoidance_select_countries"))
        .navigationBarTitleDisplayMode(.inline)
        .onAppear {
            selectedCountries = Settings.avoidedBorderCountries
            loadCountries()
        }
    }

    private func loadCountries() {
        countries = RoutingOptions().topLevelCountries.sorted()
    }
}
