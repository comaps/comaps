#include "app/organicmaps/sdk/routing/RouteStepInfo.hpp"
#include "app/organicmaps/sdk/util/Distance.hpp"

jobject ToJavaRouteStepInfo(JNIEnv * env, routing::RouteStepInfo const & step)
{
    static jclass const routeStepInfoClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/RouteStepInfo");
    static jmethodID const ctorRouteStepInfoID = jni::GetConstructorID(env, routeStepInfoClass,
                                                                       "("
                                                                       "I"
                                                                       "I"
                                                                       "I"
                                                                       "Ljava/lang/String;"
                                                                       "Ljava/lang/String;"
                                                                       "I"
                                                                       "D"
                                                                       "Lapp/organicmaps/sdk/util/Distance;"
                                                                       "Ljava/lang/String;"
                                                                       ")V"
    );

    return env->NewObject(routeStepInfoClass, ctorRouteStepInfoID,
                   static_cast<jint>(step.m_index),
                   static_cast<jint>(step.m_turn),
                   static_cast<jint>(step.m_pedestrianTurn),
                   jni::ToJavaString(env, step.m_fromStreetName),
                   jni::ToJavaString(env, step.m_toStreetName),
                   static_cast<jint>(step.m_exitNum),
                   static_cast<jdouble>(step.m_distMeters),
                   ToJavaDistance(env, step.m_formattedDistance),
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
