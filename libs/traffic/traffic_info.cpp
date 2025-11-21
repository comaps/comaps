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

namespace
{
bool ReadRemoteFile(string const & url, vector<uint8_t> & contents, int & errorCode)
{
  platform::HttpClient request(url);
  if (!request.RunHttpRequest())
  {
    errorCode = request.ErrorCode();
    LOG(LINFO, ("Couldn't run traffic request", url, ". Error:", errorCode));
    return false;
  }

  errorCode = request.ErrorCode();

  string const & result = request.ServerResponse();
  contents.resize(result.size());
  memcpy(contents.data(), result.data(), result.size());

  if (errorCode != 200)
  {
    LOG(LINFO, ("Traffic request", url, "failed. HTTP Error:", errorCode));
    return false;
  }

  return true;
}

char constexpr kETag[] = "etag";
}  // namespace

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
void TrafficInfo::SerializeTrafficKeys(vector<RoadSegmentId> const & keys, vector<uint8_t> & result)
{
  vector<uint32_t> fids;
  vector<size_t> numSegs;
  vector<bool> oneWay;
  for (size_t i = 0; i < keys.size();)
  {
    size_t j = i;
    while (j < keys.size() && keys[i].m_fid == keys[j].m_fid)
      ++j;

    bool ow = true;
    for (size_t k = i; k < j; ++k)
    {
      if (keys[k].m_dir == RoadSegmentId::kReverseDirection)
      {
        ow = false;
        break;
      }
    }

    auto const numDirs = ow ? 1 : 2;
    size_t numSegsForThisFid = j - i;
    CHECK_GREATER(numDirs, 0, ());
    CHECK_EQUAL(numSegsForThisFid % numDirs, 0, ());
    numSegsForThisFid /= numDirs;

    fids.push_back(keys[i].m_fid);
    numSegs.push_back(numSegsForThisFid);
    oneWay.push_back(ow);

    i = j;
  }

  MemWriter<vector<uint8_t>> memWriter(result);
  WriteToSink(memWriter, kLatestKeysVersion);
  WriteVarUint(memWriter, fids.size());

  {
    BitWriter<decltype(memWriter)> bitWriter(memWriter);

    uint32_t prevFid = 0;
    for (auto const & fid : fids)
    {
      uint64_t const fidDiff = static_cast<uint64_t>(fid - prevFid);
      bool ok = coding::GammaCoder::Encode(bitWriter, fidDiff + 1);
      ASSERT(ok, ());
      UNUSED_VALUE(ok);
      prevFid = fid;
    }

    for (auto const & s : numSegs)
    {
      bool ok = coding::GammaCoder::Encode(bitWriter, s + 1);
      ASSERT(ok, ());
      UNUSED_VALUE(ok);
    }

    for (auto const val : oneWay)
      bitWriter.Write(val ? 1 : 0, 1 /* numBits */);
  }
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

// static
void TrafficInfo::DeserializeTrafficValues(vector<uint8_t> const & data, vector<SpeedGroup> & result)
{
  using Inflate = coding::ZLib::Inflate;

  vector<uint8_t> decompressedData;

  Inflate inflate(Inflate::Format::ZLib);
  inflate(data.data(), data.size(), back_inserter(decompressedData));

  MemReaderWithExceptions memReader(decompressedData.data(), decompressedData.size());
  ReaderSource<decltype(memReader)> src(memReader);

  auto const version = ReadPrimitiveFromSource<uint8_t>(src);
  CHECK_EQUAL(version, kLatestValuesVersion, ("Unsupported version of traffic keys."));

  auto const n = ReadVarUint<uint32_t>(src);
  result.resize(n);
  BitReader<decltype(src)> bitReader(src);
  for (size_t i = 0; i < static_cast<size_t>(n); ++i)
  {
    // SpeedGroup's values fit into 3 bits.
    result[i] = static_cast<SpeedGroup>(bitReader.Read(3));
  }

  ASSERT_EQUAL(src.Size(), 0, ());
}

bool TrafficInfo::UpdateTrafficData(vector<SpeedGroup> const & values)
{
  m_coloring.clear();

  if (m_keys.size() != values.size())
  {
    LOG(LWARNING, ("The number of received traffic values does not correspond to the number of keys:", m_keys.size(),
                   "keys", values.size(), "values."));
    m_availability = Availability::NoData;
    return false;
  }

  for (size_t i = 0; i < m_keys.size(); ++i)
    if (values[i] != SpeedGroup::Unknown)
      m_coloring.emplace(m_keys[i], values[i]);

  return true;
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
