#include "generator/maxspeeds_builder.hpp"

#include "generator/maxspeeds_parser.hpp"
#include "generator/routing_helpers.hpp"

#include "routing/index_graph.hpp"
#include "routing/maxspeeds_serialization.hpp"
#include "routing/routing_helpers.hpp"

#include "routing_common/car_model_coefs.hpp"
#include "routing_common/maxspeed_conversion.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_processor.hpp"

#include "coding/files_container.hpp"
#include "coding/file_writer.hpp"

#include "platform/measurement_utils.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>

#include "defines.hpp"

namespace routing_builder
{
using namespace feature;
using namespace generator;
using namespace routing;
using std::string;

char constexpr kDelim[] = ", \t\r\n";
double constexpr kMinDefSpeedRoadsLengthKm = 5.0;
double constexpr kMaxPossibleDefSpeedKmH = 400.0;
// This factor should be greater than sqrt(2) / 2 - prefer diagonal link to square path.
double constexpr kLinkToMainSpeedFactor = 0.85;

template <class TokenizerT> bool ParseOneSpeedValue(TokenizerT & iter, MaxspeedType & value)
{
  if (!iter)
    return false;

  uint64_t parsedSpeed = 0;
  if (!strings::to_uint(*iter, parsedSpeed))
    return false;
  if (parsedSpeed > routing::kInvalidSpeed)
    return false;
  value = static_cast<MaxspeedType>(parsedSpeed);
  ++iter;
  return true;
}

/// \brief Collects all maxspeed tag values of specified mwm based on maxspeeds.csv file.
class MaxspeedsMwmCollector
{
  string const & m_dataPath;
  FeatureIdToOsmId const & m_ft2osm;
  IndexGraph * m_graph;

  MaxspeedConverter const & m_converter;

  std::vector<MaxspeedsSerializer::FeatureSpeedMacro> m_maxspeeds;

  struct AvgInfo
  {
    double m_lengthKM = 0;
    double m_timeH = 0;

    double m_speed = -1;  // invalid initial value

    friend std::string DebugPrint(AvgInfo const & i)
    {
      std::ostringstream ss;
      ss << "AvgInfo{ " << i.m_speed << ", " << i.m_lengthKM << ", " << i.m_timeH << " }";
      return ss.str();
    }
  };

  static int constexpr kSpeedsCount = MaxspeedsSerializer::DEFAULT_SPEEDS_COUNT;
  static int constexpr kOutsideCityIdx = 0;
  // 0 - outside a city; 1 - inside a city.
  std::unordered_map<HighwayType, AvgInfo> m_avgSpeeds[kSpeedsCount];

  base::GeoObjectId GetOsmID(uint32_t fid) const
  {
    auto const osmIdIt = m_ft2osm.find(fid);
    if (osmIdIt == m_ft2osm.cend())
      return base::GeoObjectId();
    return osmIdIt->second;
  }

  routing::RoadGeometry const & GetRoad(uint32_t fid) const
  {
    return m_graph->GetRoadGeometry(fid);
  }

  // OSM data related warning tag for convenient grep.
  std::string m_logTag;

public:
  MaxspeedsMwmCollector(string const & dataPath, FeatureIdToOsmId const & ft2osm, IndexGraph * graph)
    : m_dataPath(dataPath), m_ft2osm(ft2osm), m_graph(graph)
    , m_converter(MaxspeedConverter::Instance())
    , m_logTag("SpeedsBuilder")
  {
  }

