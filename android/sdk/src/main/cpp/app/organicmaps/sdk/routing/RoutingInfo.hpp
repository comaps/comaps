#pragma once

#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/routing/LaneInfo.hpp"
#include "app/organicmaps/sdk/util/Distance.hpp"
#include "app/organicmaps/sdk/routing/roadshield/RoadShieldInfo.hpp"

#include "map/routing_manager.hpp"

#include "routing/following_info.hpp"

jobject CreateRoutingInfo(JNIEnv * env, routing::FollowingInfo const & info, RoutingManager & rm)
{
  static jclass const klass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/RoutingInfo");
  // clang-format off
  static jmethodID const ctorRouteInfoID =
      jni::GetConstructorID(env, klass,
                            "("
                            "Lapp/organicmaps/sdk/util/Distance;"                     // distToTarget
                            "Lapp/organicmaps/sdk/util/Distance;"                     // distToTurn
                            "Ljava/lang/String;"                                      // currentStreet
                            "Ljava/lang/String;"                                      // nextStreet
                            "Lapp/organicmaps/sdk/routing/roadshield/RoadShieldInfo;" // nextStreetRoadShields
                            "Ljava/lang/String;"                                      // nextNextStreet
                            "Lapp/organicmaps/sdk/routing/roadshield/RoadShieldInfo;" // nextNextStreetRoadShields
                            "D"                                                       // completionPercent
                            "I"                                                       // vehicleTurnOrdinal
                            "I"                                                       // vehicleNextTurnOrdinal
                            "I"                                                       // pedestrianTurnOrdinal
                            "I"                                                       // exitNum
                            "I"                                                       // totalTime
                            "[Lapp/organicmaps/sdk/routing/LaneInfo;"                 // lanes
                            "D"                                                       // speedLimitMps
                            "Z"                                                       // speedLimitExceeded
                            "Z"                                                       // shouldPlayWarningSignal
                            "I"                                                       // routingSessionState
                            "I"                                                       // indexOfNextStop
                            "Lapp/organicmaps/sdk/util/Distance;"                     // distToNextStop
                            "I"                                                       // timeToNextStop
                            "Ljava/lang/String;"                                      // nextName
                            "Ljava/lang/String;"                                      // nextRef
                            "Ljava/lang/String;"                                      // nextJunctionRef
                            "Ljava/lang/String;"                                      // nextDestinationRef
                            "Ljava/lang/String;"                                      // nextDestination
                            "Z"                                                       // nextIsLink
                            "Z"                                                       // isLeftHandTraffic
                            ")V");

  jobjectArray jLanes = CreateLanesInfo(env, info.m_lanes);

  auto const isSpeedCamLimitExceeded = rm.IsSpeedCamLimitExceeded();
  auto const shouldPlaySignal = rm.GetSpeedCamManager().ShouldPlayBeepSignal();
  jobject const result = env->NewObject( klass, ctorRouteInfoID,
                                         ToJavaDistance(env, info.m_distToTarget),
                                         ToJavaDistance(env, info.m_distToTurn),
                                         jni::ToJavaString(env, info.m_currentStreetName),
                                         jni::ToJavaString(env, info.m_nextStreetName),
                                         ToJavaRoadShieldInfo(env, info.m_nextStreetShields),
                                         jni::ToJavaString(env, info.m_nextNextStreetName),
                                         ToJavaRoadShieldInfo(env, info.m_nextNextStreetShields),
                                         info.m_completionPercent,
                                         info.m_turn,
                                         info.m_nextTurn,
                                         info.m_pedestrianTurn,
                                         info.m_exitNum,
                                         info.m_time,
                                         jLanes,
                                         info.m_speedLimitMps,
                                         static_cast<jboolean>(isSpeedCamLimitExceeded),
                                         static_cast<jboolean>(shouldPlaySignal),
                                         static_cast<jint>(info.m_routingSessionState),
                                         info.m_indexOfNextStop,
                                         ToJavaDistance(env, info.m_distToNextStop),
                                         static_cast<jint>(info.m_timeToNextStop),
                                         jni::ToJavaString(env, info.m_nextName),
                                         jni::ToJavaString(env, info.m_nextRef),
                                         jni::ToJavaString(env, info.m_nextJunctionRef),
                                         jni::ToJavaString(env, info.m_nextDestinationRef),
                                         jni::ToJavaString(env, info.m_nextDestination),
                                         static_cast<jboolean>(info.m_nextIsLink),
                                         static_cast<jboolean>(info.m_isLeftHandTraffic));
  ASSERT(result, (jni::DescribeException()));
  return result;
}

jdoubleArray CreateIntermediateStopsProgressArray(JNIEnv * env, RoutingManager & rm)
{
  std::vector<double> progress = rm.GetIntermediateStopsProgress();
  jdoubleArray jProgress = env->NewDoubleArray(progress.size());
  env->SetDoubleArrayRegion(jProgress, 0, progress.size(), progress.data());
  return jProgress;
}
