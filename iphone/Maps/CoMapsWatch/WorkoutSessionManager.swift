import Combine
import Foundation
import HealthKit

/// Records a workout during navigation. Besides showing up in Health, the
/// running workout session keeps the app frontmost with continuous location
/// updates and Always-On refreshes — wrist-down no longer suspends guidance.
final class WorkoutSessionManager: NSObject, ObservableObject {
  @Published private(set) var isRunning = false

  private let store = HKHealthStore()
  private var session: HKWorkoutSession?
  private var builder: HKLiveWorkoutBuilder?

  func toggle(routerType: String) {
    if isRunning {
      stop()
    } else {
      start(routerType: routerType)
    }
  }

  func stop() {
    guard isRunning else { return }
    isRunning = false
    session?.end()
    let finishingBuilder = builder
    finishingBuilder?.endCollection(withEnd: Date()) { _, _ in
      finishingBuilder?.finishWorkout { _, _ in }
    }
    session = nil
    builder = nil
  }

  private func start(routerType: String) {
    guard HKHealthStore.isHealthDataAvailable() else { return }
    let configuration = HKWorkoutConfiguration()
    switch routerType {
    case "bicycle": configuration.activityType = .cycling
    case "pedestrian": configuration.activityType = .hiking
    default: configuration.activityType = .other
    }
    configuration.locationType = .outdoor

    store.requestAuthorization(toShare: [HKObjectType.workoutType()], read: nil) { [weak self] granted, _ in
      guard granted, let self else { return }
      DispatchQueue.main.async {
        do {
          let session = try HKWorkoutSession(healthStore: self.store, configuration: configuration)
          let builder = session.associatedWorkoutBuilder()
          builder.dataSource = HKLiveWorkoutDataSource(healthStore: self.store,
                                                       workoutConfiguration: configuration)
          session.startActivity(with: Date())
          builder.beginCollection(withStart: Date()) { _, _ in }
          self.session = session
          self.builder = builder
          self.isRunning = true
        } catch {}
      }
    }
  }
}
