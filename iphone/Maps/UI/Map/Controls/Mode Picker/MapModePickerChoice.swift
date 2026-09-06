import SwiftUI

extension MapModePicker {
    /// View for a choice in a map mode picker
    struct Choice: View {
        // MARK: Properties
        
        /// The selected mode
        @Binding var selectedMode: Mode
        
        
        /// The mode
        var mode: Mode = .walking


        /// If the mode options are being presented
        var isPresentingModeOptions: Bool
        
        
        /// If a mode is currently being dragged
        @Binding var isDragging: Bool
        
        
        /// The dragged mode to not have too quick mode changes when dragging
        @Binding var draggedMode: Mode


        /// The shared VoiceOver focus of the mode picker
        var accessibilityFocus: AccessibilityFocusState<AccessibilityFocus?>.Binding


        /// Toggles the options for the selected mode
        var toggleModeOptions: () -> Void
        
        
        /// The foreground color (for animations)
        @State private var foregroundColor: Color = .primary
        
        
        /// The actual view
        var body: some View {
            Circle()
                .fill(.clear)
                .overlay {
                    mode.image
                        .resizable()
                        .aspectRatio(contentMode: .fit)
                        .padding(mode == .cycling ? 8 : (mode == .walking ? 9 : 10))
                }
                .foregroundStyle(foregroundColor)
                .aspectRatio(1, contentMode: .fit)
                .contentShape(Rectangle())
                .onTapGesture {
                    if !isDragging, !isPresentingModeOptions {
                        activate()
                    }
                }
                .accessibilityRepresentation {
                    Button {
                        activate()
                    } label: {
                        Text(mode.description)
                    }
                    .accessibilityAddTraits(mode == selectedMode ? .isSelected : [])
                    .accessibilityHint(accessibilityHint)
                    .accessibilityFocused(accessibilityFocus, equals: .mode(mode))
                }
                .accessibilityHidden(isPresentingModeOptions)
                .onAppear {
                    foregroundColor = (draggedMode == mode ? .white : .primary)
                }
                .onChange(of: draggedMode) { changedDraggedMode in
                    if changedDraggedMode == mode, foregroundColor == .primary {
                        withAnimation(.spring.speed(3.2).delay(0.1)) {
                            foregroundColor = .white
                        }
                    } else if changedDraggedMode != mode, foregroundColor == .white {
                        withAnimation(.spring.speed(4)) {
                            foregroundColor = .primary
                        }
                    } else {
                        foregroundColor = (changedDraggedMode == mode ? .white : .primary)
                    }
                }
        }


        /// The VoiceOver instruction for the selected mode
        private var accessibilityHint: Text {
            guard mode == selectedMode else {
                return Text("")
            }

            return Text("mode_options_accessibility_hint")
        }


        /// Selects the mode, or toggles its options if it is already selected
        private func activate() {
            if mode == selectedMode {
                toggleModeOptions()
            } else {
                selectedMode = mode
            }
        }
    }
}
