import SwiftUI

/// View for the about information for CoMaps (split up in its own view because of differences between OS versions)
struct AboutCoMapsView: View {
    // MARK: Properties
    
    /// The actual view
    var body: some View {
        VStack(alignment: .leading) {
            HStack(alignment: .top, spacing: 12) {
                Image("comaps")
                    .resizable()
                    .aspectRatio(1, contentMode: .fit)
                    .frame(maxWidth: 50)
                    .padding(.top, 6)
                
                VStack(alignment: .leading) {
                    Text("about_headline")
                        .font(.headline)
                        .bold()
                    
                    VStack(alignment: .leading) {
                        HStack(alignment: .top, spacing: 4) {
                            Text(String("•"))
                            
                            Text("about_proposition_1")
                        }
                        
                        HStack(alignment: .top, spacing: 4) {
                            Text(String("•"))
                            
                            Text("about_proposition_2")
                        }
                        
                        HStack(alignment: .top, spacing: 4) {
                            Text(String("•"))
                            
                            Text("about_proposition_3")
                        }
                    }
                }
            }
        }
    }
}
