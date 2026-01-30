import SwiftUI

/// View for the about information
struct RegionView: View {
    // MARK: Properties
    
    /// If the FAQ should be shown in the Safari view
    var region: Region
    
    var isOnlyShowingDownloaded = false
    
    
    /// The current date
    @State var state: Region.State = .notDownloaded
    
    
    /// The time to refresh the current date
    private let timer = Timer.publish(every: 0.1, on: .current, in: .common).autoconnect()
    
    
    
    /// The actual view
    var body: some View {
        Button {
            if state == .downloadedButOutdated || state == .downloadedButError {
                RegionalManager.update(for: region.id)
            } else if state == .notDownloaded || state == .notDownloadedAndError {
                RegionalManager.download(for: region.id)
            } else if case .downloading(_) = state {
                RegionalManager.cancel(for: region.id)
            }
        } label: {
            HStack {
                Image(systemName: "arrow.trianglehead.2.clockwise.rotate.90")
                    .hidden()
                    .overlay {
                        if state == .notDownloaded, region.hasSubregions, region.hasDownloads {
                            Image(systemName: "checkmark.circle.dotted")
                                .foregroundStyle(Color.BaseColors.green)
                        } else if state == .notDownloaded {
                            Image(systemName: "arrow.down.to.line")
                                .foregroundStyle(.secondary)
                        } else if state == .notDownloadedAndError {
                            Image(systemName: "exclamationmark.circle")
                                .foregroundStyle(Color.BaseColors.red)
                        } else if case .downloading(let progress) = state {
                            if progress == 0 {
                                ProgressView()
                                    .foregroundStyle(.secondary)
                            } else {
                                ProgressView(value: progress, total: 1)
                                    .progressViewStyle(DonutProgressStyle())
                                    .tint(Color.BaseColors.green)
                            }
                        } else if state == .downloaded {
                            Image(systemName: "checkmark.circle")
                                .foregroundStyle(Color.BaseColors.green)
                        } else if state == .downloadedButOutdated {
                            Image(systemName: "arrow.trianglehead.2.clockwise.rotate.90")
                                .foregroundStyle(Color.BaseColors.yellow)
                        } else if state == .downloadedButError {
                            Image(systemName: "exclamationmark.arrow.trianglehead.2.clockwise.rotate.90")
                                .foregroundStyle(Color.BaseColors.red)
                        }
                    }
                
                VStack(alignment: .leading) {
                    Text(region.name)
                        .bold()
                    
                    if region.hasSubregions {
                        Text("\(region.numberOfTotalDownloadedSubregions) of \(region.numberOfTotalSubregions)")
                            .foregroundStyle(.secondary)
                            .font(.caption2)
                    } else if let description = region.description {
                        Text(description)
                            .foregroundStyle(.secondary)
                            .font(.caption2)
                    }
                }
                
                Spacer()
                
                if #available(iOS 16.0, *) {
                    if isOnlyShowingDownloaded, let size = region.downloadedSize {
                        Text(size, format: .byteCount(style: .file))
                            .foregroundStyle(.secondary)
                            .font(.caption)
                    } else if let size = region.downloadingSize {
                        Text(size, format: .byteCount(style: .file))
                            .foregroundStyle(.secondary)
                            .font(.caption)
                    }
                }
            }
        }
        .foregroundStyle(.primary)
        .id(state)
        .onAppear {
            state = region.state
        }
        .onReceive(timer) {_ in
            state = region.state
        }
    }
}
