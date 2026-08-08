#pragma once

#include "indexer/feature_decl.hpp"
#include "indexer/mwm_set.hpp"

#include "geometry/spline.hpp"

#include <atomic>

#include "3party/ankerl/unordered_dense.h"

namespace df
{
class MapDataProvider;

using MetalineCache = ankerl::unordered_dense::map<FeatureID, m2::SharedSpline>;

class ReadMetalineTask
{
public:
  ReadMetalineTask(MapDataProvider & model, MwmSet::MwmId const & mwmId);

  void Run();
  bool UpdateCache(MetalineCache & cache);

  void Cancel() { m_isCancelled = true; }
  bool IsCancelled() const { return m_isCancelled; }
  MwmSet::MwmId const & GetMwmId() const { return m_mwmId; }

private:
  MapDataProvider & m_model;
  MwmSet::MwmId m_mwmId;
  MetalineCache m_metalines;
  std::atomic<bool> m_isCancelled;
};
}  // namespace df
