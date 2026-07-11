import XCTest
@testable import CoMaps__Debug_

private final class TestObserver: NSObject, MWMIndoorObserver {
  var updateCount = 0
  func onIndoorLevelsUpdated() {
    updateCount += 1
  }
}

final class IndoorManagerTests: XCTestCase {

  func testInitialStateIsEmpty() {
    XCTAssertTrue(IndoorManager.levels().isEmpty)
  }

  func testSelectInvalidLevelIsIgnored() {
    let activeBefore = IndoorManager.activeLevel()
    IndoorManager.selectLevel("penthouse")
    XCTAssertEqual(IndoorManager.activeLevel(), activeBefore)
  }

  func testObserverAddRemove() {
    let observer = TestObserver()
    IndoorManager.add(observer)
    IndoorManager.remove(observer)
    // No crash and no spurious notifications.
    XCTAssertEqual(observer.updateCount, 0)
  }

  func testDeallocatedObserverIsSafe() {
    autoreleasepool {
      let observer = TestObserver()
      IndoorManager.add(observer)
      // Observer goes out of scope; the weak table must not retain or crash.
    }
    // Accessing the manager after the observer deallocated must be safe.
    XCTAssertTrue(IndoorManager.levels().isEmpty)
  }
}
