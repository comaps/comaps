import SwiftUI

extension MapLayersButton {
    /// View for a toogle of an extended map layers button
    struct LayerToggle: View {
        // MARK: Properties
        
        /// If the layer is visible
        @State private var isVisible: Bool = false
        
        
        /// If the layer is currently disabled for power saving
        @State private var isDisabledForPowerSaving: Bool = false
        
        
        /// The layer
        @State var layer: Layer
        
        
        /// If the power saving hint is being presented
        @State private var isPresentingPowerSavingHint: Bool = false
        
        
        /// The publisher for receiving the updates on the power saving adjustments
        private let changePowerSavingAdjustmentsPublisher = NotificationCenter.default.publisher(for: Settings.changePowerSavingAdjustmentsNotificationName)
        
        
        /// The actual view
        var body: some View {
            Button {
                if isDisabledForPowerSaving {
                    isPresentingPowerSavingHint = true
                } else {
                    withAnimation(.none) {
                        isVisible.toggle()
                    }
                }
            } label: {
                Label {
                    Text(layer.description)
                } icon: {
                    Image(layer.badgedImageName(isVisible: isVisible))
                }
            }
            .labelStyle(.iconOnly)
            .font(.title2)
            .symbolRenderingMode(.palette)
            .foregroundStyle((isVisible ? Color.BaseColors.green : Color.BaseColors.red), (isDisabledForPowerSaving ? .secondary.opacity(0.5) : layer.color))
            .contentShape(Rectangle())
            .id(layer.id)
            .alert("layers_power-saving", isPresented: $isPresentingPowerSavingHint, actions: {
                Button("ok") {
                    isPresentingPowerSavingHint = false
                }
            })
            .onAppear {
                isVisible = layer.isVisible
                isDisabledForPowerSaving = layer.isDisabledForPowerSaving
            }
            .onChange(of: isVisible) { changedIsVisible in
                layer.isVisible = changedIsVisible
            }
            .onReceive(changePowerSavingAdjustmentsPublisher) { _ in
                isDisabledForPowerSaving = layer.isDisabledForPowerSaving
            }
        }
    }
}
