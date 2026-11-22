import SwiftUI

extension EnvironmentValues {
  @Entry var isSearchPresented = false
  @Entry var searchPanelInset: CGFloat = 0
}

struct MapControlsFrames: Equatable {
  var top: CGRect?
  var bottom: CGRect?
}

struct MapControlsFramesKey: PreferenceKey {
  static let defaultValue = MapControlsFrames()

  static func reduce(value: inout MapControlsFrames, nextValue: () -> MapControlsFrames) {
    let next = nextValue()
    value.top = next.top ?? value.top
    value.bottom = next.bottom ?? value.bottom
  }
}

extension View {
  func searchPresentation(isSuspended: Bool) -> some View {
    modifier(SearchPresentation(isSuspended: isSuspended))
  }
}

private struct SearchPresentation: ViewModifier {
  private enum Constants {
    static let panelWidth: CGFloat = 350
    static let panelMaxWidthFraction: CGFloat = 0.5
    static let panelPadding: CGFloat = 8
    static let panelCornerRadius: CGFloat = 16
  }

  let isSuspended: Bool

  @Environment(\.horizontalSizeClass) private var horizontalSizeClass
  @Environment(\.verticalSizeClass) private var verticalSizeClass
  @Environment(\.accessibilityReduceMotion) private var reduceMotion
  @ScaledMetric(relativeTo: .body) private var preferredPanelWidth = Constants.panelWidth
  @ScaledMetric(relativeTo: .body) private var fieldHeight = SearchHeader.controlHeight
  @State private var containerWidth: CGFloat = 0
  @State private var panelArea = CGRect.zero

  private let model = SearchModel.shared

  private var layout: SearchOnMap.Layout {
    horizontalSizeClass == .compact && verticalSizeClass == .regular ? .sheet : .panel
  }

  private var panelWidth: CGFloat {
    guard containerWidth > 0 else { return Constants.panelWidth }
    return min(preferredPanelWidth, containerWidth * Constants.panelMaxWidthFraction)
  }

  private func panelTopInset(alignedWith controlsFrames: MapControlsFrames) -> CGFloat {
    guard let top = controlsFrames.top else { return Constants.panelPadding }
    return max(0, top.midY - panelArea.minY - SearchHeader.panelTopPadding - fieldHeight / 2)
  }

  private func panelBottomInset(alignedWith controlsFrames: MapControlsFrames) -> CGFloat {
    guard let bottom = controlsFrames.bottom else { return Constants.panelPadding }
    return max(0, panelArea.maxY - bottom.maxY)
  }

  private var panelInset: CGFloat {
    model.isPanelPresented ? panelWidth + 2 * Constants.panelPadding : 0
  }

  func body(content: Content) -> some View {
    @Bindable var model = model
    content
      .environment(model)
      .environment(\.isSearchPresented, model.isOpen && !model.isCollapsed)
      .environment(\.searchPanelInset, panelInset)
      .onGeometryChange(for: CGFloat.self) { proxy in
        proxy.size.width
      } action: { width in
        containerWidth = width
      }
      .overlayPreferenceValue(MapControlsFramesKey.self, alignment: .topLeading) { controlsFrames in
        ZStack(alignment: .topLeading) {
          if model.isPanelPresented {
            panel(topInset: panelTopInset(alignedWith: controlsFrames),
                  bottomInset: panelBottomInset(alignedWith: controlsFrames))
              .transition(reduceMotion ? AnyTransition.opacity : .move(edge: .leading))
          }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
        .onGeometryChange(for: CGRect.self) { proxy in
          proxy.frame(in: .global)
        } action: { frame in
          panelArea = frame
        }
      }
      .animation(reduceMotion ? .default : .spring, value: model.isPanelPresented)
      .sheet(isPresented: $model.isSheetPresented) {
        SearchView(model: model, placement: .sheet)
          .presentationDetents([.medium, .large], selection: $model.detent)
          .presentationBackgroundInteraction(.enabled(upThrough: .medium))
          .presentationDragIndicator(.visible)
      }
      .onChange(of: layout, initial: true) {
        model.layout = layout
      }
      .onChange(of: isSuspended, initial: true) {
        model.isSuspended = isSuspended
      }
      .onChange(of: panelInset, initial: true) {
        MapViewController.shared()?.setSearchPanelInset(panelInset)
      }
  }

  private func panel(topInset: CGFloat, bottomInset: CGFloat) -> some View {
    SearchView(model: model, placement: .panel)
      .frame(width: panelWidth)
      .clipShape(.rect(cornerRadius: Constants.panelCornerRadius))
      .panelBackground(cornerRadius: Constants.panelCornerRadius)
      .avoidingWindowControls(sizeToFit: true)
      .padding(.horizontal, Constants.panelPadding)
      .padding(.top, topInset)
      .padding(.bottom, bottomInset)
  }
}

private extension View {
  @ViewBuilder
  func panelBackground(cornerRadius: CGFloat) -> some View {
    if #available(iOS 26.0, *) {
      glassEffect(.regular, in: .rect(cornerRadius: cornerRadius))
    } else {
      background {
        RoundedRectangle(cornerRadius: cornerRadius)
          .fill(BackgroundStyle())
          .shadow(color: .black.opacity(0.15), radius: 8)
      }
    }
  }
}

private extension SearchModel {
  var detent: PresentationDetent {
    get { step.detent }
    set { didSelectStep(.init(newValue)) }
  }
}

private extension ModalPresentationStep {
  var detent: PresentationDetent {
    switch self {
    case .fullScreen:
      return .large
    case .halfScreen, .compact, .hidden:
      return .medium
    }
  }

  init(_ detent: PresentationDetent) {
    switch detent {
    case .large:
      self = .fullScreen
    default:
      self = .halfScreen
    }
  }
}
