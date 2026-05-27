#include "drape/support_manager.hpp"
#include "drape/gl_functions.hpp"
#include "drape/vulkan/vulkan_base_context.hpp"

#include "platform/settings.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <array>
#include <string>

namespace dp
{
struct SupportManager::Configuration
{
  std::string_view m_deviceName;
  Version m_apiVersion;
  Version m_driverVersion;
};

std::string_view kSupportedAntialiasing = "Antialiasing";
std::string_view constexpr kVulkanForbidden = "VulkanForbidden";

void SupportManager::Init(ref_ptr<GraphicsContext> context)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_isInitialized)
    return;

  m_rendererName = context->GetRendererName();
  m_rendererVersion = context->GetRendererVersion();
  LOG(LINFO, ("Renderer =", m_rendererName, "| Api =", context->GetApiVersion(), "| Version =", m_rendererVersion));

  auto const apiVersion = context->GetApiVersion();
  if (apiVersion == dp::ApiVersion::OpenGLES3)
  {
    m_maxLineWidth = static_cast<float>(std::max(1, GLFunctions::glGetMaxLineWidth()));
    m_maxTextureSize = static_cast<uint32_t>(GLFunctions::glGetInteger(gl_const::GLMaxTextureSize));
  }
  else if (apiVersion == dp::ApiVersion::Metal)
  {
    // Metal does not support thick lines.
    m_maxLineWidth = 1.0f;
    m_maxTextureSize = 4096;
  }
  else if (apiVersion == dp::ApiVersion::Vulkan)
  {
    ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext = context;
    auto const & props = vulkanContext->GetGpuProperties();
    m_maxLineWidth = std::max(props.limits.lineWidthRange[0], props.limits.lineWidthRange[1]);
    m_maxTextureSize = vulkanContext->GetGpuProperties().limits.maxImageDimension2D;
  }
  LOG(LINFO, ("Max line width =", m_maxLineWidth, "| Max texture size =", m_maxTextureSize));

  // Set up default antialiasing value.
  // Turn off AA for a while by energy-saving issues.
  //  bool val;
  //  if (!settings::Get(kSupportedAntialiasing, val))
  //  {
  // #ifdef OMIM_OS_ANDROID
  //    std::vector<std::string> const models = {"Mali-G71", "Mali-T880", "Adreno (TM) 540",
  //                                             "Adreno (TM) 530", "Adreno (TM) 430"};
  //    m_isAntialiasingEnabledByDefault = base::IsExist(models, m_rendererName);
  // #else
  //    m_isAntialiasingEnabledByDefault = true;
  // #endif
  //    settings::Set(kSupportedAntialiasing, m_isAntialiasingEnabledByDefault);
  //  }

  m_isInitialized = true;
}

void SupportManager::ForbidVulkan()
{
  settings::Set(kVulkanForbidden, true);
}

bool SupportManager::IsVulkanForbidden()
{
  bool forbidden;
  if (!settings::Get(kVulkanForbidden, forbidden))
    forbidden = false;
  return forbidden;
}

