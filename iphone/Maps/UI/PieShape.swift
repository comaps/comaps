import SwiftUI




/// A custom pie shape
struct Pie: Shape {
    // MARK: - Properties
        
    /// The degrees
    nonisolated(unsafe) var degrees: Double
    
    
    
    /// The animatable data
    nonisolated var animatableData: Double {
        get { degrees }
        set { degrees = newValue }
    }
    
    
    
    
    // MARK: - Methods
    
    /// Creates a path in the given rectangle
    /// - Parameter rect: A rectangle
    /// - Returns: A path
    func path(in rect: CGRect) -> Path {
        var path = Path()
        path.move(to: CGPoint(x: rect.maxX/2, y: rect.maxY/2))
        path.addArc(center: CGPoint(x: rect.maxX/2, y: rect.maxY/2), radius: rect.width/2, startAngle: Angle(degrees: 0), endAngle: Angle(degrees: degrees), clockwise: false)
        return path
   }
}