  void Process(string const & maxspeedCsvPath)
  {
    OsmIdToMaxspeed osmIdToMaxspeed;
    CHECK(ParseMaxspeeds(maxspeedCsvPath, osmIdToMaxspeed), (maxspeedCsvPath));

    auto const GetSpeed = [&](uint32_t fid) -> Maxspeed *
    {
      auto const osmId = GetOsmID(fid);
      if (osmId.GetType() == base::GeoObjectId::Type::Invalid)
        return nullptr;

      auto const maxspeedIt = osmIdToMaxspeed.find(osmId);
      if (maxspeedIt == osmIdToMaxspeed.cend())
        return nullptr;

      return &maxspeedIt->second;
    };
    auto const GetLastIndex = [&](uint32_t fid)
    {
      return GetRoad(fid).GetPointsCount() - 2;
    };
    auto const GetOpposite = [&](Segment const & seg)
    {
      // Assume that links are connected with main roads in first or last point, always.
      uint32_t const fid = seg.GetFeatureId();
      return Segment(0, fid, seg.GetSegmentIdx() > 0 ? 0 : GetLastIndex(fid), seg.IsForward());
    };
    auto const GetHighwayType = [&](uint32_t fid)
    {
      return GetRoad(fid).GetHighwayType();
    };

    auto const & converter = GetMaxspeedConverter();
    using HwTypeT = std::optional<routing::HighwayType>;
    auto const CalculateSpeed = [&](uint32_t parentFID, Maxspeed const & s, HwTypeT hwType) -> std::optional<SpeedInUnits>
    {
      HwTypeT const parentHwType = GetHighwayType(parentFID);
      if (!parentHwType)
        return {};

      // Set speed as-is from parent link.
      if (parentHwType == hwType)
        return {{s.GetForward(), s.GetUnits()}};

      using routing::HighwayType;
      if ((*parentHwType == HighwayType::HighwayMotorway && hwType == HighwayType::HighwayMotorwayLink) ||
          (*parentHwType == HighwayType::HighwayTrunk && hwType == HighwayType::HighwayTrunkLink) ||
          (*parentHwType == HighwayType::HighwayPrimary && hwType == HighwayType::HighwayPrimaryLink) ||
          (*parentHwType == HighwayType::HighwaySecondary && hwType == HighwayType::HighwaySecondaryLink) ||
          (*parentHwType == HighwayType::HighwayTertiary && hwType == HighwayType::HighwayTertiaryLink))
      {
        // Reduce factor from parent road. See DontUseLinksWhenRidingOnMotorway test.
        return converter.ClosestValidMacro(
              { base::asserted_cast<MaxspeedType>(std::lround(s.GetForward() * kLinkToMainSpeedFactor)), s.GetUnits() });
      }

      return {};
    };

    ForEachFeature(m_dataPath, [&](FeatureType & ft, uint32_t fid)
    {
      if (!routing::IsCarRoad(TypesHolder(ft)))
        return;

      Maxspeed * maxSpeed = GetSpeed(fid);
      if (!maxSpeed)
          return;

      auto const osmID = GetOsmID(fid).GetSerialId();

      if (ft.GetGeomType() != GeomType::Line)
      {
        LOG(LWARNING, ("Non linear road with speed for way", osmID));
        return;
      }

#define LOG_MAX_SPEED(msg) if (false) LOG(LINFO, msg)

      LOG_MAX_SPEED(("Start osmid =", osmID));

      // Recalculate link speed according to the ingoing highway.
      // See MaxspeedsCollector::CollectFeature.
      if (maxSpeed->GetForward() == routing::kCommonMaxSpeedValue)
      {
        // Check if we are in unit tests.
        if (m_graph == nullptr)
          return;

        // 0 - not updated, 1 - goto next iteration, 2 - updated
        int status = 0;

        HwTypeT const hwType = GetHighwayType(fid);

        // Check ingoing first, then - outgoing.
        for (bool direction : { false, true })
        {
          LOG_MAX_SPEED(("Search dir =", direction));

          Segment seg(0, fid, 0, true);
          if (direction)
            seg = GetOpposite(seg);

          std::unordered_set<uint32_t> reviewed;
          do
          {
            LOG_MAX_SPEED(("Input seg =", seg));

            status = 0;
            reviewed.insert(seg.GetFeatureId());

            IndexGraph::SegmentEdgeListT edges;
            m_graph->GetEdgeList(seg, direction, false /* useRoutingOptions */, edges);
            for (auto const & e : edges)
            {
              LOG_MAX_SPEED(("Edge =", e));
              Segment const target = e.GetTarget();

              uint32_t const targetFID = target.GetFeatureId();
              uint64_t const targetOsmID = GetOsmID(targetFID).GetSerialId();
              LOG_MAX_SPEED(("Edge target =", target, "; osmid =", targetOsmID));

              Maxspeed const * s = GetSpeed(targetFID);
              if (s)
              {
                if (routing::IsNumeric(s->GetForward()))
                {
                  auto const speed = CalculateSpeed(targetFID, *s, hwType);
                  if (speed)
                  {
                    status = 2;

                    maxSpeed->SetForward(speed->GetSpeed());
                    maxSpeed->SetUnits(speed->GetUnits());

                    LOG(LINFO, ("Updated link speed for way", osmID, "with", *maxSpeed, "from", targetOsmID));
                    break;
                  }
                }
                else if (s->GetForward() == routing::kCommonMaxSpeedValue &&
                         reviewed.size() < 4 &&   // limit with some reasonable transitions
                         reviewed.find(targetFID) == reviewed.end() &&
                         hwType == GetHighwayType(targetFID))
                {
                  LOG_MAX_SPEED(("Add reviewed"));

                  // Found another input link. Save it for the next iteration.
                  status = 1;
                  seg = GetOpposite(target);
                }
              }
            }
          } while (status == 1);

          if (status == 2)
            break;
        }

        if (status == 0)
        {
          LOG(LWARNING, ("Didn't find connected edge with speed for way", osmID));
          return;
        }
      }

      LOG_MAX_SPEED(("End osmid =", osmID));

      AddSpeed(fid, osmID, *maxSpeed);
    });
  }

private:
  void AddSpeed(uint32_t featureID, uint64_t osmID, Maxspeed const & speed)
  {
    MaxspeedType constexpr kMaxReasonableSpeed = 280;
    if ((speed.GetSpeedKmPH(true) >= kMaxReasonableSpeed) ||
        (speed.IsBidirectional() && speed.GetSpeedKmPH(false) >= kMaxReasonableSpeed))
    {
      LOG(LWARNING, (m_logTag, "Very big speed", speed, "for way", osmID));
    }

    // Add converted macro speed.
    SpeedInUnits const forward(speed.GetForward(), speed.GetUnits());
    CHECK(forward.IsValid(), ());
    SpeedInUnits const backward(speed.GetBackward(), speed.GetUnits());

    /// @todo Should we find closest macro speed here when Undefined? OSM has bad data sometimes.
    SpeedMacro const backwardMacro = m_converter.SpeedToMacro(backward);
    MaxspeedsSerializer::FeatureSpeedMacro ftSpeed{featureID, m_converter.SpeedToMacro(forward), backwardMacro};

    if (ftSpeed.m_forward == SpeedMacro::Undefined)
    {
      LOG(LWARNING, (m_logTag, "Undefined forward speed macro", forward, "for way", osmID));
      return;
    }
    if (backward.IsValid() && backwardMacro == SpeedMacro::Undefined)
    {
      LOG(LWARNING, (m_logTag, "Undefined backward speed macro", backward, "for way", osmID));
    }

    m_maxspeeds.push_back(ftSpeed);

    // Possible in unit tests.
    if (m_graph == nullptr)
      return;

    // Update average speed information.
    auto const & rd = GetRoad(featureID);

    auto const hwType = rd.GetHighwayType();
    if (hwType)
    {
      auto & info = m_avgSpeeds[rd.IsInCity() ? 1 : 0][*hwType];

      double const lenKM = rd.GetRoadLengthM() / 1000.0;
      for (auto const & s : { forward, backward })
      {
        if (s.IsNumeric())
        {
          info.m_lengthKM += lenKM;
          info.m_timeH += lenKM / s.GetSpeedKmPH();
        }
      }
    }
    else
      LOG(LWARNING, (m_logTag, "Undefined HighwayType for way", osmID));
  }

public:
  void CalculateDefaultTypeSpeeds(MaxspeedsSerializer::HW2SpeedMap typeSpeeds[])
  {
    std::vector<std::pair<HighwayType, InOutCitySpeedKMpH>> baseSpeeds(
        kHighwayBasedSpeeds.begin(), kHighwayBasedSpeeds.end());
    // Remove links, because they don't conform speed consistency.
    baseSpeeds.erase(std::remove_if(baseSpeeds.begin(), baseSpeeds.end(), [](auto const & e)
    {
      return (e.first == HighwayType::HighwayMotorwayLink ||
              e.first == HighwayType::HighwayTrunkLink ||
              e.first == HighwayType::HighwayPrimaryLink ||
              e.first == HighwayType::HighwaySecondaryLink ||
              e.first == HighwayType::HighwayTertiaryLink);
    }), baseSpeeds.end());

    for (int ind = 0; ind < kSpeedsCount; ++ind)
    {
      // Calculate average speed.
      for (auto & e : m_avgSpeeds[ind])
      {
        // Check some reasonable conditions when assigning average speed.
        if (e.second.m_lengthKM > kMinDefSpeedRoadsLengthKm)
        {
          auto const speed = e.second.m_lengthKM / e.second.m_timeH;
          if (speed < kMaxPossibleDefSpeedKmH)
            e.second.m_speed = speed;
        }
      }

      // Prepare ethalon vector.
      bool const inCity = ind != kOutsideCityIdx;
      std::sort(baseSpeeds.begin(), baseSpeeds.end(), [inCity](auto const & l, auto const & r)
      {
        // Sort from biggest to smallest.
        return r.second.GetSpeed(inCity).m_weight < l.second.GetSpeed(inCity).m_weight;
      });

      // First of all check that calculated speed and base speed difference is less than 2x.
      for (auto const & e : baseSpeeds)
      {
        auto & l = m_avgSpeeds[ind][e.first];
        if (l.m_speed > 0)
        {
          double const base = e.second.GetSpeed(inCity).m_weight;
          double const factor = l.m_speed / base;
          if (factor > 2 || factor < 0.5)
          {
            LOG(LWARNING, (m_logTag, "More than 2x diff:", e.first, l.m_speed, base));
            l.m_speed = -1;
          }
        }
      }

      // Check speed's pairs consistency.
      // Constraints from the previous iteration can be broken if we modify l-speed on the next iteration.
      for (size_t il = 0, ir = 1; ir < baseSpeeds.size(); ++ir)
      {
        auto & l = m_avgSpeeds[ind][baseSpeeds[il].first];
        if (l.m_speed < 0)
        {
          ++il;
          continue;
        }
        auto & r = m_avgSpeeds[ind][baseSpeeds[ir].first];
        if (r.m_speed < 0)
          continue;

        // |l| should be greater than |r|
        if (l.m_speed < r.m_speed)
        {
          LOG(LWARNING, (m_logTag, "Bad def speeds pair:", baseSpeeds[il].first, baseSpeeds[ir].first, l, r));

          if (l.m_lengthKM >= r.m_lengthKM)
            r.m_speed = l.m_speed;
          else
            l.m_speed = r.m_speed;
        }

        il = ir;
      }

      auto const getSpeed = [this, ind, inCity](HighwayType type)
      {
        auto const s = m_avgSpeeds[ind][type].m_speed;
        if (s > 0)
          return s;
        auto const * p = kHighwayBasedSpeeds.Find(type);
        CHECK(p, ());
        return p->GetSpeed(inCity).m_weight;
      };

      // These speeds: Primary, Secondary, Tertiary, Residential have the biggest routing quality impact.
      {
        double const primaryS = getSpeed(HighwayType::HighwayPrimary);
        double const secondaryS = getSpeed(HighwayType::HighwaySecondary);
        double const tertiaryS = getSpeed(HighwayType::HighwayTertiary);
        double const residentialS = getSpeed(HighwayType::HighwayResidential);
        double constexpr eps = 1.0;
        if (primaryS + eps < secondaryS || secondaryS + eps < tertiaryS || tertiaryS + eps < residentialS)
        {
          LOG(LWARNING, (m_logTag, "Ignore primary, secondary, tertiary, residential speeds:",
                                    primaryS, secondaryS, tertiaryS, residentialS));

          m_avgSpeeds[ind][HighwayType::HighwayPrimary].m_speed = -1;
          m_avgSpeeds[ind][HighwayType::HighwaySecondary].m_speed = -1;
          m_avgSpeeds[ind][HighwayType::HighwayTertiary].m_speed = -1;
          m_avgSpeeds[ind][HighwayType::HighwayResidential].m_speed = -1;
        }
      }

      // Update links.
      std::pair<HighwayType, HighwayType> arrLinks[] = {
        {HighwayType::HighwayMotorway, HighwayType::HighwayMotorwayLink},
        {HighwayType::HighwayTrunk, HighwayType::HighwayTrunkLink},
        {HighwayType::HighwayPrimary, HighwayType::HighwayPrimaryLink},
        {HighwayType::HighwaySecondary, HighwayType::HighwaySecondaryLink},
        {HighwayType::HighwayTertiary, HighwayType::HighwayTertiaryLink},
      };
      for (auto const & e : arrLinks)
      {
        auto const main = m_avgSpeeds[ind][e.first].m_speed;
        auto & link = m_avgSpeeds[ind][e.second].m_speed;
        if (main > 0)
          link = kLinkToMainSpeedFactor * main;
        else
          link = -1;
      }

      // Fill type-speed map.
      LOG(LINFO, ("Average speeds", ind == kOutsideCityIdx ? "outside" : "inside", "a city:"));
      for (auto const & e : m_avgSpeeds[ind])
      {
        if (e.second.m_speed > 0)
        {
          // Store type speeds in Metric system, like VehicleModel profiles.
          auto const speedInUnits = m_converter.ClosestValidMacro(
                { static_cast<MaxspeedType>(e.second.m_speed), measurement_utils::Units::Metric });

          LOG(LINFO, ("*", e.first, "=", speedInUnits));

          typeSpeeds[ind][e.first] = m_converter.SpeedToMacro(speedInUnits);
        }
      }
    }
  }

