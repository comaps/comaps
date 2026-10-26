#include "AndroidTraffSource.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

namespace traffxml {
void AndroidTraffSourceV0_7::Create(TraffSourceManager & manager)
{
  std::unique_ptr<AndroidTraffSourceV0_7> source = std::unique_ptr<AndroidTraffSourceV0_7>(new AndroidTraffSourceV0_7(manager));
  manager.RegisterSource(std::move(source));
}

AndroidTraffSourceV0_7::AndroidTraffSourceV0_7(TraffSourceManager & manager)
  : TraffSource(manager)
{
  JNIEnv * env = jni::GetEnv();

  static jclass const implClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/traffxml/SourceImplV0_7");

  static jmethodID const implConstructor = jni::GetConstructorID(env, implClass, "(Landroid/content/Context;J)V");

  jlong nativeManager = reinterpret_cast<jlong>(&manager);

  jobject implObject = env->NewObject(
      implClass, implConstructor, android::Platform::Instance().GetContext(), nativeManager);

  m_implObject = env->NewGlobalRef(implObject);

  m_subscribeImpl = jni::GetMethodID(env, m_implObject, "subscribe", "(Ljava/lang/String;)V");
  m_unsubscribeImpl = jni::GetMethodID(env, m_implObject, "unsubscribe", "()V");
}

AndroidTraffSourceV0_7::~AndroidTraffSourceV0_7()
{
  jni::GetEnv()->DeleteGlobalRef(m_implObject);
}

void AndroidTraffSourceV0_7::Close()
{
  Unsubscribe();
}

void AndroidTraffSourceV0_7::Subscribe(std::set<MwmSet::MwmId> const & mwms)
{
  jni::GetEnv()->CallVoidMethod(m_implObject, m_subscribeImpl, nullptr);
}

void AndroidTraffSourceV0_7::Unsubscribe()
{
  jni::GetEnv()->CallVoidMethod(m_implObject, m_unsubscribeImpl);
}

void AndroidTraffSourceV0_8::Create(TraffSourceManager & manager, std::string const & packageId)
{
  std::unique_ptr<AndroidTraffSourceV0_8> source = std::unique_ptr<AndroidTraffSourceV0_8>(new AndroidTraffSourceV0_8(manager, packageId));
  manager.RegisterSource(std::move(source));
}

AndroidTraffSourceV0_8::AndroidTraffSourceV0_8(TraffSourceManager & manager, std::string const & packageId)
  : TraffSource(manager)
{
  JNIEnv * env = jni::GetEnv();

  static jclass const implClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/traffxml/SourceImplV0_8");

  static jmethodID const implConstructor = jni::GetConstructorID(env, implClass, "(Landroid/content/Context;JLjava/lang/String;)V");

  jlong nativeManager = reinterpret_cast<jlong>(&manager);

  jobject implObject = env->NewObject(
      implClass, implConstructor, android::Platform::Instance().GetContext(), nativeManager, jni::ToJavaString(env, packageId));

  m_implObject = env->NewGlobalRef(implObject);

  m_subscribeImpl = jni::GetMethodID(env, m_implObject, "subscribe", "(Ljava/lang/String;)V");
  m_changeSubscriptionImpl = jni::GetMethodID(env, m_implObject, "changeSubscription", "(Ljava/lang/String;)V");
  m_unsubscribeImpl = jni::GetMethodID(env, m_implObject, "unsubscribe", "()V");

  // TODO packageId (if we need that at all here)
}

AndroidTraffSourceV0_8::~AndroidTraffSourceV0_8()
{
  jni::GetEnv()->DeleteGlobalRef(m_implObject);
}

void AndroidTraffSourceV0_8::Close()
{
  Unsubscribe();
}

void AndroidTraffSourceV0_8::Subscribe(std::set<MwmSet::MwmId> const & mwms)
{
  JNIEnv * env = jni::GetEnv();
  std::string data = "<filter_list>\n"
      + GetMwmFilters(mwms)
      + "</filter_list>";

  env->CallVoidMethod(m_implObject, m_subscribeImpl, jni::ToJavaString(env, data));
}

void AndroidTraffSourceV0_8::ChangeSubscription(std::set<MwmSet::MwmId> const & mwms)
{
  JNIEnv * env = jni::GetEnv();
  std::string data = "<filter_list>\n"
      + GetMwmFilters(mwms)
      + "</filter_list>";

  env->CallVoidMethod(m_implObject, m_changeSubscriptionImpl, jni::ToJavaString(env, data));
}

void AndroidTraffSourceV0_8::Unsubscribe()
{
  jni::GetEnv()->CallVoidMethod(m_implObject, m_unsubscribeImpl);
}
}  // namespace traffxml
