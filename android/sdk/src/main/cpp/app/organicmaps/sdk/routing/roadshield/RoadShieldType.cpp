#include "RoadShieldType.hpp"

namespace
{
std::string ToJavaRoadShieldTypeName(ftypes::RoadShieldType roadShieldType)
{
  switch (roadShieldType)
  {
    using enum ftypes::RoadShieldType;
  case Default: [[fallthrough]];
  case Hidden: [[fallthrough]];
  case Generic_White: return "GenericWhite";
  case Generic_Green: return "GenericGreen";
  case Generic_Blue: return "GenericBlue";
  case Generic_Red: return "GenericRed";
  case Generic_Orange: return "GenericOrange";
  case Generic_White_Bordered: return "GenericWhite";
  case Generic_Green_Bordered: return "GenericGreen";
  case Generic_Blue_Bordered: return "GenericBlue";
  case Generic_Red_Bordered: return "GenericRed";
  case Generic_Orange_Bordered: return "GenericOrange";
  case Generic_Pill_White: return "GenericWhite";
  case Generic_Pill_Green: return "GenericGreen";
  case Generic_Pill_Blue: return "GenericBlue";
  case Generic_Pill_Red: return "GenericRed";
  case Generic_Pill_Orange: return "GenericOrange";
  case Generic_Pill_White_Bordered: return "GenericWhite";
  case Generic_Pill_Green_Bordered: return "GenericGreen";
  case Generic_Pill_Blue_Bordered: return "GenericBlue";
  case Generic_Pill_Red_Bordered: return "GenericRed";
  case Generic_Pill_Orange_Bordered: return "GenericOrange";
  case Highway_Hexagon_Green: return "GenericGreen";
  case Highway_Hexagon_Blue: return "GenericBlue";
  case Highway_Hexagon_Red: return "GenericRed";
  case Highway_Hexagon_Turkey: return "GenericBlue";
  case US_Interstate: return "USInterstate";
  case US_Highway: return "USHighway";
  case UK_Highway: return "UKHighway";
  case UY_National: return "GenericBlue";
  case Italy_Autostrada: return "GenericGreen";
  case Hungary_Green: return "GenericGreen";
  case Hungary_Blue: return "GenericBlue";
  case Argentina_RN: return "GenericBlue";
  case Bolivia_Fundamental: return "GenericBlue";
  case Brazil_National: return "GenericWhite";
  case Brazil_State: return "GenericWhite";
  default: UNREACHABLE();
  }
}
}  // namespace

jobject ToJavaRoadShieldType(JNIEnv * env, ftypes::RoadShieldType roadShieldType)
{
  static jclass const roadShieldTypeClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/roadshield/RoadShieldType");
  jfieldID const fieldID = env->GetStaticFieldID(roadShieldTypeClass, ToJavaRoadShieldTypeName(roadShieldType).c_str(),
                                                 "Lapp/organicmaps/sdk/routing/roadshield/RoadShieldType;");
  return env->GetStaticObjectField(roadShieldTypeClass, fieldID);
}
