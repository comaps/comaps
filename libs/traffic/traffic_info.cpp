#include "traffic/traffic_info.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"

#include "routing_common/car_model.hpp"

#include "indexer/feature_algo.hpp"
#include "indexer/feature_processor.hpp"

#include "coding/bit_streams.hpp"
#include "coding/elias_coder.hpp"
#include "coding/files_container.hpp"
#include "coding/reader.hpp"
#include "coding/url.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"
#include "coding/zlib.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <sstream>
#include <string>

#include "defines.hpp"
#include "private.h"

namespace traffic
{
using namespace std;

// TrafficInfo::RoadSegmentId -----------------------------------------------------------------
TrafficInfo::RoadSegmentId::RoadSegmentId() : m_fid(0), m_idx(0), m_dir(0) {}

TrafficInfo::RoadSegmentId::RoadSegmentId(uint32_t fid, uint16_t idx, uint8_t dir) : m_fid(fid), m_idx(idx), m_dir(dir)
{}

// TrafficInfo --------------------------------------------------------------------------------

// static
uint8_t const TrafficInfo::kLatestKeysVersion = 0;
uint8_t const TrafficInfo::kLatestValuesVersion = 0;

TrafficInfo::TrafficInfo(MwmSet::MwmId const & mwmId, Coloring && coloring)
  : m_mwmId(mwmId)
  , m_coloring(std::move(coloring))
{
  m_availability = Availability::IsAvailable;
}

// static
TrafficInfo TrafficInfo::BuildForTesting(Coloring && coloring)
{
  TrafficInfo info;
  info.m_coloring = std::move(coloring);
  return info;
}

void TrafficInfo::SetTrafficKeysForTesting(vector<RoadSegmentId> const & keys)
{
  m_keys = keys;
  m_availability = Availability::IsAvailable;
}

SpeedGroup TrafficInfo::GetSpeedGroup(RoadSegmentId const & id) const
{
  auto const it = m_coloring.find(id);
  if (it == m_coloring.cend())
    return SpeedGroup::Unknown;
  return it->second;
}

// static
void TrafficInfo::ExtractTrafficKeys(string const & mwmPath, vector<RoadSegmentId> & result)
{
  result.clear();
  feature::ForEachFeature(mwmPath, [&](FeatureType & ft, uint32_t const fid)
  {
    feature::TypesHolder const types(ft);
    if (!routing::CarModel::AllLimitsInstance().IsRoad(types))
      return;

    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
    auto const numPoints = static_cast<uint16_t>(ft.GetPointsCount());
    uint8_t const numDirs = routing::CarModel::AllLimitsInstance().IsOneWay(types) ? 1 : 2;
    for (uint16_t i = 0; i + 1 < numPoints; ++i)
      for (uint8_t dir = 0; dir < numDirs; ++dir)
        result.emplace_back(fid, i, dir);
  });

  ASSERT(is_sorted(result.begin(), result.end()), ());
}

// static
void TrafficInfo::CombineColorings(vector<TrafficInfo::RoadSegmentId> const & keys,
                                   TrafficInfo::Coloring const & knownColors, TrafficInfo::Coloring & result)
{
  result.clear();
  size_t numKnown = 0;
  size_t numUnknown = 0;
#ifdef DEBUG
  size_t numUnexpectedKeys = knownColors.size();
#endif
  for (auto const & key : keys)
  {
    auto it = knownColors.find(key);
    if (it == knownColors.end())
    {
      result[key] = SpeedGroup::Unknown;
      ++numUnknown;
    }
    else
    {
      result[key] = it->second;
      ASSERT_GREATER(numUnexpectedKeys--, 0, ());
      ++numKnown;
    }
  }

  LOG(LINFO, ("Road segments: known/unknown/total =", numKnown, numUnknown, numKnown + numUnknown));
  ASSERT_EQUAL(numUnexpectedKeys, 0, ());
}

// static
void TrafficInfo::DeserializeTrafficKeys(vector<uint8_t> const & data, vector<TrafficInfo::RoadSegmentId> & result)
{
  MemReaderWithExceptions memReader(data.data(), data.size());
  ReaderSource<decltype(memReader)> src(memReader);
  auto const version = ReadPrimitiveFromSource<uint8_t>(src);
  CHECK_EQUAL(version, kLatestKeysVersion, ("Unsupported version of traffic values."));
  auto const n = static_cast<size_t>(ReadVarUint<uint64_t>(src));

  vector<uint32_t> fids(n);
  vector<size_t> numSegs(n);
  vector<bool> oneWay(n);

  {
    BitReader<decltype(src)> bitReader(src);
    uint32_t prevFid = 0;
    for (size_t i = 0; i < n; ++i)
    {
      prevFid += coding::GammaCoder::Decode(bitReader) - 1;
      fids[i] = prevFid;
    }

    for (size_t i = 0; i < n; ++i)
      numSegs[i] = static_cast<size_t>(coding::GammaCoder::Decode(bitReader) - 1);

    for (size_t i = 0; i < n; ++i)
      oneWay[i] = bitReader.Read(1) > 0;
  }

  ASSERT_EQUAL(src.Size(), 0, ());

  result.clear();
  for (size_t i = 0; i < n; ++i)
  {
    auto const fid = fids[i];
    uint8_t numDirs = oneWay[i] ? 1 : 2;
    for (size_t j = 0; j < numSegs[i]; ++j)
    {
      for (uint8_t dir = 0; dir < numDirs; ++dir)
      {
        RoadSegmentId key(fid, j, dir);
        result.push_back(key);
      }
    }
  }
}

// static
void TrafficInfo::SerializeTrafficValues(vector<SpeedGroup> const & values, vector<uint8_t> & result)
{
  vector<uint8_t> buf;
  MemWriter<vector<uint8_t>> memWriter(buf);
  WriteToSink(memWriter, kLatestValuesVersion);
  WriteVarUint(memWriter, values.size());
  {
    BitWriter<decltype(memWriter)> bitWriter(memWriter);
    auto const numSpeedGroups = static_cast<uint8_t>(SpeedGroup::Count);
    static_assert(numSpeedGroups <= 8, "A speed group's value may not fit into 3 bits");
    for (auto const & v : values)
    {
      uint8_t const u = static_cast<uint8_t>(v);
      CHECK_LESS(u, numSpeedGroups, ());
      bitWriter.Write(u, 3);
    }
  }

  using Deflate = coding::ZLib::Deflate;
  Deflate deflate(Deflate::Format::ZLib, Deflate::Level::BestCompression);

  deflate(buf.data(), buf.size(), back_inserter(result));
}

string DebugPrint(TrafficInfo::RoadSegmentId const & id)
{
  string const dir = id.m_dir == TrafficInfo::RoadSegmentId::kForwardDirection ? "Forward" : "Backward";
  ostringstream oss;
  oss << "RoadSegmentId ["
      << " fid = " << id.m_fid << " idx = " << id.m_idx << " dir = " << dir << " ]";
  return oss.str();
}
}  // namespace traffic
