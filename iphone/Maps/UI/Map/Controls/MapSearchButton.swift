import SwiftUI

/// View for a map search button (later to be replaced by an actual textfield)
struct MapSearchButton: View {
    // MARK: Properties
    
    /// The vertical size class of the environment
    @Environment(\.verticalSizeClass) private var verticalSizeClass
    
    
    /// The color scheme of the environment
    @Environment(\.colorScheme) private var colorScheme
    
    
    /// The default height of a map control
    var controlHeight: CGFloat
    
    
    /// The actual view
    var body: some View {
        HStack(spacing: 0) {
            if verticalSizeClass != .compact {
                Spacer(minLength: 0)
            }
            
            HStack(spacing: 0) {
                ForEach(Mode.allCases) { mode in
                    Circle()
                        .fill(.clear)
                        .aspectRatio(1, contentMode: .fit)
                }
            }
            .padding(4)
            .accessibilityHidden(true)
            .overlay {
                if #available(iOS 17.0, *) {
                    CollapsedSearchField {
                        searchButton
                    }
                } else {
                    searchButton
                }
            }
            .background {
                if #unavailable(iOS 27, macOS 27) {
                    RoundedRectangle(cornerRadius: 28, style: .continuous)
                        .stroke(Color.MapButtons.border, lineWidth: 1)
                        .background {
                            if colorScheme == .dark {
                                RoundedRectangle(cornerRadius: 28, style: .continuous)
                                    .fill(Color.black)
                            }

                            RoundedRectangle(cornerRadius: 28, style: .continuous)
                                .fill(Color.white.opacity(colorScheme == .dark ? 0.25 : 1))
                        }
                        .shadow(radius: 2)
                        .foregroundStyle(Color.secondary)
                        .compositingGroup()
                } else {
                    RoundedRectangle(cornerRadius: 28, style: .continuous)
                        .fill(Color.MapButtons.background.opacity(0.3))
                        .glassEffect(.regular, in: RoundedRectangle(cornerRadius: 28, style: .continuous))
                        .shadow(radius: 2)
                }
            }
            .contentShape(Rectangle())
            .padding(.leading, verticalSizeClass == .compact ? (controlHeight + 24) : 0)
            
            Spacer(minLength: 0)
        }
    }
    
    
    private var searchButton: some View {
        Button {
            NotificationCenter.default.post(Notification(name: MapControls.presentSearchNotificationName))
        } label: {
            Label("search", systemImage: "magnifyingglass")
                .foregroundStyle(.secondary)
                .padding(.leading)
                .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .leading)
                .compositingGroup()
                .background(.white.opacity(0.01))
        }
        .buttonStyle(.plain)
        .dynamicTypeSize(...DynamicTypeSize.xxxLarge)
        .accessibilityShowsLargeContentViewer()
    }
}


@available(iOS 17.0, *)
private struct CollapsedSearchField<Fallback: View>: View {
    @Environment(SearchModel.self) private var model: SearchModel?
    @ViewBuilder let fallback: () -> Fallback
    
    
    var body: some View {
        if let model, model.isCollapsed {
            HStack(spacing: 0) {
                Button(action: model.expand) {
                    Label {
                        if model.text.isEmpty {
                            Text("search")
                                .foregroundStyle(.secondary)
                        } else {
                            Text(verbatim: model.text)
                                .foregroundStyle(.primary)
                        }
                    } icon: {
                        Image(systemName: "magnifyingglass")
                            .foregroundStyle(.secondary)
                    }
                    .lineLimit(1)
                    .padding(.leading)
                    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .leading)
                    .contentShape(Rectangle())
                }
                .buttonStyle(.plain)
                .accessibilityLabel(Text("search"))
                .accessibilityValue(model.text)
                .accessibilityActions {
                    if !model.text.isEmpty {
                        Button(L("search_results")) {
                            model.didSelectStep(.halfScreen)
                        }
                    }
                }
                .accessibilityShowsLargeContentViewer()
                
                Button(action: model.didTapClose) {
                    Image(systemName: "xmark.circle.fill")
                        .foregroundStyle(.secondary)
                        .padding(.horizontal)
                        .frame(maxHeight: .infinity)
                        .contentShape(Rectangle())
                }
                .buttonStyle(.plain)
                .accessibilityLabel(L("close"))
                .accessibilityShowsLargeContentViewer()
            }
            .overlay(alignment: .top) {
                Capsule()
                    .fill(.secondary)
                    .frame(width: 36, height: 5)
                    .padding(.top, 5)
                    .accessibilityHidden(true)
            }
            .highPriorityGesture(
                DragGesture(minimumDistance: 10)
                    .onEnded { value in
                        if value.translation.height < -30 || value.predictedEndTranslation.height < -80 {
                            model.didSelectStep(.halfScreen)
                        }
                    }
            )
            .dynamicTypeSize(...DynamicTypeSize.xxxLarge)
        } else {
            fallback()
        }
    }
}
