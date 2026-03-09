import SwiftUI

/// View for the about information for CoMaps (split up in its own view because of differences between OS versions)
struct ApoutOpenStreetMapView: View {
    // MARK: Properties
    
    /// The date fo the maps
    private var mapsDate: String? {
        let dateFormatter = DateFormatter()
        dateFormatter.locale = Locale(identifier: "en_US_POSIX")
        dateFormatter.dateFormat = "yyMMdd"
        if let date = dateFormatter.date(from: String(FrameworkHelper.dataVersion())) {
            dateFormatter.locale = Locale.autoupdatingCurrent
            dateFormatter.dateStyle = .long
            dateFormatter.timeStyle = .none
            return dateFormatter.string(from: date)
        }
        
        return nil
    }
    
    
    /// The actual view
    var body: some View {
        if let mapsDate {
            VStack(alignment: .leading) {
                HStack(alignment: .top, spacing: 12) {
                    Image(.openStreetMapLogo)
                        .resizable()
                        .aspectRatio(1, contentMode: .fit)
                        .frame(maxWidth: 50)
                        .padding(.top, 6)
                    VStack(alignment: .leading) {
                        Text("osm_mapdata")
                            .font(.headline)
                            .bold()
                        
                        Text("osm_mapdata_explanation \(mapsDate)")
                            .tint(.alternativeAccent)
                    }
                }
            }
        }
    }
}
