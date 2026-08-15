#include "editor/osm_feature_lookup.hpp"

#include "base/exception.hpp"

#include "editor/feature_matcher.hpp"
#include "editor/osm_auth.hpp"
#include "editor/xml_feature.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "indexer/map_object.hpp"

// ReSharper disable once CppUnusedIncludeDirective
#include <pugixml.hpp>

#include <random>
#include <vector>

namespace
{
m2::RectD GetBoundingRect(std::vector<m2::PointD> const & geometry)
{
  m2::RectD rect;
  for (auto const & p : geometry)
  {
    auto const latLon = mercator::ToLatLon(p);
    rect.Add({latLon.m_lon, latLon.m_lat});
  }
  return rect;
}

bool OsmFeatureHasTags(pugi::xml_node const & osmFt)
{
  return osmFt.child("tag");
}

std::vector<m2::PointD> NaiveSample(std::vector<m2::PointD> const & source, size_t count)
{
  count = std::min(count, source.size());
  std::vector<m2::PointD> result;
  result.reserve(count);
  std::vector<size_t> indexes;
  indexes.reserve(count);

  std::random_device r;
  std::minstd_rand engine(r());
  std::uniform_int_distribution<size_t> distrib(0, source.size());

  while (count--)
  {
    size_t index;
    do
    {
      index = distrib(engine);
    }
    while (find(begin(indexes), end(indexes), index) != end(indexes));
    result.push_back(source[index]);
    indexes.push_back(index);
  }

  return result;
}

}  // namespace

namespace osm
{

void OsmFeatureLookup::LoadXmlFromOSM(ms::LatLon const & ll, pugi::xml_document & doc, double radiusInMeters) const
{
  auto const response = m_api.GetXmlFeaturesAtLatLon(ll.m_lat, ll.m_lon, radiusInMeters);
  if (response.first != OsmOAuth::HTTP::OK)
    MYTHROW(HttpErrorException, ("HTTP error", response, "with GetXmlFeaturesAtLatLon", ll));

  if (pugi::status_ok != doc.load_string(response.second.c_str()).status)
    MYTHROW(OsmXmlParseException,
            ("Can't parse OSM server response for GetXmlFeaturesAtLatLon request", response.second));
}

void OsmFeatureLookup::LoadXmlFromOSM(ms::LatLon const & min, ms::LatLon const & max, pugi::xml_document & doc) const
{
  auto const response = m_api.GetXmlFeaturesInRect(min.m_lat, min.m_lon, max.m_lat, max.m_lon);
  if (response.first != OsmOAuth::HTTP::OK)
    MYTHROW(HttpErrorException, ("HTTP error", response, "with GetXmlFeaturesInRect", min, max));

  if (pugi::status_ok != doc.load_string(response.second.c_str()).status)
    MYTHROW(OsmXmlParseException,
            ("Can't parse OSM server response for GetXmlFeaturesInRect request", response.second));
}

editor::XMLFeature OsmFeatureLookup::GetMatchingNodeFeatureFromOSM(m2::PointD const & center) const
{
  // Match with OSM node.
  ms::LatLon const ll = mercator::ToLatLon(center);
  pugi::xml_document doc;
  // Throws!
  LoadXmlFromOSM(ll, doc);

  pugi::xml_node const bestNode = matcher::GetBestOsmNode(doc, ll);
  if (bestNode.empty())
  {
    MYTHROW(OsmObjectWasDeletedException,
            ("OSM does not have any nodes at the coordinates", ll, ", server has returned:", doc));
  }

  if (!OsmFeatureHasTags(bestNode))
  {
    std::ostringstream sstr;
    bestNode.print(sstr);
    auto const strNode = sstr.str();
    LOG(LDEBUG, ("Node has no tags", strNode));
    MYTHROW(EmptyFeatureException, ("Node has no tags", strNode));
  }

  return {bestNode};
}

editor::XMLFeature OsmFeatureLookup::GetMatchingAreaFeatureFromOSM(std::vector<m2::PointD> const & geometry) const
{
  auto constexpr kSamplePointsCount = 3;
  bool hasRelation = false;
  // Try several points in case of poor osm response.
  for (auto const & pt : NaiveSample(geometry, kSamplePointsCount))
  {
    ms::LatLon const ll = mercator::ToLatLon(pt);
    pugi::xml_document doc;
    // Throws!
    LoadXmlFromOSM(ll, doc);

    if (doc.select_node("osm/relation"))
    {
      auto const rect = GetBoundingRect(geometry);
      LoadXmlFromOSM(ms::LatLon(rect.minY(), rect.minX()), ms::LatLon(rect.maxY(), rect.maxX()), doc);
      hasRelation = true;
    }

    pugi::xml_node const bestWayOrRelation = matcher::GetBestOsmWayOrRelation(doc, geometry);
    if (!bestWayOrRelation)
    {
      if (hasRelation)
        break;
      continue;
    }

    if (!OsmFeatureHasTags(bestWayOrRelation))
    {
      std::ostringstream sstr;
      bestWayOrRelation.print(sstr);
      LOG(LDEBUG, ("The matched object has no tags", sstr.str()));
      MYTHROW(EmptyFeatureException, ("The matched object has no tags"));
    }

    return {bestWayOrRelation};
  }
  MYTHROW(OsmObjectWasDeletedException, ("OSM does not have any matching way for feature"));
}

}  // namespace osm
