#pragma once

#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "3party/ankerl/unordered_dense.h"
#include "3party/skarupke/flat_hash_map.hpp"
#include "boost/container_hash/hash.hpp"

namespace routing
{
class RoutingOptions
{
public:
  enum Road : uint8_t
  {
    Usual = 1u << 0,
    Toll = 1u << 1,
    Motorway = 1u << 2,
    Ferry = 1u << 3,
    Dirty = 1u << 4,
    Steps = 1u << 5,
    Paved = 1u << 6,

    Max = (1u << 6) + 1
  };

  using RoadType = std::underlying_type_t<Road>;

  RoutingOptions() = default;
  explicit RoutingOptions(RoadType mask) : m_options(mask) {}

  static RoutingOptions LoadCarOptionsFromSettings();
  static void SaveCarOptionsToSettings(RoutingOptions options);

  void Add(Road type);
  void Remove(Road type);
  bool Has(Road type) const;

  RoadType GetOptions() const { return m_options; }

private:
  RoadType m_options = 0;
};

/// Border crossing avoidance mode for routing.
enum class BorderAvoidance : uint8_t
{
  None = 0,         /// Normal routing with soft penalty for non-internal border.
  Any = 1,          /// Avoid all border crossings.
  NonInternal = 2,  /// Avoid non-Schengen/EAEU borders.
  Specific = 3      /// Avoid crossing into specific countries.
};

std::string ToString(BorderAvoidance mode);
BorderAvoidance BorderAvoidanceFromString(std::string const & str);
std::string DebugPrint(BorderAvoidance mode);

/// Border crossing avoidance settings: mode + list of avoided country IDs.
class BorderAvoidanceSettings
{
public:
  BorderAvoidanceSettings() = default;

  static BorderAvoidanceSettings LoadFromSettings();
  void SaveToSettings() const;

  BorderAvoidance GetMode() const { return m_mode; }
  void SetMode(BorderAvoidance mode) { m_mode = mode; }

  ankerl::unordered_dense::set<std::string> const & GetAvoidedCountries() const { return m_avoidedCountries; }
  void SetAvoidedCountries(ankerl::unordered_dense::set<std::string> countries)
  {
    m_avoidedCountries = std::move(countries);
  }

  bool IsEmpty() const { return m_mode == BorderAvoidance::None && m_avoidedCountries.empty(); }

private:
  BorderAvoidance m_mode = BorderAvoidance::None;
  ankerl::unordered_dense::set<std::string> m_avoidedCountries;
};

class RoutingOptionsClassifier
{
public:
  RoutingOptionsClassifier();

  std::optional<RoutingOptions::Road> Get(uint32_t type) const;
  static RoutingOptionsClassifier const & Instance();

private:
  ska::flat_hash_map<uint32_t, RoutingOptions::Road, boost::hash<uint32_t>> m_data;
};

RoutingOptions::Road ChooseMainRoutingOptionRoad(RoutingOptions options, bool isCarRouter);

std::string DebugPrint(RoutingOptions const & routingOptions);
std::string DebugPrint(RoutingOptions::Road type);

/// Options guard for debugging/tests.
class RoutingOptionSetter
{
public:
  explicit RoutingOptionSetter(RoutingOptions::RoadType roadsMask);
  ~RoutingOptionSetter();

private:
  RoutingOptions m_saved;
};
}  // namespace routing
