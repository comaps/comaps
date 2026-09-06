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


        /// If the mode was activated by a long press
        @Binding var isLongPressingMode: Bool
        
        
        /// The dragged mode to not have too quick mode changes when dragging
        @Binding var draggedMode: Mode


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
                .overlay(alignment: .bottomTrailing) {
                    if mode == selectedMode {
                        Image(systemName: isPresentingModeOptions ? "chevron.up" : "chevron.down")
                            .font(.system(size: 7, weight: .bold))
                            .padding(6)
                            .accessibilityHidden(true)
                    }
                }
                .foregroundStyle(foregroundColor)
                .aspectRatio(1, contentMode: .fit)
                .contentShape(Rectangle())
                .accessibilityElement(children: .ignore)
                .accessibilityLabel(Text(mode.description))
                .accessibilityAddTraits(.isButton)
                .accessibilityAddTraits(mode == selectedMode ? .isSelected : [])
                .accessibilityHint(accessibilityHint)
                .accessibilityAction {
                    if !isDragging, !isLongPressingMode {
                        activate()
                    }
                }
                .onTapGesture {
                    if !isDragging, !isLongPressingMode {
                        activate()
                    }
                }
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

            return isPresentingModeOptions ? Text("hide") : Text("show")
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
