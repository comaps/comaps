import SwiftUI

/// View for a map layers button
struct MapLayersButton: View {
    // MARK: Properties
    
    /// The width
    @State private var width: CGFloat = 0
    
    
    /// The height
    @State private var height: CGFloat = 0
    
    
    /// The height for animations
    @State private var heightForAnimations: CGFloat = 0
    
    
    /// The options height
    @State private var optionsHeight: CGFloat = 0
    
    
    /// The options height for animations
    @State private var optionsHeightForAnimations: CGFloat = 0
    
    
    /// If the layer options are being presented horizontally instead of vertically
    var isHorizontal: Bool = false
    
    
    /// If the layer options are being presented via the extended layers button
    @Binding var isPresentingLayers: Bool
    
    
    /// If the layers options, that might being presented via the extended layers button, should be temporarily hidden to not be weirdly overlapped by for example the mode options
    var shouldTemporarilyHideLayers: Bool
    
    
    /// The actual view
    var body: some View {
        ZStack {
            Button {
                isPresentingLayers.toggle()
            } label: {
                Label {
                    Text("layers")
                } icon: {
                    Image(systemName: "square.stack.3d.up.fill")
                        .padding(.bottom, 1)
                }
            }
            .buttonStyle(MapButtonStyle(isClear: isPresentingLayers))
            .background {
                GeometryReader { geometry in
                    if #available(iOS 16.0, *) {
                        Color.clear
                            .onAppear {
                                if isHorizontal {
                                    height = geometry.size.width
                                    width = geometry.size.height
                                } else {
                                    height = geometry.size.height
                                    width = geometry.size.width
                                }
                            }
                            .onGeometryChange(for: CGRect.self) { changedGeometry in
                                changedGeometry.frame(in: .global)
                            } action: { _ in
                                if isHorizontal {
                                    height = geometry.size.width
                                    width = geometry.size.height
                                } else {
                                    height = geometry.size.height
                                    width = geometry.size.width
                                }
                            }
                    } else {
                        Color.clear
                            .onAppear {
                                if isHorizontal {
                                    height = geometry.size.width
                                    width = geometry.size.height
                                } else {
                                    height = geometry.size.height
                                    width = geometry.size.width
                                }
                            }
                    }
                }
            }
        }
        .background(alignment: isHorizontal ? .trailing : .top) {
            ZStack(alignment: isHorizontal ? .trailing : .top) {
                if isPresentingLayers, !shouldTemporarilyHideLayers {
                    if isHorizontal {
                        HStack(spacing: 12) {
                            ForEach(Layer.allCases.reversed()) { layer in
                                MapLayersButton.LayerToggle(layer: layer)
                                    .frame(height: max(0, width - 8))
                            }
                        }
                        .padding(4)
                        .padding(.horizontal, 6)
                    } else {
                        VStack(spacing: 8) {
                            ForEach(Layer.allCases) { layer in
                                MapLayersButton.LayerToggle(layer: layer)
                                    .frame(width: max(0, width - 8))
                            }
                        }
                        .padding(4)
                        .padding(.vertical, 6)
                    }
                }
            }
            .background {
                GeometryReader { optionsGeometry in
                    if #available(iOS 16.0, *) {
                        Color.clear
                            .onAppear {
                                if isHorizontal {
                                    optionsHeight = optionsGeometry.size.width
                                } else {
                                    optionsHeight = optionsGeometry.size.height
                                }
                            }
                            .onGeometryChange(for: CGRect.self) { changedContentGeometry in
                                changedContentGeometry.frame(in: .global)
                            } action: { _ in
                                if isHorizontal {
                                    optionsHeight = optionsGeometry.size.width
                                } else {
                                    optionsHeight = optionsGeometry.size.height
                                }
                            }
                    } else {
                        Color.clear
                            .onAppear {
                                if isHorizontal {
                                    optionsHeight = optionsGeometry.size.width
                                } else {
                                    optionsHeight = optionsGeometry.size.height
                                }
                            }
                    }
                }
            }
            .frame(width: isHorizontal ? (optionsHeightForAnimations) : (width + 2), height: isHorizontal ? (width + 2) : (optionsHeightForAnimations), alignment: isHorizontal ? .trailing : .top)
            .padding(isHorizontal ? .trailing : .top, heightForAnimations)
            .frame(width: (isHorizontal ? height + optionsHeightForAnimations : nil), height: (isHorizontal ? nil : height + optionsHeightForAnimations), alignment: isHorizontal ? .trailing : .top)
            .background(alignment: isHorizontal ? .trailing : .top) {
                if isPresentingLayers, !shouldTemporarilyHideLayers {
                    if #unavailable(iOS 27, macOS 27) {
                        Capsule()
                            .stroke(Color.MapButtons.border, lineWidth: 1)
                            .background(alignment: .top) {
                                Capsule()
                                    .fill(EllipticalGradient(colors: [Color.MapButtons.backgroundGlow, Color.MapButtons.background]))
                            }
                            .shadow(radius: 2)
                            .foregroundStyle(Color.primary)
                            .compositingGroup()
                    } else {
                        Capsule()
                            .fill(Color.MapButtons.background.opacity(0.3))
                            .glassEffect(.regular, in: RoundedRectangle(cornerRadius: 28))
                            .shadow(radius: 2)
                    }
                }
            }
            .compositingGroup()
            .padding(4)
            .onTapGesture {
                // Trick to extend the safe area around it
            }
            .padding(-4)
            .contentShape(Rectangle())
            .onChange(of: optionsHeight) { changedOptionsHeight in
                withAnimation(.spring.speed(4)) {
                    optionsHeightForAnimations = changedOptionsHeight
                }
                withAnimation(.spring.speed(2.1)) {
                    heightForAnimations = (changedOptionsHeight == 0 ? 0 : height)
                }
            }
        }
    }
}
