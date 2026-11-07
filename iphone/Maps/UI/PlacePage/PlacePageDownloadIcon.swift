import SwiftUI

struct PlacePageDownloadIcon: View {
  let status: MapNodeStatus?
  let progress: Double?

  @ScaledMetric(relativeTo: .title3) private var size: CGFloat = 24

  var body: some View {
    switch status {
    case .downloading:
      ZStack {
        Circle()
          .stroke(.tint.opacity(0.25), lineWidth: 2.5)
        Circle()
          .trim(from: 0, to: progress ?? 0)
          .stroke(.tint, style: StrokeStyle(lineWidth: 2.5, lineCap: .round))
          .rotationEffect(.degrees(-90))
          .animation(.linear, value: progress)
        Image(systemName: "stop.fill")
          .font(.system(size: size * 0.35))
      }
      .frame(width: size, height: size)
    case .applying, .inQueue:
      ProgressView()
    case .error:
      Image(systemName: "exclamationmark.arrow.circlepath")
    default:
      PlacePageActionImage(name: "ic_download")
    }
  }
}
