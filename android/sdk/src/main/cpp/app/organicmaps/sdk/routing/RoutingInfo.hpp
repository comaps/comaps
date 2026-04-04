#pragma once

#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/routing/LaneInfo.hpp"
#include "app/organicmaps/sdk/util/Distance.hpp"

#include "map/routing_manager.hpp"

#include "routing/following_info.hpp"

jobject CreateRoutingInfo(JNIEnv * env, routing::FollowingInfo const & info, RoutingManager & rm)
{
  static jclass const klass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/RoutingInfo");
  // Java signature : RoutingInfo(Distance distToTarget, Distance distToTurn,
  //                              String currentStreet, String nextStreet, String nextNextStreet,
  //                              double completionPercent, int vehicleTurnOrdinal,
  //                              int vehicleNextTurnOrdinal, int pedestrianTurnOrdinal, int exitNum,
  //                              int totalTime, LaneInfo[] lanes, double speedLimitMps,
  //                              boolean speedLimitExceeded, boolean shouldPlayWarningSignal,
  //                              int routingSessionState, int indexOfNextStop,
  //                              Distance distToNextStop, int timeToNextStop)
  static jmethodID const ctorRouteInfoID =
      jni::GetConstructorID(env, klass,
                            "(Lapp/organicmaps/sdk/util/Distance;Lapp/organicmaps/sdk/util/Distance;"
                            "Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DIIIII"
                            "[Lapp/organicmaps/sdk/routing/LaneInfo;DZZII"
                            "Lapp/organicmaps/sdk/util/Distance;I)V");

  jobjectArray jLanes = CreateLanesInfo(env, info.m_lanes);

  auto const isSpeedCamLimitExceeded = rm.IsSpeedCamLimitExceeded();
  auto const shouldPlaySignal = rm.GetSpeedCamManager().ShouldPlayBeepSignal();
  jobject const result = env->NewObject(
      klass, ctorRouteInfoID, ToJavaDistance(env, info.m_distToTarget), ToJavaDistance(env, info.m_distToTurn),
      jni::ToJavaString(env, info.m_currentStreetName), jni::ToJavaString(env, info.m_nextStreetName),
      jni::ToJavaString(env, info.m_nextNextStreetName), info.m_completionPercent, info.m_turn, info.m_nextTurn,
      info.m_pedestrianTurn, info.m_exitNum, info.m_time, jLanes, info.m_speedLimitMps,
      static_cast<jboolean>(isSpeedCamLimitExceeded), static_cast<jboolean>(shouldPlaySignal),
      static_cast<jint>(info.m_routingSessionState), info.m_indexOfNextStop,
      ToJavaDistance(env, info.m_distToNextStop), static_cast<jint>(info.m_timeToNextStop));
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
