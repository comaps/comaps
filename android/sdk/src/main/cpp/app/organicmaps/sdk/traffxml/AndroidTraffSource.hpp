#pragma once

#include <jni.h>

#include "app/organicmaps/sdk/platform/AndroidPlatform.hpp"

#include "traffxml/traff_source.hpp"

namespace traffxml
{
/**
 * @brief A TraFF source which relies on Android Binder for message delivery, using version 0.7 of the TraFF protocol.
 *
 * TraFF 0.7 does not support subscriptions. Messages are broadcast as the payload to a `FEED` intent.
 */
class AndroidTraffSourceV0_7 : public TraffSource
{
public:
  /**
   * @brief Creates a new `AndroidTraffSourceV0_7` instance and registers it with the traffic manager.
   *
   * @param manager The traffic manager to register the new instance with
   */
  static void Create(TraffSourceManager & manager);

  virtual ~AndroidTraffSourceV0_7() override;

  /**
   * @brief Prepares the traffic source for unloading.
   */
  // TODO do we need a close operation here?
  // TODO move this to the parent class and override it here?
  void Close();

  /**
   * @brief Subscribes to a traffic service.
   *
   * TraFF 0.7 does not support subscriptions. This implementation registers a broadcast receiver.
   *
   * @param mwms The MWMs for which data is needed (not used by this implementation).
   */
  virtual void Subscribe(std::set<MwmSet::MwmId> & mwms) override;

  /**
   * @brief Changes an existing traffic subscription.
   *
   * This implementation does nothing, as TraFF 0.7 does not support subscriptions.
   *
   * @param mwms The new set of MWMs for which data is needed.
   */
  virtual void ChangeSubscription(std::set<MwmSet::MwmId> & mwms) override {};

  /**
   * @brief Unsubscribes from a traffic service we are subscribed to.
   *
   * TraFF 0.7 does not support subscriptions. This implementation unregisters the broadcast
   * receiver which was registered by `Subscribe()`.
   */
  virtual void Unsubscribe() override;

  /**
   * @brief Whether this source should be polled.
   *
   * Prior to calling `Poll()` on a source, the caller should always first call `IsPollNeeded()` and
   * poll the source only if the result is true.
   *
   * This implementation always returns false, as message delivery on Android uses `FEED` (push).
   *
   * @return true if the source should be polled, false if not.
   */
  virtual bool IsPollNeeded() override { return false; };

  /**
   * @brief Polls the traffic service for updates.
   *
   * This implementation does nothing, as message delivery on Android uses `FEED` (push).
   */
  virtual void Poll() override {};

protected:
  /**
   * @brief Constructs a new `AndroidTraffSourceV0_7`.
   * @param manager The `TrafficSourceManager` instance to register the source with.
   */
  AndroidTraffSourceV0_7(TraffSourceManager & manager);

private:
  // TODO “subscription” (i.e. broadcast receiver) state

  /**
   * @brief The Java implementation class instance.
   */
  jobject m_implObject;

  /**
   * @brief The Java subscribe method.
   */
  jmethodID m_subscribeImpl;

  /**
   * @brief The Java unsubscribe method.
   */
  jmethodID m_unsubscribeImpl;
};

/**
 * @brief A TraFF source which relies on Android Binder for message delivery, using version 0.8 of the TraFF protocol.
 *
 * TraFF 0.8 supports subscriptions. Messages are announced through a `FEED` intent, whereupon the
 * consumer can retrieve them from a content provider.
 */
class AndroidTraffSourceV0_8 : public TraffSource
{
public:
  /**
   * @brief Creates a new `AndroidTraffSourceV0_8` instance and registers it with the traffic manager.
   *
   * @param manager The traffic manager to register the new instance with
   * @param packageId The package ID of the app providing the TraFF source.
   */
  static void Create(TraffSourceManager & manager, std::string const & packageId);

  virtual ~AndroidTraffSourceV0_8() override;

  /**
   * @brief Prepares the traffic source for unloading.
   *
   * If there is still an active subscription, it unsubscribes, but without processing the result
   * received from the service. Otherwise, teardown is a no-op.
   */
  // TODO move this to the parent class and override it here?
  void Close();

  /**
   * @brief Subscribes to a traffic service.
   *
   * @param mwms The MWMs for which data is needed.
   */
  virtual void Subscribe(std::set<MwmSet::MwmId> & mwms) override;

  /**
   * @brief Changes an existing traffic subscription.
   *
   * @param mwms The new set of MWMs for which data is needed.
   */
  virtual void ChangeSubscription(std::set<MwmSet::MwmId> & mwms) override;

  /**
   * @brief Unsubscribes from a traffic service we are subscribed to.
   */
  virtual void Unsubscribe() override;

  /**
   * @brief Whether this source should be polled.
   *
   * Prior to calling `Poll()` on a source, the caller should always first call `IsPollNeeded()` and
   * poll the source only if the result is true.
   *
   * This implementation always returns false, as message delivery on Android uses `FEED` (push).
   *
   * @return true if the source should be polled, false if not.
   */
  virtual bool IsPollNeeded() override { return false; };

  /**
   * @brief Polls the traffic service for updates.
   *
   * This implementation does nothing, as message delivery on Android uses `FEED` (push).
   */
  virtual void Poll() override {};

protected:
  /**
   * @brief Constructs a new `AndroidTraffSourceV0_8`.
   * @param manager The `TrafficSourceManager` instance to register the source with.
   * @param packageId The package ID of the app providing the TraFF source.
   */
  AndroidTraffSourceV0_8(TraffSourceManager & manager, std::string const & packageId);

private:
  // TODO subscription state

  /**
   * @brief The Java implementation class instance.
   */
  jobject m_implObject;

  /**
   * @brief The Java subscribe method.
   */
  jmethodID m_subscribeImpl;

  /**
   * @brief The Java changeSubscription method.
   */
  jmethodID m_changeSubscriptionImpl;

  /**
   * @brief The Java unsubscribe method.
   */
  jmethodID m_unsubscribeImpl;
};
}  // namespace traffxml