  void SerializeMaxspeeds()
  {
    if (m_maxspeeds.empty())
      return;

    MaxspeedsSerializer::HW2SpeedMap typeSpeeds[kSpeedsCount];
    /// @todo There are too many claims/bugs with Turkey calculated defaults.
    /// And yes, now this dummy country check :)
    if (m_dataPath.find("Turkey_") == std::string::npos)
      CalculateDefaultTypeSpeeds(typeSpeeds);

    // Serialize speeds.
    FilesContainerW cont(m_dataPath, FileWriter::OP_WRITE_EXISTING);
    auto writer = cont.GetWriter(MAXSPEEDS_FILE_TAG);
    MaxspeedsSerializer::Serialize(m_maxspeeds, typeSpeeds, *writer);

    LOG(LINFO, ("Serialized", m_maxspeeds.size(), "speeds for", m_dataPath));
  }
};

bool ParseMaxspeeds(string const & filePath, OsmIdToMaxspeed & osmIdToMaxspeed)
{
  osmIdToMaxspeed.clear();

  std::ifstream stream(filePath);
  if (!stream)
    return false;

  string line;
  while (stream.good())
  {
    getline(stream, line);
    strings::SimpleTokenizer iter(line, kDelim);

    if (!iter)  // empty line
      continue;

    uint64_t osmId = 0;
    if (!strings::to_uint(*iter, osmId))
      return false;
    ++iter;

    if (!iter)
      return false;
    Maxspeed speed;
    speed.SetUnits(StringToUnits(*iter));
    ++iter;

    MaxspeedType forward = 0;
    if (!ParseOneSpeedValue(iter, forward))
      return false;

    speed.SetForward(forward);

    if (iter)
    {
      // There's backward maxspeed limit.
      MaxspeedType backward = 0;
      if (!ParseOneSpeedValue(iter, backward))
        return false;

      speed.SetBackward(backward);

      if (iter)
        return false;
    }

    auto const res = osmIdToMaxspeed.emplace(base::MakeOsmWay(osmId), speed);
    if (!res.second)
      return false;
  }
  return true;
}

void BuildMaxspeedsSection(IndexGraph * graph, string const & dataPath,
                           FeatureIdToOsmId const & featureIdToOsmId,
                           string const & maxspeedsFilename)
{
  MaxspeedsMwmCollector collector(dataPath, featureIdToOsmId, graph);

  collector.Process(maxspeedsFilename);
  collector.SerializeMaxspeeds();
}

void BuildMaxspeedsSection(IndexGraph * graph, string const & dataPath,
                           string const & osmToFeaturePath, string const & maxspeedsFilename)
{
  FeatureIdToOsmId featureIdToOsmId;
  ParseWaysFeatureIdToOsmIdMapping(osmToFeaturePath, featureIdToOsmId);
  BuildMaxspeedsSection(graph, dataPath, featureIdToOsmId, maxspeedsFilename);
}
}  // namespace routing_builder
