import SwiftUI

/// View for configuring the map download server
struct CustomMapServerView: View {
    // MARK: Properties

    /// The value
    @State private var customMapDownloadUrl: String = ""


    /// Save callback
    private let onSaved: (String) -> Void


    /// Validation error shown below the URL field
    @State private var customMapDownloadUrlError: String?


    /// Prevents the initial setting triggering a save
    @State private var readyToSave: Bool = false


    /// Pending save
    @State private var saveTask: Task<Void, Never>?


    /// Whether the current value is valid
    @State private var isCustomMapDownloadUrlValid: Bool = false

    init(onSaved: @escaping (String) -> Void = { _ in }) {
        self.onSaved = onSaved
    }


    /// The actual view
    var body: some View {
        Form {
            Section {
                HStack {
                    ZStack(alignment: .leading) {
                        if customMapDownloadUrl.isEmpty {
                            Text(verbatim: "https://cdn-fi-1.comaps.app/")
                                .foregroundColor(.gray)
                                .allowsHitTesting(false)
                        }

                        TextField("", text: $customMapDownloadUrl)
                            .textInputAutocapitalization(.never)
                            .autocorrectionDisabled()
                            .keyboardType(.URL)
                            .textFieldStyle(.plain)
                    }

                    if isCustomMapDownloadUrlValid {
                        Image(systemName: "checkmark.circle.fill")
                            .foregroundColor(.green)
                    }
                }

                if let customMapDownloadUrlError {
                    Text(customMapDownloadUrlError)
                        .font(.footnote)
                        .foregroundColor(.red)
                }
            } footer: {
                VStack(alignment: .leading, spacing: 12) {
                    Text("custom_map_server_message")
                    Text(String(format: L("custom_map_server_map_series"), Settings.mapSeries))
                }
            }
        }
        .navigationTitle("custom_map_server")
        .navigationBarTitleDisplayMode(.inline)
        .accentColor(.accent)
        .onAppear {
            customMapDownloadUrl = Settings.customMapDownloadUrl
            readyToSave = true
        }
        .onChange(of: customMapDownloadUrl) { _ in
            guard readyToSave else { return }

            customMapDownloadUrlError = nil
            isCustomMapDownloadUrlValid = false
            saveTask?.cancel()
            saveTask = Task { @MainActor in
                do {
                    try await Task.sleep(nanoseconds: 500_000_000)
                    guard !Task.isCancelled else { return }
                    saveCustomMapDownloadUrl()
                } catch {
                    // ignored
                }
            }
        }
        .onDisappear {
            saveTask?.cancel()
        }
    }

    // MARK: Methods

    private func saveCustomMapDownloadUrl() {
        let url = customMapDownloadUrl.trimmingCharacters(in: .whitespacesAndNewlines)
        if url.isEmpty || url.lowercased().hasPrefix("http://") || url.lowercased().hasPrefix("https://") {
            Settings.customMapDownloadUrl = url
            isCustomMapDownloadUrlValid = !url.isEmpty
            customMapDownloadUrlError = nil
            onSaved(url)
            return
        } else {
            isCustomMapDownloadUrlValid = false
            customMapDownloadUrlError = L("custom_map_server_error_scheme")
        }
    }
}
