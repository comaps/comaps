#pragma once

#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/routing/LaneInfo.hpp"
#include "app/organicmaps/sdk/util/Distance.hpp"
#include "app/organicmaps/sdk/routing/roadshield/RoadShieldInfo.hpp"

#include "map/routing_manager.hpp"

#include "routing/following_info.hpp"
#include "routing/road_info.hpp"

#include <optional>

jobject ToJavaRoadInfo(JNIEnv * env, routing::RoadInfo const & roadInfo)
{
  static jclass const roadInfoClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/RoadInfo");
  static jmethodID const roadInfoConstructorID =
      jni::GetConstructorID(env, roadInfoClass,
                           "("
                           "Ljava/lang/String;"
                           "Lapp/organicmaps/sdk/routing/roadshield/RoadShieldInfo;"
                           "Ljava/lang/String;"
                           "Ljava/lang/String;"
                           "Ljava/lang/String;"
                           "Ljava/lang/String;"
                           "Ljava/lang/String;"
                           "Z"
                           ")V");

  jobject const result = env->NewObject(
      roadInfoClass, roadInfoConstructorID, jni::ToJavaString(env, roadInfo.m_formattedName),
      ToJavaRoadShieldInfo(env, roadInfo.m_shields), jni::ToJavaString(env, roadInfo.m_name),
      jni::ToJavaString(env, roadInfo.m_ref), jni::ToJavaString(env, roadInfo.m_junctionRef),
      jni::ToJavaString(env, roadInfo.m_destinationRef), jni::ToJavaString(env, roadInfo.m_destination),
      static_cast<jboolean>(roadInfo.m_isLink));
  ASSERT(result, (jni::DescribeException()));
  return result;
}

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
                            "Lapp/organicmaps/sdk/routing/RoadInfo;"                  // nextRoadInfo
                            "Z"                                                       // isLeftHandTraffic
                            "J"                                                       // routeId
                            "Ljava/lang/Integer;"                                     // currentStep
                            "Ljava/lang/Integer;"                                     // nextStep
                            ")V");

  jobjectArray jLanes = CreateLanesInfo(env, info.m_lanes);
  static jclass const integerClass = jni::GetGlobalClassRef(env, "java/lang/Integer");
  static jmethodID const integerValueOfID =
      jni::GetStaticMethodID(env, integerClass, "valueOf", "(I)Ljava/lang/Integer;");
  auto const toJavaStep = [env](std::optional<uint32_t> const & step)
  {
    if (!step)
      return static_cast<jobject>(nullptr);
    return env->CallStaticObjectMethod(integerClass, integerValueOfID, static_cast<jint>(*step));
  };
  jni::TScopedLocalRef currentStepRef(env, toJavaStep(info.m_currentStep));
  jni::TScopedLocalRef nextStepRef(env, toJavaStep(info.m_nextStep));

  auto const isSpeedCamLimitExceeded = rm.IsSpeedCamLimitExceeded();
  auto const shouldPlaySignal = rm.GetSpeedCamManager().ShouldPlayBeepSignal();
  jobject const result = env->NewObject(klass, ctorRouteInfoID,
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
                                        ToJavaRoadInfo(env, info.m_nextRoadInfo),
                                        static_cast<jboolean>(info.m_isLeftHandTraffic),
                                        static_cast<jlong>(info.m_routeId),
                                        currentStepRef.get(), nextStepRef.get());
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
