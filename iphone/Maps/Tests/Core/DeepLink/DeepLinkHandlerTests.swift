import XCTest
import UIKit
@testable import CoMaps__Debug_

final class DeepLinkHandlerTests: XCTestCase {
  private let handler = DeepLinkHandler.shared

  override func setUp() {
    super.setUp()
    handler.reset()
  }

  override func tearDown() {
    handler.reset()
    super.tearDown()
  }

  func testColdStartCustomSchemeLinkBecomesPending() throws {
    let deepLink = try XCTUnwrap(URL(string: "cm://s4jeUNcio8/Puttebo"))

    handler.applicationDidFinishLaunching([.url: deepLink])

    XCTAssertTrue(handler.hasPendingDeepLink)
    XCTAssertTrue(handler.isLaunchedByDeeplink)
    XCTAssertFalse(handler.isLaunchedByUniversalLink)
    XCTAssertEqual(handler.url, deepLink)
  }

  func testColdStartUniversalLinksAreConvertedAndBecomePending() throws {
    for scheme in ["http", "https"] {
      handler.reset()
      let universalLink = try XCTUnwrap(URL(string: "\(scheme)://comaps.at/s4jeUNcio8/Puttebo"))

      XCTAssertTrue(handler.applicationDidFinishLaunching(withUniversalLink: universalLink))

      XCTAssertTrue(handler.hasPendingDeepLink, scheme)
      XCTAssertFalse(handler.isLaunchedByDeeplink, scheme)
      XCTAssertTrue(handler.isLaunchedByUniversalLink, scheme)
      XCTAssertEqual(handler.url?.absoluteString, "cm://s4jeUNcio8/Puttebo", scheme)
    }
  }

  func testResetClearsPendingDeepLinkState() throws {
    let universalLink = try XCTUnwrap(URL(string: "https://comaps.at/s4jeUNcio8/Puttebo"))
    XCTAssertTrue(handler.applicationDidFinishLaunching(withUniversalLink: universalLink))

    handler.reset()

    XCTAssertFalse(handler.hasPendingDeepLink)
    XCTAssertFalse(handler.isLaunchedByDeeplink)
    XCTAssertFalse(handler.isLaunchedByUniversalLink)
    XCTAssertNil(handler.url)
  }
}
