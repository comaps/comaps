#pragma once

#include "editor/osm_auth.hpp"
#include "editor/server_api.hpp"
#include "editor/xml_feature.hpp"

#include "base/exception.hpp"

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"

// ReSharper disable once CppUnusedIncludeDirective
#include "indexer/map_object.hpp"

// ReSharper disable once CppUnusedIncludeDirective
#include <pugixml.hpp>

#include <vector>

namespace osm
{
/// Calls OSM API to find OSM entities that match supplied geometry. Used to
/// identify OSM entities to which the edits should be applied.
class OsmFeatureLookup
{
public:
  DECLARE_EXCEPTION(OsmFeatureLookupException, RootException);
  DECLARE_EXCEPTION(HttpErrorException, OsmFeatureLookupException);
  DECLARE_EXCEPTION(OsmXmlParseException, OsmFeatureLookupException);
  DECLARE_EXCEPTION(OsmObjectWasDeletedException, OsmFeatureLookupException);
  DECLARE_EXCEPTION(EmptyFeatureException, OsmFeatureLookupException);

  // use ServerAuth without logged in user info - we will only be invoking public APIs
  OsmFeatureLookup() : m_api(OsmOAuth::ServerAuth()) {}

  /// Throws many exceptions from above list, plus including XMLNode's parsing ones.
  /// OsmObjectWasDeletedException means that node was deleted from OSM server by someone else.
  [[nodiscard]] editor::XMLFeature GetMatchingFeatureFromOSM(osm::MapObject const & o) const;
  [[nodiscard]] editor::XMLFeature GetMatchingNodeFeatureFromOSM(m2::PointD const & center) const;
  [[nodiscard]] editor::XMLFeature GetMatchingAreaFeatureFromOSM(std::vector<m2::PointD> const & geometry) const;

private:
  /// Unfortunately, pugi can't return xml_documents from methods.
  /// Throws exceptions from above list.
  void LoadXmlFromOSM(ms::LatLon const & ll, pugi::xml_document & doc, double radiusInMeters = 1.0) const;
  void LoadXmlFromOSM(ms::LatLon const & min, ms::LatLon const & max, pugi::xml_document & doc) const;

  ServerApi06 m_api;
};

}  // namespace osm