bool SupportManager::IsVulkanForbidden(std::string const & deviceName, Version apiVersion, Version driverVersion,
                                       bool isCustomROM, int sdkVersion)
{
  LOG(LINFO, ("Device =", deviceName, "API =", apiVersion, "Driver =", driverVersion, "SDK =", sdkVersion));

  // Vulkan crashes on Android Emulator (API 30 and API 36) due to a bug in the SwiftShader driver.
  // The workaround falls back to OpenGL on these configurations. Remove this check once the
  // upstream SwiftShader/ANGLE bug is confirmed fixed and the affected emulator images are no
  // longer in common use by developers and CI.
  if (deviceName == "SwiftShader Device (LLVM 10.0.0)" && (sdkVersion == 30 || sdkVersion == 36))
  {
    LOG(LWARNING, ("Use OpenGL instead of Vulkan on Android Emulator due to crashes caused by graphics driver."));
    return true;
  }

  static char const * kBannedDevices[] = {
      // PowerVR Rogue G6110/GE8100/GE8300: confirmed Vulkan crashes on these specific models.
      // Broader PowerVR Rogue ban was considered but rejected — only these three variants were
      // reproducibly broken; other Rogue variants appear fine with Vulkan.
      // See: https://github.com/organicmaps/organicmaps/issues/1379
      "PowerVR Rogue G6110",
      "PowerVR Rogue GE8100",
      "PowerVR Rogue GE8300",
      // Adreno 418: Vulkan driver produces rendering corruption; OpenGL fallback is stable.
      // See: https://github.com/organicmaps/organicmaps/issues/5539
      "Adreno (TM) 418",
  };

  for (auto const d : kBannedDevices)
    if (d == deviceName)
      return true;

  if (isCustomROM)
  {
    // Crash on LineageOS, stock Android works ok (with same api = 1.0.82; driver = 28.0.0).
    // https://github.com/organicmaps/organicmaps/issues/2739
    // https://github.com/organicmaps/organicmaps/issues/9255
    // SM-G930F (S7, heroltexx, hero2ltexx). Crash on vkCreateSwapchainKHR and we don't even get to
    // SupportManager::Init. SM-G920F (S6)
    if (deviceName.starts_with("Mali-T"))
      return true;
  }

  // On these configurations we've detected fatal driver-specific Vulkan errors.
  static Configuration constexpr kBannedConfigurations[] = {
      Configuration{"Adreno (TM) 506", {1, 0, 31}, {42, 264, 975}},
      Configuration{"Adreno (TM) 506", {1, 1, 66}, {512, 313, 0}},
      // Xiaomi Redmi Note 5
      Configuration{"Adreno (TM) 506", {1, 1, 128}, {512, 502, 0}},
      Configuration{"Adreno (TM) 530", {1, 1, 66}, {512, 313, 0}},

      // Mali-G71 (Samsung Galaxy S8, SM-G950F): route line flickers in navigation mode with
      // this specific API/driver combination. The root cause is a driver bug; no upstream fix
      // is expected. Vulkan is permanently disabled for this configuration.
      Configuration{"Mali-G71", {1, 0, 97}, {16, 0, 0}},

      // Mali-G72 (Huawei P20): dashed lines stopped rendering after a LineShape refactor in
      // DashedLineBuilder. The regression is driver-specific — the same shader works on other
      // Mali-G72 variants. Vulkan is permanently disabled for this API/driver pair.
      Configuration{"Mali-G72", {1, 1, 97}, {18, 0, 0}},
      /// Samsung SM-A505FN (a50), hangs when showing the subway layer.
      Configuration{"Mali-G72", {1, 1, 131}, {26, 0, 0}},
  };

  for (auto const & c : kBannedConfigurations)
    if (c.m_deviceName == deviceName && c.m_apiVersion == apiVersion && c.m_driverVersion == driverVersion)
      return true;
  return false;
}

// Finally, the result of this function is used in GraphicsContext::HasPartialTextureUpdates.
bool SupportManager::IsVulkanTexturePartialUpdateBuggy(int sdkVersion, std::string const & deviceName,
                                                       Version apiVersion, Version driverVersion)
{
  // Android 10+ (API 29+) devices are blanket-marked as not supporting partial Vulkan texture
  // updates. This is a conservative assumption: partial updates were confirmed broken on several
  // API 29+ devices and the safe fallback (full re-upload) has no observable perf impact on
  // modern hardware. If a future investigation finds that specific API 29+ drivers do support
  // partial updates correctly, this threshold can be narrowed to a device/driver allowlist.
  if (sdkVersion >= 29)
    return true;

  // For these configurations partial updates of texture clears whole texture except part updated
  static Configuration constexpr kBadConfigurations[] = {
      {"Mali-G76", {1, 1, 97}, {18, 0, 0}},
  };

  for (auto const & c : kBadConfigurations)
    if (c.m_deviceName == deviceName && c.m_apiVersion == apiVersion && c.m_driverVersion == driverVersion)
      return true;
  return false;
}

SupportManager & SupportManager::Instance()
{
  static SupportManager manager;
  return manager;
}
}  // namespace dp
