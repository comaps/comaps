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
                }
            }
            .navigationTitle(String(localized: "driving_options_title"))
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
            shouldAvoidTollRoadsWhileRouting = Settings.shouldAvoidTollRoadsWhileRouting
            shouldAvoidUnpavedRoadsWhileRouting = Settings.shouldAvoidUnpavedRoadsWhileRouting
            shouldAvoidPavedRoadsWhileRouting = Settings.shouldAvoidPavedRoadsWhileRouting
            shouldAvoidFerriesWhileRouting = Settings.shouldAvoidFerriesWhileRouting
            shouldAvoidMotorwaysWhileRouting = Settings.shouldAvoidMotorwaysWhileRouting
            shouldAvoidStepsWhileRouting = Settings.shouldAvoidStepsWhileRouting
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
        .accentColor(.toolbarAccent)
    }
}
