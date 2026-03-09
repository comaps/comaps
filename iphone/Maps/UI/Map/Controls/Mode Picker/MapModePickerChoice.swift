import SwiftUI

extension MapModePicker {
    /// View for a choice in a map mode picker
    struct Choice: View {
        // MARK: Properties
        
        /// The selected mode
        @Binding var selectedMode: Mode
        
        
        /// The mode
        var mode: Mode = .walking
        
        
        /// If a mode is currently being dragged
        @Binding var isDragging: Bool
        
        
        /// The dragged mode to not have too quick mode changes when dragging
        @Binding var draggedMode: Mode
        
        
        /// The foreground color (for animations)
        @State private var foregroundColor: Color = .secondary
        
        
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
                .contentShape(Rectangle())
                .onTapGesture {
                    if !isDragging {
                        selectedMode = mode
                    }
                }
                .onAppear {
                    foregroundColor = (draggedMode == mode ? .white : .secondary)
                }
                .onChange(of: draggedMode) { changedDraggedMode in
                    if changedDraggedMode == mode, foregroundColor == .secondary {
                        withAnimation(.spring.speed(3.2).delay(0.1)) {
                            foregroundColor = .white
                        }
                    } else if changedDraggedMode != mode, foregroundColor == .white {
                        withAnimation(.spring.speed(4)) {
                            foregroundColor = .secondary
                        }
                    } else {
                        foregroundColor = (changedDraggedMode == mode ? .white : .secondary)
                    }
                }
        }
    }
}
