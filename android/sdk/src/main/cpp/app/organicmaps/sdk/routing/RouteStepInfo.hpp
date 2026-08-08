#pragma once

#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "routing/route_step.hpp"

jobject ToJavaRouteStepInfo(JNIEnv * env, routing::RouteStepInfo const & step);

jobjectArray CreateRouteStepInfoArray(JNIEnv * env, std::vector<routing::RouteStepInfo> const & steps);

