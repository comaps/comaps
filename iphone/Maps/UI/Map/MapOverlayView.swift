import SwiftUI

/// The view for all the map overlays
struct MapOverlayView: View {
    // MARK: Properties
    
    /// The horizontal size class of the environment
    @Environment(\.horizontalSizeClass) private var horizontalSizeClass
    
    
    /// The vertical size class of the environment
    @Environment(\.verticalSizeClass) private var verticalSizeClass
    
    
    /// The width
    @State private var width: CGFloat = 0
    
    
    /// The height
    @State private var height: CGFloat = 0
    
    
    /// The height used by the window controls
    @State private var windowControlsHeight: CGFloat = 0
    
    
    /// The default height of a map control
    @State private var controlHeight: CGFloat = 0
    
    
    /// If the top map controls are being displayed
    @State private var hasTopControls: Bool = false
    
    
    /// The height of the top map controls
    @State private var topControlsHeight: CGFloat = 0
    
    
    /// If the bottom map controls are being displayed
    @State private var hasBottomControls: Bool = false
    
    
    /// The height of the bottom map controls
    @State private var bottomControlsHeight: CGFloat = 0
    
    
    /// If the zoom buttons are being displayed
    @State private var hasZoomButtons: Bool = false
    
    
    /// If it is horizontally too tight to show the extended layers button
    private var isHorizontallyTooTight: Bool {
        return width < (11.6 * controlHeight)
    }
    
    
    /// If it is vertically too tight to show the extended layers button
    private var isVerticallyTooTight: Bool {
        return hasZoomButtons && height < (10.7 * controlHeight)
    }
    
    
    /// If the layer options are being presented via the extended layers button
    @AppStorage("IsPresentingMapLayers") private var isPresentingLayers: Bool = false
    
    
    /// If the layers options, that might being presented via the extended layers button, should be temporarily hidden, if necessary, to not be weirdly overlapped by for example the mode options
    @State private var shouldTemporarilyHideLayersIfNecessary: Bool = false
    
    
    /// If the layers options, that might being presented via the extended layers button, should be temporarily hidden to not be weirdly overlapped by for example the mode options
    private var shouldTemporarilyHideLayers: Bool {
        return shouldTemporarilyHideLayersIfNecessary && ((horizontalSizeClass == .compact && verticalSizeClass != .compact) || (isVerticallyTooTight && width < (14.3 * controlHeight)))
    }
    
