import SwiftUI

extension View {
  func placePagePresentation(isSuspended: Bool) -> some View {
    modifier(PlacePagePresentation(isSuspended: isSuspended))
  }
}

private struct PlacePagePresentation: ViewModifier {
  let isSuspended: Bool

  @Environment(\.horizontalSizeClass) private var horizontalSizeClass
  @Environment(\.verticalSizeClass) private var verticalSizeClass
  @Environment(\.mapPanelWidth) private var panelWidth

  private let model = PlacePageModel.shared

  private var placement: PlacePageModel.Placement {
    horizontalSizeClass == .compact && verticalSizeClass == .regular ? .sheet : .panel
  }

  func body(content: Content) -> some View {
    @Bindable var model = model
    content
      .mapPanelOverlay(isPresented: model.isPanelPresented) {
        if let content = model.content {
          PlacePageView(content: content, placement: .panel)
            .id(ObjectIdentifier(content))
            .frame(width: panelWidth)
            .frame(maxHeight: model.contentHeight > 0 ? model.contentHeight : nil)
        }
      }
      .sheet(isPresented: $model.isSheetPresented, onDismiss: { model.sheetDidDisappear() }) {
        if let content = model.content {
          PlacePageView(content: content, placement: .sheet)
            .id(ObjectIdentifier(content))
            .onGeometryChange(for: CGFloat.self) { proxy in
              proxy.frame(in: .global).minY
            } action: { top in
              model.sheetTopDidChange(top)
            }
            .onAppear {
              model.sheetDidAppear()
            }
            .presentationDetents(model.detents, selection: $model.detent)
            .presentationBackgroundInteraction(.enabled(upThrough: .medium))
            .presentationDragIndicator(.visible)
        }
      }
      .onChange(of: placement, initial: true) {
        model.placement = placement
      }
      .onChange(of: isSuspended, initial: true) {
        model.isSuspended = isSuspended
      }
  }
}
