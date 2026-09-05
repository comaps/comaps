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
                let willPresentLayers = !isPresentingLayers
                isPresentingLayers = willPresentLayers
                updateLayerOptionsSize(isPresented: willPresentLayers)
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
                                    .frame(width: optionSize, height: optionSize)
                            }
                        }
                        .padding(4)
                        .padding(.horizontal, 6)
                    } else {
                        VStack(spacing: 8) {
                            ForEach(Layer.allCases) { layer in
                                MapLayersButton.LayerToggle(layer: layer)
                                    .frame(width: optionSize, height: optionSize)
                            }
                        }
                        .padding(4)
                        .padding(.vertical, 6)
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
                            .glassEffect(.regular, in: RoundedRectangle(cornerRadius: 28, style: .continuous))
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
            .onChange(of: width) { _ in
                updateLayerOptionsSize()
            }
            .onChange(of: height) { _ in
                updateLayerOptionsSize()
            }
            .onChange(of: shouldTemporarilyHideLayers) { _ in
                if shouldTemporarilyHideLayers {
                    isPresentingLayers = false
                }
                updateLayerOptionsSize()
            }
        }
    }


    /// The size of a layer option
    private var optionSize: CGFloat {
        max(0, width - 8)
    }


    /// The extent spacing occupied by all optons
    private var layerOptionsExtent: CGFloat {
        let count = CGFloat(Layer.allCases.count)
        let spacing = CGFloat(max(0, Layer.allCases.count - 1)) * (isHorizontal ? 12 : 8)
        return (count * optionSize) + spacing + 20
    }


    // MARK: Methods

    private func updateLayerOptionsSize(isPresented: Bool? = nil) {
        let isVisible = (isPresented ?? isPresentingLayers) && !shouldTemporarilyHideLayers
        let changedOptionsHeight = layerOptionsExtent


        if #available(iOS 16.0,*) {
            withAnimation(.spring.speed(4)) {
                optionsHeightForAnimations = isVisible ? changedOptionsHeight : 0
            }
        } else {
            // This animation doesn't work properly on ios 15
            optionsHeightForAnimations = isVisible ? changedOptionsHeight : 0
        }
        
        withAnimation(.spring.speed(2.1)) {
            heightForAnimations = isVisible ? height : 0
        }    }
}
