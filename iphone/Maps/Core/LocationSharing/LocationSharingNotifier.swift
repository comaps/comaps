import Foundation
import UIKit
import UserNotifications
import CoreLocation

/// Manages notifications for location sharing
@objc class LocationSharingNotifier: NSObject {

  @objc static let shared = LocationSharingNotifier()

  private let notificationCenter = UNUserNotificationCenter.current()
  private var reminderTimer: Timer?

  private static let activeNotificationId = "location_sharing_active"
  private static let reminderNotificationId = "location_sharing_reminder"
  private static let categoryId = "LOCATION_SHARING"

  private override init() {
    super.init()
    setupNotificationCategories()
  }

  // MARK: - Setup

  private func setupNotificationCategories() {
    // Define stop action
    let stopAction = UNNotificationAction(
      identifier: "STOP_SHARING",
      title: "Stop Sharing",
      options: [.destructive])

    // Define category
    let category = UNNotificationCategory(
      identifier: Self.categoryId,
      actions: [stopAction],
      intentIdentifiers: [],
      options: [])

    notificationCenter.setNotificationCategories([category])
  }

  // MARK: - Authorization

  func requestAuthorization(completion: @escaping (Bool) -> Void) {
    notificationCenter.requestAuthorization(options: [.alert, .sound, .badge]) { granted, error in
      if let error = error {
        NSLog("Notification authorization error: \(error)")
      }
      completion(granted)
    }
  }

  // MARK: - Active Notification

  /// Schedule persistent notification while sharing is active
  func scheduleActiveNotification() {
    requestAuthorization { [weak self] granted in
      guard granted else { return }
      self?.showActiveNotification()
    }

    // Schedule reminder notifications every 10 minutes
    scheduleReminderNotifications()
  }

  private func showActiveNotification() {
    let content = UNMutableNotificationContent()
    content.title = "Location Sharing Active"
    content.body = "Your live location is being shared. Tap to view or stop."
    content.sound = nil // Silent notification
    content.categoryIdentifier = Self.categoryId

    // Add badge
    content.badge = 1

    let request = UNNotificationRequest(
      identifier: Self.activeNotificationId,
      content: content,
      trigger: nil) // Immediate

    notificationCenter.add(request) { error in
      if let error = error {
        NSLog("Failed to schedule active notification: \(error)")
      }
    }
  }

  /// Update notification with current location info
  func updateNotification(location: CLLocation) {
    let content = UNMutableNotificationContent()
    content.title = "Location Sharing Active"

    // Format accuracy
    let accuracyText = formatAccuracy(location.horizontalAccuracy)
    content.body = "Sharing your location (accuracy: \(accuracyText))"

    content.sound = nil
    content.categoryIdentifier = Self.categoryId
    content.badge = 1

    let request = UNNotificationRequest(
      identifier: Self.activeNotificationId,
      content: content,
      trigger: nil)

    notificationCenter.add(request)
  }

  // MARK: - Reminder Notifications

  private func scheduleReminderNotifications() {
    // Cancel existing reminders
    notificationCenter.removePendingNotificationRequests(withIdentifiers: [Self.reminderNotificationId])

    // Schedule timer to show reminder every 10 minutes
    reminderTimer?.invalidate()
    reminderTimer = Timer.scheduledTimer(
      withTimeInterval: 600, // 10 minutes
      repeats: true) { [weak self] _ in
      self?.showReminderNotification()
    }
  }

  private func showReminderNotification() {
    let content = UNMutableNotificationContent()
    content.title = "Location Sharing Reminder"
    content.body = "Your location is still being shared. Tap to stop if needed."
    content.sound = .default
    content.categoryIdentifier = Self.categoryId

    let request = UNNotificationRequest(
      identifier: Self.reminderNotificationId,
      content: content,
      trigger: nil)

    notificationCenter.add(request) { error in
      if let error = error {
        NSLog("Failed to schedule reminder notification: \(error)")
      }
    }
  }

  // MARK: - Cancel

  func cancelNotifications() {
    // Cancel all location sharing notifications
    notificationCenter.removePendingNotificationRequests(
      withIdentifiers: [Self.activeNotificationId, Self.reminderNotificationId])

    notificationCenter.removeDeliveredNotifications(
      withIdentifiers: [Self.activeNotificationId, Self.reminderNotificationId])

    // Stop reminder timer
    reminderTimer?.invalidate()
    reminderTimer = nil

    // Clear badge
    UIApplication.shared.applicationIconBadgeNumber = 0
  }

  // MARK: - Helpers

  private func formatAccuracy(_ accuracy: Double) -> String {
    if accuracy < 10 {
      return "high"
    } else if accuracy < 50 {
      return "medium"
    } else {
      return "low"
    }
  }
}

// MARK: - Live Activities (iOS 16.1+)

#if canImport(ActivityKit)
import ActivityKit

@available(iOS 16.1, *)
extension LocationSharingNotifier {

  /// Start Live Activity for location sharing
  func startLiveActivity() {
    // TODO: Implement Live Activity
    // This would show persistent UI on lock screen and Dynamic Island
    // Requires defining ActivityAttributes and ActivityConfiguration
    NSLog("Live Activity support would be implemented here for iOS 16.1+")
  }

  /// Update Live Activity with current location
  func updateLiveActivity(location: CLLocation, eta: TimeInterval?, distance: Int?) {
    // TODO: Update Live Activity content
  }

  /// End Live Activity
  func endLiveActivity() {
    // TODO: End Live Activity
  }
}
#endif
