#include "app/organicmaps/sdk/routing/RouteStepInfo.hpp"
#include "app/organicmaps/sdk/routing/LaneInfo.hpp"
#include "app/organicmaps/sdk/routing/roadshield/RoadShieldInfo.hpp"
#include "app/organicmaps/sdk/util/Distance.hpp"

jobject ToJavaRouteStepInfo(JNIEnv * env, routing::RouteStepInfo const & step)
{
  static jclass const routeStepInfoClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/RouteStepInfo");
  static jmethodID const ctorRouteStepInfoID =
      jni::GetConstructorID(env, routeStepInfoClass,
                           "("
                           "I"
                           "I"
                           "I"
                           "I"
                           "Ljava/lang/String;"
                           "Ljava/lang/String;"
                           "Ljava/lang/String;"
                           "Ljava/lang/String;"
                           "Ljava/lang/String;"
                           "Ljava/lang/String;"
                           "Ljava/lang/String;"
                           "Z"
                           "I"
                           "D"
                           "D"
                           "Lapp/organicmaps/sdk/util/Distance;"
                           "[Lapp/organicmaps/sdk/routing/LaneInfo;"
                           "Lapp/organicmaps/sdk/routing/roadshield/RoadShieldInfo;"
                           "Ljava/lang/String;"
                           ")V");

    return env->NewObject(routeStepInfoClass, ctorRouteStepInfoID, static_cast<jint>(step.m_index),
                          static_cast<jint>(step.m_sequence), static_cast<jint>(step.m_turn),
                          static_cast<jint>(step.m_pedestrianTurn), jni::ToJavaString(env, step.m_fromStreetName),
                          jni::ToJavaString(env, step.m_toRoad.m_name),
                          jni::ToJavaString(env, step.m_toRoad.m_formattedName),
                          jni::ToJavaString(env, step.m_toRoad.m_ref),
                          jni::ToJavaString(env, step.m_toRoad.m_junctionRef),
                          jni::ToJavaString(env, step.m_toRoad.m_destinationRef),
                          jni::ToJavaString(env, step.m_toRoad.m_destination),
                          static_cast<jboolean>(step.m_toRoad.m_isLink), static_cast<jint>(step.m_exitNum),
                          static_cast<jdouble>(step.m_distMeters), static_cast<jdouble>(step.m_distFromStartMeters),
                          ToJavaDistance(env, step.m_formattedDistance), CreateLanesInfo(env, step.m_lanes),
                          ToJavaRoadShieldInfo(env, step.m_toRoad.m_shields),
                          jni::ToJavaString(env, step.m_textualInstruction));
}

jobjectArray CreateRouteStepInfoArray(JNIEnv * env, std::vector<routing::RouteStepInfo> const & steps)
{
  if (steps.empty())
    return nullptr;

  static jclass const routeStepInfoClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/RouteStepInfo");

  auto const resultSize = static_cast<jsize>(steps.size());
  jobjectArray result = env->NewObjectArray(resultSize, routeStepInfoClass, nullptr);

  for (jsize i = 0; i < resultSize; ++i)
  {
    auto const & step = steps[i];
    jni::TScopedLocalRef routeStepInfo(env, ToJavaRouteStepInfo(env, step));
    env->SetObjectArrayElement(result, i, routeStepInfo.get());
  }

  return result;
}

jobject ToJavaRoutePlan(JNIEnv * env, routing::RoutePlan const & plan)
{
  static jclass const routePlanClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/RoutePlan");
  static jmethodID const routePlanConstructorID =
      jni::GetConstructorID(env, routePlanClass, "(J[Lapp/organicmaps/sdk/routing/RouteStepInfo;)V");
  static jclass const routeStepInfoClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/RouteStepInfo");

  jobjectArray routeSteps = CreateRouteStepInfoArray(env, plan.m_steps);
  if (routeSteps == nullptr)
    routeSteps = env->NewObjectArray(0, routeStepInfoClass, nullptr);
  jni::TScopedLocalObjectArrayRef scopedRouteSteps(env, routeSteps);

  jobject const result = env->NewObject(routePlanClass, routePlanConstructorID, static_cast<jlong>(plan.m_routeId),
                                        scopedRouteSteps.get());
  ASSERT(result, (jni::DescribeException()));
  return result;
}
