import SwiftUI
import WidgetKit

/// Watch-face complication: a CoMaps glyph that opens the watch app in one
/// tap. Static for now — showing live route data here needs an App Group
/// shared container between the app and this extension.
struct LauncherEntry: TimelineEntry {
  let date: Date
}

struct LauncherProvider: TimelineProvider {
  func placeholder(in context: Context) -> LauncherEntry { LauncherEntry(date: Date()) }

  func getSnapshot(in context: Context, completion: @escaping (LauncherEntry) -> Void) {
    completion(LauncherEntry(date: Date()))
  }

  func getTimeline(in context: Context, completion: @escaping (Timeline<LauncherEntry>) -> Void) {
    completion(Timeline(entries: [LauncherEntry(date: Date())], policy: .never))
  }
}

struct LauncherView: View {
  @Environment(\.widgetFamily) private var family
  var entry: LauncherEntry

  var body: some View {
    if family == .accessoryInline {
      Text("CoMaps")
    } else {
      Image(systemName: "map.fill")
        .font(.title3)
        .widgetAccentable()
    }
  }
}

@main
struct CoMapsWatchWidget: Widget {
  var body: some WidgetConfiguration {
    StaticConfiguration(kind: "app.comaps.watch.launcher", provider: LauncherProvider()) { entry in
      LauncherView(entry: entry)
        .containerBackground(for: .widget) { Color.clear }
    }
    .configurationDisplayName("CoMaps")
    .description("Opens the route glance.")
    .supportedFamilies([.accessoryCircular, .accessoryCorner, .accessoryInline])
  }
}
