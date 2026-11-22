import SwiftUI

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
  func mapPanelOverlay<Panel: View>(isPresented: Bool,
                                    isCovered: Bool = false,
                                    onFrameChange: @escaping (CGRect) -> Void = { _ in },
                                    @ViewBuilder panel: @escaping () -> Panel) -> some View {
    modifier(MapPanelOverlay(isPresented: isPresented, isCovered: isCovered, onFrameChange: onFrameChange,
                             panel: panel))
  }
}

private struct MapPanelOverlay<Panel: View>: ViewModifier {
  let isPresented: Bool
  let isCovered: Bool
  let onFrameChange: (CGRect) -> Void
  @ViewBuilder let panel: () -> Panel

  @Environment(\.accessibilityReduceMotion) private var reduceMotion
  @ScaledMetric(relativeTo: .body) private var controlSize = MapPanelStyle.controlSize
  @State private var area = CGRect.zero

  func body(content: Content) -> some View {
    content
      .overlayPreferenceValue(MapControlsFramesKey.self, alignment: .topLeading) { controlsFrames in
        ZStack(alignment: .topLeading) {
          if isPresented {
            panel()
              .clipShape(.rect(cornerRadius: MapPanelStyle.cornerRadius))
              .mapPanelBackground()
              .avoidingWindowControls(sizeToFit: true)
              .onGeometryChange(for: CGRect.self) { proxy in
                proxy.frame(in: .global)
              } action: { frame in
                onFrameChange(frame)
              }
              .padding(.horizontal, MapPanelStyle.padding)
              .padding(.top, topInset(alignedWith: controlsFrames))
              .padding(.bottom, bottomInset(alignedWith: controlsFrames))
              .opacity(isCovered ? 0 : 1)
              .allowsHitTesting(!isCovered)
              .disabled(isCovered)
              .accessibilityHidden(isCovered)
              .transition(reduceMotion ? AnyTransition.opacity : .move(edge: .leading))
          }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
        .onGeometryChange(for: CGRect.self) { proxy in
          proxy.frame(in: .global)
        } action: { frame in
          area = frame
        }
      }
      .animation(reduceMotion ? .default : .spring, value: isPresented)
      .animation(reduceMotion ? .default : .spring, value: isCovered)
  }

  private func topInset(alignedWith controlsFrames: MapControlsFrames) -> CGFloat {
    guard let top = controlsFrames.top else { return MapPanelStyle.padding }
    return max(0, top.midY - area.minY - MapPanelStyle.headerTopPadding - controlSize / 2)
  }

  private func bottomInset(alignedWith controlsFrames: MapControlsFrames) -> CGFloat {
    guard let bottom = controlsFrames.bottom else { return MapPanelStyle.padding }
    return max(0, area.maxY - bottom.maxY)
  }
}