    /// If the settings are being presented
    @Binding var isPresentingSettings: Bool
    
    
    /// If the app information is being presented
    @Binding var isPresentingInformation: Bool
    
    
    /// If the search is being presented externally
    @AppStorage("IsSearchPresented") private var isSearchPresented: Bool = false
    
    
    /// A necessary adjusmented need for the externally presented search
    @AppStorage("SearchAdjustment") private var searchAdjustment: Double = 0
    
    
    /// If the keyboard is being presented externally
    @State private var isKeyboardPresented: Bool = false
    
    
    /// The publisher for receiving the updates on the user defaults
    private let userDefaultsDidChangeNotificationPublisher = NotificationCenter.default.publisher(for: UserDefaults.didChangeNotification)
    
    
    /// The publisher for receiving the updates on the keyboard being shown
    private let keyboardWillShowNotificationPublisher = NotificationCenter.default.publisher(for: UIResponder.keyboardWillShowNotification)
    
    
    /// The publisher for receiving the updates on the keyboard being hidden
    private let keyboardWillHideNotificationPublisher = NotificationCenter.default.publisher(for: UIResponder.keyboardWillHideNotification)
    
    
    /// The actual view
    var body: some View {
        GeometryReader { geometry in
            VStack {
                ZStack(alignment: .top) {
                    if hasTopControls {
                        HStack {
                            MapMoreButton()
                                .frame(width: controlHeight)
                            
                            Spacer(minLength: 0)
                            
                            MapLayersButton(isHorizontal: isVerticallyTooTight && !isHorizontallyTooTight, isPresentingLayers: $isPresentingLayers, shouldTemporarilyHideLayers: shouldTemporarilyHideLayers)
                                .frame(width: controlHeight)
                        }
                        .frame(height: controlHeight)
                        .fixedSize(horizontal: false, vertical: true)
                        
                        MapModePicker(shouldTemporarilyHideLayersIfNecessary: $shouldTemporarilyHideLayersIfNecessary)
                            .frame(height: controlHeight)
                            .background(alignment: .top) {
                                MapTrackRecordingIndicator()
                                    .frame(height: controlHeight)
                                    .padding(.top, controlHeight)
                            }
                    }
                }
                .background {
                    GeometryReader { topControlsGeometry in
                        if #available(iOS 16.0, *) {
                            Color.black
                                .hidden()
                                .onAppear {
                                    topControlsHeight = topControlsGeometry.size.height
                                }
                                .onGeometryChange(for: CGRect.self) { changedTopControlsGeometry in
                                    changedTopControlsGeometry.frame(in: .global)
                                } action: { _ in
                                    topControlsHeight = topControlsGeometry.size.height
                                }
                                .onReceive(userDefaultsDidChangeNotificationPublisher) { _ in
                                    topControlsHeight = topControlsGeometry.size.height
                                }
                        } else {
                            Color.black
                                .hidden()
                                .onAppear {
                                    topControlsHeight = topControlsGeometry.size.height
                                }
                                .onReceive(userDefaultsDidChangeNotificationPublisher) { _ in
                                    topControlsHeight = topControlsGeometry.size.height
                                }
                        }
                    }
                }
                
                Spacer(minLength: 0)
                
                ZStack(alignment: .bottom) {
                    if hasBottomControls {
                        HStack {
                            MapCustomButton()
                                .frame(width: controlHeight)
                            
                            Spacer(minLength: 0)
                            
                            MapPositionButton()
                                .frame(width: controlHeight)
                        }
                        .frame(height: controlHeight)
                        .fixedSize(horizontal: false, vertical: true)
                        
                        if !isSearchPresented {
                            MapSearchButton()
                                .frame(height: controlHeight)
                        }
                    }
                }
                .background {
                    GeometryReader { bottomControlsGeometry in
                        if #available(iOS 16.0, *) {
                            Color.black
                                .hidden()
                                .onAppear {
                                    bottomControlsHeight = bottomControlsGeometry.size.height
                                }
                                .onGeometryChange(for: CGRect.self) { changedBottomControlsGeometry in
                                    changedBottomControlsGeometry.frame(in: .global)
                                } action: { _ in
                                    bottomControlsHeight = bottomControlsGeometry.size.height
                                }
                                .onReceive(userDefaultsDidChangeNotificationPublisher) { _ in
                                    bottomControlsHeight = bottomControlsGeometry.size.height
                                }
                        } else {
                            Color.black
                                .hidden()
                                .onAppear {
                                    bottomControlsHeight = bottomControlsGeometry.size.height
                                }
                                .onReceive(userDefaultsDidChangeNotificationPublisher) { _ in
                                    bottomControlsHeight = bottomControlsGeometry.size.height
                                }
                        }
                    }
                }
            }
            .frame(maxWidth: .infinity)
            .background(alignment: isHorizontallyTooTight && isVerticallyTooTight ? .bottomTrailing : .trailing) {
                if hasZoomButtons {
                    MapZoomButtons(maxDragHeight: height)
                        .frame(width: controlHeight, height: 24 + (2 * controlHeight))
                        .padding(.bottom, isHorizontallyTooTight && isVerticallyTooTight ? (bottomControlsHeight + 24) : 0)
                }
            }
            .padding(.top, geometry.safeAreaInsets.top == 0 ? (16 + windowControlsHeight) : windowControlsHeight)
            .padding(.bottom, geometry.safeAreaInsets.bottom == 0 ? 16 : (isKeyboardPresented ? 16 : 0))
            .padding(.leading, geometry.safeAreaInsets.leading == 0 ? (16 + searchAdjustment) : searchAdjustment)
            .padding(.trailing, geometry.safeAreaInsets.trailing == 0 ? 16 : 0)
            .animation(.spring, value: isSearchPresented)
            .animation(.spring, value: searchAdjustment)
            .animation(.spring.speed(7), value: isHorizontallyTooTight)
            .animation(.spring.speed(7), value: isVerticallyTooTight)
            .onAppear {
                updateControlVisbility(with: geometry, shouldAnimate: false)
            }
            .onChange(of: topControlsHeight) { _ in
                updatePadding(with: geometry)
            }
            .onChange(of: bottomControlsHeight) { _ in
                updatePadding(with: geometry)
            }
            .onReceive(userDefaultsDidChangeNotificationPublisher) { _ in
                updateControlVisbility(with: geometry)
            }
            .onReceive(keyboardWillShowNotificationPublisher) { _ in
                isKeyboardPresented = true
            }
            .onReceive(keyboardWillHideNotificationPublisher) { _ in
                isKeyboardPresented = false
            }
            .background {
                if #available(iOS 16.0, *) {
                    Color.black
                        .hidden()
                        .onGeometryChange(for: CGRect.self) { changedGeometry in
                            changedGeometry.frame(in: .global)
                        } action: { _ in
                            updatePadding(with: geometry)
                        }
                }
                
