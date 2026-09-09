extension AccessibilityNodeContext {
    borrowing func GetNodeInfo() -> AccessibilityNodeInfo {
      return __GetNodeInfoUnsafe().pointee
    }
    
    borrowing func GetBounds() -> RectD {
      return __GetBoundsUnsafe().pointee
    }
}
