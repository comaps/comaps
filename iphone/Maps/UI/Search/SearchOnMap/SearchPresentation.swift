import SwiftUI

extension EnvironmentValues {
  @Entry var isSearchPresented = false
  @Entry var searchPanelInset: CGFloat = 0
}

extension View {
  func searchPresentation(isSuspended: Bool) -> some View {
    modifier(SearchPresentation(isSuspended: isSuspended))
  }
}

private struct SearchPresentation: ViewModifier {
  let isSuspended: Bool

  @Environment(\.horizontalSizeClass) private var horizontalSizeClass
  @Environment(\.verticalSizeClass) private var verticalSizeClass
  @ScaledMetric(relativeTo: .body) private var preferredPanelWidth = MapPanelStyle.width
  @State private var containerWidth: CGFloat = 0

  private let model = SearchModel.shared

  private var layout: SearchOnMap.Layout {
    horizontalSizeClass == .compact && verticalSizeClass == .regular ? .sheet : .panel
  }

  private var panelWidth: CGFloat {
    guard containerWidth > 0 else { return MapPanelStyle.width }
    return min(preferredPanelWidth, containerWidth * MapPanelStyle.maxWidthFraction)
  }

  private var panelInset: CGFloat {
    model.isPanelPresented ? panelWidth + 2 * MapPanelStyle.padding : 0
  }

  func body(content: Content) -> some View {
    @Bindable var model = model
    content
      .environment(model)
      .environment(\.isSearchPresented, model.isOpen && !model.isCollapsed)
      .environment(\.searchPanelInset, panelInset)
      .environment(\.mapPanelWidth, panelWidth)
      .onGeometryChange(for: CGFloat.self) { proxy in
        proxy.size.width
      } action: { width in
        containerWidth = width
      }
      .mapPanelOverlay(isPresented: model.isPanelPresented) {
        SearchView(model: model, placement: .panel)
          .frame(width: panelWidth)
      }
      .sheet(isPresented: $model.isSheetPresented, onDismiss: { model.isSheetOnScreen = false }) {
        SearchView(model: model, placement: .sheet)
          .onAppear {
            model.isSheetOnScreen = true
          }
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
