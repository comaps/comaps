#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "app/organicmaps/sdk/Framework.hpp"

#include "indexer/map_style.hpp"

#include "map/routing_manager.hpp"

extern "C"
{
JNIEXPORT void JNICALL Java_app_organicmaps_sdk_Router_nativeSet(JNIEnv *, jclass, jint routerType)
{
  using Type = routing::RouterType;
  Type type;
  int mapMode;

  switch (routerType)
  {
  case 0: type = Type::Vehicle; mapMode = 2; break;
  case 1: type = Type::Pedestrian; mapMode = 0; break;
  case 2: type = Type::Bicycle; mapMode = 1; break;
  case 3: type = Type::Transit; mapMode = 3; break;
  case 4: type = Type::Ruler; mapMode = 4; break;
  default: ASSERT(false, (routerType)); return;
  }

  frm()->SwitchToMapMode(static_cast<MapMode>(mapMode), true);
  frm()->GetRoutingManager().SetRouter(type);
}

JNIEXPORT jint JNICALL Java_app_organicmaps_sdk_Router_nativeGet(JNIEnv *, jclass)
{
  return static_cast<jint>(frm()->GetRoutingManager().GetRouter());
}

JNIEXPORT jint JNICALL Java_app_organicmaps_sdk_Router_nativeGetLastUsed(JNIEnv *, jclass)
{
  return static_cast<jint>(routing::GetLastUsedRouter());
}

JNIEXPORT jint JNICALL Java_app_organicmaps_sdk_Router_nativeGetBest(JNIEnv *, jclass, jdouble srcLat, jdouble srcLon,
                                                                     jdouble dstLat, jdouble dstLon)
{
  return static_cast<jint>(frm()->GetRoutingManager().GetBestRouter(mercator::FromLatLon(srcLat, srcLon),
                                                                    mercator::FromLatLon(dstLat, dstLon)));
}
}
