import SwiftUI

/// View for a map mode picker
struct MapModePicker: View {
    // MARK: Properties
    
    /// The horizontal size class of the environment
    @Environment(\.horizontalSizeClass) private var horizontalSizeClass
    
    
    /// The vertical size class of the environment
    @Environment(\.verticalSizeClass) private var verticalSizeClass
    
    
    /// The selected mode
    @State private var selectedMode: Mode = MapControls.mode
    
    
    /// If the mode options are being presented
    @State private var isPresentingModeOptions: Bool = false
    
    
    /// If the public transport mode has transit lines
    @State private var hasTransitLinesForPublicTransportMode: Bool = true
    
    
    /// If the layers options, that might being presented via the extended layers button, should be temporarily hidden to not be weirdly overlapped by for example the mode options
    @Binding var shouldTemporarilyHideLayersIfNecessary: Bool
    
    
    /// If the dragging should be prevented, because it didn't start at the selected mode
    @State private var preventDraging: Bool = false
    
    
    /// If a mode is currently being dragged
    @State private var isDragging: Bool = false
    
    
    /// The dragged mode to not have too quick mode changes when dragging
    @State private var draggedMode: Mode = MapControls.mode
    
    
    /// The actual view
    var body: some View {
        GeometryReader { geometry in
            VStack(spacing: 0) {
                HStack(spacing: 0) {
                    ForEach(Mode.allCases) { mode in
                        if !isPresentingModeOptions || mode == selectedMode {
                            MapModePicker.Choice(selectedMode: $selectedMode, mode: mode, isDragging: $isDragging, draggedMode: $draggedMode)
                                .simultaneousGesture(
                                    LongPressGesture().onEnded { _ in
                                        if !isDragging {
                                            selectedMode = mode
                                            shouldTemporarilyHideLayersIfNecessary = true
                                            isPresentingModeOptions.toggle()
                                        }
                                    }
                                )
                        }
                    }
                    
                    if isPresentingModeOptions {
                        Text(selectedMode.description)
                            .font(.title2)
                            .bold()
                            .foregroundStyle(Color.white)
                        
                        Spacer(minLength: 0)
                        
                        Button {
                            isPresentingModeOptions.toggle()
                            shouldTemporarilyHideLayersIfNecessary = false
                        } label: {
                            Label("close", systemImage: "xmark.circle.fill")
                                .labelStyle(.iconOnly)
                                .imageScale(.large)
                                .padding(.horizontal, 8)
                        }
                        .buttonStyle(.plain)
                        .font(.title2)
                        .foregroundStyle(Color.white.opacity(0.5))
                    }
                }
                .padding(isPresentingModeOptions ? 4 : 0)
                .background(alignment: .leading) {
                    HStack(spacing: 0) {
                        if !isPresentingModeOptions {
                            ForEach(0..<draggedMode.id, id: \.self) { _ in
                                Circle()
                                    .hidden()
                            }
                        }
                        
                        Capsule()
                            .fill(isPresentingModeOptions ? Color.clear : draggedMode.color)
                        
                        if !isPresentingModeOptions {
                            ForEach((draggedMode.id + 1)..<Mode.allCases.count, id: \.self) { _ in
                                Circle()
                                    .hidden()
                            }
                        }
                    }
                    .compositingGroup()
                    .animation(.spring.speed(1.5), value: draggedMode)
                }
                .simultaneousGesture(
                    DragGesture(minimumDistance: 0, coordinateSpace: .local).onChanged({ value in
                        if !preventDraging, let mode = Mode(rawValue: min(max(Int(Float(value.location.x / (CGFloat(Mode.allCases.count) * CGFloat(geometry.size.height - 8)) * 4).rounded(.up)), 1), Mode.allCases.count) - 1) {
                            if isDragging {
                                if selectedMode != mode, draggedMode != mode {
                                    draggedMode = mode
                                    Task {
                                        try? await Task.sleep(nanoseconds: 160_000_000)
                                        
                                        if draggedMode == mode {
                                            selectedMode = mode
                                        }
                                    }
                                }
                            } else if mode == selectedMode {
                                if abs(value.translation.width) > 7 {
                                    isDragging = true
                                }
                            } else {
                                preventDraging = true
                            }
                        }
                    }).onEnded({ _ in
                        Task {
                            isDragging = false
                            preventDraging = false
                        }
                    })
                )
                .padding(isPresentingModeOptions ? 0 : 4)
                .background {
                    if isPresentingModeOptions {
                        ZStack {
                            RoundedRectangle(cornerRadius: 28)
                                .fill(selectedMode.color)
                            
                            VStack {
                                Color.black
                                    .hidden()
                                
                                selectedMode.color
                            }
                        }
                        .compositingGroup()
                    }
                }
                .frame(height: geometry.size.height)
                
                if isPresentingModeOptions {
                    VStack {
                        if selectedMode == .publicTransport {
                            Toggle("mode_option_transitlines", isOn: $hasTransitLinesForPublicTransportMode)
                                .tint(selectedMode.color)
                        } else {
                            Toggle("mode_option_comingsoon", isOn: .constant(false))
                                .hidden()
                                .overlay {
                                    Label("mode_option_comingsoon", systemImage: "clock")
                                }
                        }
                    }
                    .padding()
                }
            }
            .background {
                if isPresentingModeOptions {
                    if #unavailable(anyAppleOS 27) {
                        RoundedRectangle(cornerRadius: 25)
                            .stroke(selectedMode.color, lineWidth: 1)
                            .background {
                                RoundedRectangle(cornerRadius: 25)
                                    .fill(Color.MapButtons.background)
                            }
                            .shadow(radius: 2)
                            .foregroundStyle(Color.secondary)
                            .compositingGroup()
                    } else {
                        RoundedRectangle(cornerRadius: 25)
                            .stroke(selectedMode.color, lineWidth: 1)
                            .background {
                                RoundedRectangle(cornerRadius: 25)
                                    .fill(Color.MapButtons.background.opacity(0.5))
                                    .glassEffect(.regular, in: RoundedRectangle(cornerRadius: 25))
                            }
                            .shadow(radius: 2)
                            .foregroundStyle(Color.secondary)
                            .compositingGroup()
                    }
                } else {
                    if #unavailable(anyAppleOS 27) {
                        RoundedRectangle(cornerRadius: 28)
                            .stroke(Color.MapButtons.border, lineWidth: 1)
                            .background {
                                RoundedRectangle(cornerRadius: 28)
                                    .fill(EllipticalGradient(colors: [Color.MapButtons.backgroundGlow, Color.MapButtons.background]))
                            }
                            .shadow(radius: 2)
                            .foregroundStyle(Color.secondary)
                            .compositingGroup()
                    } else {
                        RoundedRectangle(cornerRadius: 28)
                            .fill(Color.MapButtons.background.opacity(0.3))
                            .glassEffect(.regular, in: RoundedRectangle(cornerRadius: 28))
                            .shadow(radius: 2)
                    }
                }
            }
            .padding(4)
            .onTapGesture {
                // Trick to extend the safe area around it
            }
            .padding(-4)
            .contentShape(Rectangle())
            .accessibilityRepresentation {
                Picker("mode", selection: $selectedMode) {
                    ForEach(Mode.allCases) { mode in
                        Text(mode.description)
                    }
                }
            }
            .frame(maxWidth: !isPresentingModeOptions || (horizontalSizeClass == .compact && verticalSizeClass != .compact) ? .infinity : 320)
            .frame(maxWidth: .infinity, alignment: .center)
            .animation(.spring.speed(2), value: isPresentingModeOptions)
            .onChange(of: selectedMode) { changedSelectedMode in
                draggedMode = changedSelectedMode
                selectedMode = changedSelectedMode
                MapControls.mode = changedSelectedMode
            }
        }
        .onAppear {
            hasTransitLinesForPublicTransportMode = MapControls.publicTransportModeHasTransitLines()
        }
        .onChange(of: hasTransitLinesForPublicTransportMode) { changedHasTransitLinesForPublicTransport in
            hasTransitLinesForPublicTransportMode = changedHasTransitLinesForPublicTransport
            MapControls.publicTransportModeSetTransitLines(changedHasTransitLinesForPublicTransport)
        }
    }
}
