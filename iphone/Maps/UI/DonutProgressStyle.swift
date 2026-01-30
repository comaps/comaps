import SwiftUI




/// A custom style for a `ProgressView`, that shows progress as a donut
struct DonutProgressStyle: ProgressViewStyle {
    // MARK: - Methods
    
    /// Creates a body with the given progress ciew style configuration
    /// - Parameter configuration: A progress ciew style configuration
    /// - Returns: A body
    func makeBody(configuration: Configuration) -> some View {
        let fractionCompleted = configuration.fractionCompleted ?? 0
        
        return Image(systemName: "circle")
            .foregroundStyle(Color.secondary.opacity(0.1))
            .overlay {
                Image(systemName: "circle")
                    .foregroundStyle(TintShapeStyle())
                    .clipShape(Pie(degrees: fractionCompleted * 360))
                .rotationEffect(.degrees(-90))
            }
            .compositingGroup()
    }
}