                Image(systemName: "circle")
                    .font(.title2)
                    .padding(13)
                    .hidden()
                    .background {
                        GeometryReader { controlGeometry in
                            if #available(iOS 16.0, *) {
                                Color.black
                                    .hidden()
                                    .onAppear {
                                        controlHeight = controlGeometry.size.height
                                    }
                                    .onGeometryChange(for: CGRect.self) { changedControlGeometry in
                                        changedControlGeometry.frame(in: .global)
                                    } action: { _ in
                                        controlHeight = controlGeometry.size.height
                                    }
                            } else {
                                Color.black
                                    .hidden()
                                    .onAppear {
                                        controlHeight = controlGeometry.size.height
                                    }
                            }
                        }
                    }
            }
        }
    }
    
    
    
    // MARK: Methods
    
    /// Update the visibility of the controls
    /// - Parameter geometry: The geometry of the view
    /// - Parameter shouldAnimate: If the update should be animated
    func updateControlVisbility(with geometry: GeometryProxy, shouldAnimate: Bool = true) {
        if shouldAnimate {
            withAnimation(.spring.speed(1)) {
                hasTopControls = !MapControls.areMapControlsHidden
                hasBottomControls = !MapControls.areMapControlsHidden
                hasZoomButtons = !MapControls.areMapZoomButtonsHidden && Settings.hasZoomButtons
                
                updatePadding(with: geometry)
            }
        } else {
            hasTopControls = !MapControls.areMapControlsHidden
            hasBottomControls = !MapControls.areMapControlsHidden
            hasZoomButtons = !MapControls.areMapZoomButtonsHidden && Settings.hasZoomButtons
            
            updatePadding(with: geometry)
        }
    }
    
    
    /// Update the paddings
    /// - Parameter geometry: The geometry of the view
    func updatePadding(with geometry: GeometryProxy) {
        width = geometry.size.width
        height = geometry.size.height
        if #available(iOS 26.0, *) {
            windowControlsHeight = geometry.containerCornerInsets.topLeading.height
        }
        
        let top = windowControlsHeight + topControlsHeight + (geometry.safeAreaInsets.top == 0 ? 16 : geometry.safeAreaInsets.top) + 4
        let bottom = bottomControlsHeight + (geometry.safeAreaInsets.bottom == 0 ? 16 : geometry.safeAreaInsets.bottom)
        let leading = (geometry.safeAreaInsets.leading == 0 ? (16 + searchAdjustment) : (geometry.safeAreaInsets.leading + searchAdjustment))
        let trailing = (geometry.safeAreaInsets.trailing == 0 ? 16 : geometry.safeAreaInsets.trailing)
        
        MWMMapWidgetsHelper.updatePadding(forTop: top, bottom: bottom, leading: leading, trailing: trailing)
    }
}
