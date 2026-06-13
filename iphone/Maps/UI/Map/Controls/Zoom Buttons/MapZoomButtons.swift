import SwiftUI

/// View for the map zoom buttons
struct MapZoomButtons: View {
    // MARK: Properties
    
    /// The maximum drag height
    var maxDragHeight: CGFloat
    
    
    /// If it is allowed to drag
    @State private var isAllowedToDrag: Bool = false
    
    
    /// The current drag distance
    @State private var dragDistance: CGFloat = 0
    
    
    /// If a drag currently is being detected
    @GestureState private var isDetectingDrag = false
    
    
    /// The actual view
    var body: some View {
        GeometryReader { geometry in
            VStack(spacing: 24) {
                MapZoomButton(kind: .in)
                    .frame(height: geometry.size.width)
                
                MapZoomButton(kind: .out)
                    .frame(height: geometry.size.width)
            }
            .opacity(isAllowedToDrag ? 0 : 1)
            .accessibilityHidden(isAllowedToDrag)
            .disabled(isAllowedToDrag)
            .background {
                if isAllowedToDrag {
                    if #unavailable(iOS 27, macOS 27) {
                        RoundedRectangle(cornerRadius: 28)
                            .stroke(Color.MapButtons.border, lineWidth: 1)
                            .background {
                                RoundedRectangle(cornerRadius: 28)
                                    .fill(EllipticalGradient(colors: [Color.MapButtons.backgroundGlow, Color.MapButtons.background]))
                                    .opacity(0.9)
                            }
                            .shadow(radius: 2)
                            .overlay(content: {
                                VStack(spacing: 0) {
                                    MapZoomButton.Kind.in.image
                                        .font(.title2)
                                        .scaleEffect(1.1)
                                        .foregroundStyle(Color.mapButtonForeground)
                                        .scaleEffect(0.96)
                                        .padding(.top, 15)
                                    
                                    Spacer(minLength: 0)
                                    
                                    MapZoomButton.Kind.out.image
                                        .font(.title2)
                                        .scaleEffect(1.1)
                                        .foregroundStyle(Color.mapButtonForeground)
                                        .scaleEffect(0.96)
                                        .padding(.bottom, 23)
                                }
                            })
                            .compositingGroup()
                    } else {
                        RoundedRectangle(cornerRadius: 28)
                            .fill(Color.MapButtons.background.opacity(0.3))
                            .glassEffect(.regular, in: RoundedRectangle(cornerRadius: 28))
                            .shadow(radius: 2)
                            .overlay(content: {
                                VStack(spacing: 0) {
                                    MapZoomButton.Kind.in.image
                                        .font(.title2)
                                        .scaleEffect(1.1)
                                        .foregroundStyle(Color.mapButtonForeground)
                                        .scaleEffect(0.96)
                                        .padding(.top, 15)
                                    
                                    Spacer(minLength: 0)
                                    
                                    MapZoomButton.Kind.out.image
                                        .font(.title2)
                                        .scaleEffect(1.1)
                                        .foregroundStyle(Color.mapButtonForeground)
                                        .scaleEffect(0.96)
                                        .padding(.bottom, 23)
                                }
                            })
                            .compositingGroup()
                    }
                }
            }
            .animation(.spring.speed(5), value: isAllowedToDrag)
            .simultaneousGesture(
                LongPressGesture(minimumDuration: 0.3)
                    .onEnded({ _ in
                        isAllowedToDrag = true
                    }).sequenced(
                        before: DragGesture()
                            .updating($isDetectingDrag) { currentState, state, transaction in
                                state = true
                            }
                            .onChanged({ value in
                                if isAllowedToDrag {
                                    if abs(value.translation.width) < 100 {
                                        let changedDragDistance = (-20 * value.translation.height) / maxDragHeight
                                        let dragDifference = dragDistance - changedDragDistance
                                        if abs(dragDifference) > 0.3 {
                                            dragDistance = changedDragDistance
                                            let scale: CGFloat
                                            if dragDifference > 0 {
                                                scale = 1.1 * abs(dragDifference)
                                            } else {
                                                scale = 10 * abs(dragDifference)
                                            }
                                            
                                            if scale != 0 {
                                                MapControls.zoom(scale: scale)
                                            }
                                        }
                                    } else {
                                        isAllowedToDrag = false
                                        dragDistance = 0
                                    }
                                }
                            })
                    ).updating(
                        $isDetectingDrag,
                        body: { _, state, _ in
                            state = true
                        })
            )
            .onChange(of: isDetectingDrag) { changedIsDetectingDrag in
                if !changedIsDetectingDrag {
                    isAllowedToDrag = false
                    dragDistance = 0
                }
            }
        }
    }
}
