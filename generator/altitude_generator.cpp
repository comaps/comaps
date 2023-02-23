#include "generator/altitude_generator.hpp"
#include "generator/srtm_parser.hpp"

#include "routing/routing_helpers.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_altitude.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_processor.hpp"

#include "coding/files_container.hpp"
#include "coding/succinct_mapper.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"

#include "defines.hpp"

#include "3party/succinct/elias_fano.hpp"
#include "3party/succinct/rs_bit_vector.hpp"

namespace routing
{
using namespace feature;
using namespace geometry;

namespace
{
class SrtmGetter : public AltitudeGetter
{
public:
  explicit SrtmGetter(std::string const & srtmDir) : m_srtmManager(srtmDir) {}

  // AltitudeGetter overrides:
  Altitude GetAltitude(m2::PointD const & p) override
  {
    return m_srtmManager.GetAltitude(mercator::ToLatLon(p));
  }

  void PrintStatsAndPurge() override
  {
    LOG(LINFO, ("Srtm tiles number (x26Mb):", m_srtmManager.GeTilesNumber()));
    m_srtmManager.Purge();
  }

private:
  generator::SrtmTileManager m_srtmManager;
};

class Processor
{
public:
  struct FeatureAltitude
  {
    FeatureAltitude(uint32_t featureId, geometry::Altitudes && altitudes)
      : m_featureId(featureId), m_altitudes(std::move(altitudes))
    {
    }

    uint32_t m_featureId;
    feature::Altitudes m_altitudes;
  };

  explicit Processor(AltitudeGetter & altitudeGetter)
    : m_minAltitude(geometry::kInvalidAltitude), m_altitudeGetter(altitudeGetter)
  {
  }

  void operator()(FeatureType & f, uint32_t id)
  {
    CHECK_EQUAL(f.GetID().m_index, id, ());
    CHECK_EQUAL(id, m_altitudeAvailabilityBuilder.size(), ());

    bool hasAltitude = false;
    SCOPE_GUARD(altitudeAvailabilityBuilding,
                [&]() { m_altitudeAvailabilityBuilder.push_back(hasAltitude); });

    if (!routing::IsRoad(feature::TypesHolder(f)))
      return;

    f.ParseGeometry(FeatureType::BEST_GEOMETRY);
    size_t const pointsCount = f.GetPointsCount();
    if (pointsCount == 0)
      return;

    geometry::Altitudes altitudes;
    altitudes.reserve(pointsCount);
    Altitude minFeatureAltitude = geometry::kInvalidAltitude;
    for (size_t i = 0; i < pointsCount; ++i)
    {
      auto const & pt = f.GetPoint(i);
      Altitude const a = m_altitudeGetter.GetAltitude(pt);
      if (a == geometry::kInvalidAltitude)
      {
        // Print warning for missing altitude point (if not a ferry or so).
        auto const type = CarModel::AllLimitsInstance().GetHighwayType(feature::TypesHolder(f));
        if (type && *type != HighwayType::RouteFerry && *type != HighwayType::RouteShuttleTrain)
          LOG(LWARNING, ("Invalid altitude at:", mercator::ToLatLon(pt)));

        // One invalid point invalidates the whole feature.
        return;
      }

      if (minFeatureAltitude == geometry::kInvalidAltitude)
        minFeatureAltitude = a;
      else
        minFeatureAltitude = std::min(minFeatureAltitude, a);

      altitudes.push_back(a);
    }

    hasAltitude = true;
    m_featureAltitudes.emplace_back(id, std::move(altitudes));

    if (m_minAltitude == geometry::kInvalidAltitude)
      m_minAltitude = minFeatureAltitude;
    else
      m_minAltitude = std::min(minFeatureAltitude, m_minAltitude);
  }

  bool HasAltitudeInfo() const { return !m_featureAltitudes.empty(); }

public:
  std::vector<FeatureAltitude> m_featureAltitudes;
  succinct::bit_vector_builder m_altitudeAvailabilityBuilder;
  Altitude m_minAltitude;

  AltitudeGetter & m_altitudeGetter;
};
}  // namespace

void BuildRoadAltitudes(std::string const & mwmPath, AltitudeGetter & altitudeGetter)
{
  try
  {
    // Preparing altitude information.
    Processor processor(altitudeGetter);
    feature::ForEachFeature(mwmPath, processor);
    processor.m_altitudeGetter.PrintStatsAndPurge();

    if (!processor.HasAltitudeInfo())
    {
      // Possible for small islands like Bouvet or Willis.
      LOG(LWARNING, ("No altitude information for road features of mwm:", mwmPath));
      return;
    }

    FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
    auto w = cont.GetWriter(ALTITUDES_FILE_TAG);

    AltitudeHeader header;
    header.m_minAltitude = processor.m_minAltitude;

    auto const startOffset = w->Pos();
    header.Serialize(*w);
    {
      // Altitude availability serialization.
      coding::FreezeVisitor<Writer> visitor(*w);
      succinct::rs_bit_vector(&processor.m_altitudeAvailabilityBuilder).map(visitor);
    }
    header.m_featureTableOffset = base::checked_cast<uint32_t>(w->Pos() - startOffset);

    std::vector<uint32_t> offsets;
    std::vector<uint8_t> deltas;
    {
      // Altitude info serialization to memory.
      MemWriter<std::vector<uint8_t>> writer(deltas);
      for (auto const & a : processor.m_featureAltitudes)
      {
        offsets.push_back(base::checked_cast<uint32_t>(writer.Pos()));
        a.m_altitudes.Serialize(header.m_minAltitude, writer);
      }
    }
    {
      // Altitude offsets serialization.
      CHECK(base::IsSortedAndUnique(offsets.begin(), offsets.end()), ());

      succinct::elias_fano::elias_fano_builder builder(offsets.back(), offsets.size());
      for (uint32_t offset : offsets)
        builder.push_back(offset);

      coding::FreezeVisitor<Writer> visitor(*w);
      succinct::elias_fano(&builder).map(visitor);
    }
    // Writing altitude info.
    header.m_altitudesOffset = base::checked_cast<uint32_t>(w->Pos() - startOffset);
    w->Write(deltas.data(), deltas.size());
    w->WritePaddingByEnd(8);
    header.m_endOffset = base::checked_cast<uint32_t>(w->Pos() - startOffset);

    // Rewriting header info.
    auto const endOffset = w->Pos();
    w->Seek(startOffset);
    header.Serialize(w);
    w->Seek(endOffset);

    LOG(LINFO, (ALTITUDES_FILE_TAG, "section is ready. The size is", header.m_endOffset));
    if (processor.HasAltitudeInfo())
      LOG(LINFO, ("Min altitude is", processor.m_minAltitude));
    else
      LOG(LINFO, ("Min altitude isn't defined."));
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("An exception happened while creating", ALTITUDES_FILE_TAG, "section:", e.what()));
  }
}

void BuildRoadAltitudes(std::string const & mwmPath, std::string const & srtmDir)
{
  LOG(LINFO, ("mwmPath =", mwmPath, "srtmDir =", srtmDir));
  SrtmGetter srtmGetter(srtmDir);
  BuildRoadAltitudes(mwmPath, srtmGetter);
}
}  // namespace routing
