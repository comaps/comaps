#include "generator/panoramax_generator.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_meta.hpp"

#include "coding/file_reader.hpp"
#include "coding/read_write_utils.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <cstdint>
#include <fstream>

namespace generator
{
namespace
{
std::string_view const kPanoramax = "panoramax";
std::string_view const kImage = "image";

std::string GetPanoramaxFilePath(std::string const & countryName, std::string const & panoramaxDir)
{
  return panoramaxDir + "/" + countryName + ".panoramax";
}

struct PanoramaxPoint
{
  double lat;
  double lon;
  std::string imageId;
};

bool LoadPanoramaxPoints(std::string const & filePath, std::vector<PanoramaxPoint> & points)
{
  try
  {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
      LOG(LWARNING, ("Can't open panoramax file", filePath));
      return false;
    }

    // Read header
    uint32_t version;
    uint64_t pointCount;

    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    file.read(reinterpret_cast<char*>(&pointCount), sizeof(pointCount));

    if (version != 1)
    {
      LOG(LERROR, ("Unsupported panoramax file version", version));
      return false;
    }

    points.reserve(static_cast<size_t>(pointCount));

    // Read points
    for (uint64_t i = 0; i < pointCount; ++i)
    {
      PanoramaxPoint point;

      file.read(reinterpret_cast<char*>(&point.lat), sizeof(point.lat));
      file.read(reinterpret_cast<char*>(&point.lon), sizeof(point.lon));

      // Read image_id (length-prefixed string)
      uint32_t imageIdLength;
      file.read(reinterpret_cast<char*>(&imageIdLength), sizeof(imageIdLength));

      if (imageIdLength > 0 && imageIdLength < 10000)  // Sanity check
      {
        point.imageId.resize(imageIdLength);
        file.read(&point.imageId[0], imageIdLength);
      }

      if (file.fail())
      {
        LOG(LERROR, ("Error reading panoramax point", i, "from", filePath));
        return false;
      }

      points.push_back(std::move(point));
    }

    return true;
  }
  catch (std::exception const & e)
  {
    LOG(LERROR, ("Exception loading panoramax file", filePath, ":", e.what()));
    return false;
  }
}
}  // namespace

PanoramaxFeaturesGenerator::PanoramaxFeaturesGenerator(std::string const & panoramaxDir)
  : m_panoramaxDir(panoramaxDir)
{
  Classificator const & c = classif();
  m_panoramaxType = c.GetTypeByPath({kPanoramax, kImage});
}

void PanoramaxFeaturesGenerator::GeneratePanoramax(std::string const & countryName,
                                                    FeaturesCollectFn const & fn) const
{
  auto const panoramaxPath = GetPanoramaxFilePath(countryName, m_panoramaxDir);

  std::vector<PanoramaxPoint> points;
  if (!LoadPanoramaxPoints(panoramaxPath, points))
  {
    LOG(LWARNING, ("Can't load panoramax points for", countryName));
    return;
  }

  LOG(LINFO, ("Generating", points.size(), "panoramax points for", countryName));

  for (auto const & point : points)
  {
    feature::FeatureBuilder fb;

    // Set point geometry
    m2::PointD const mercatorPoint = mercator::FromLatLon(point.lat, point.lon);
    fb.SetCenter(mercatorPoint);

    // Add classificator type
    fb.AddType(m_panoramaxType);

    // Add metadata with image ID
    if (!point.imageId.empty())
    {
      fb.GetMetadata().Set(feature::Metadata::FMD_PANORAMAX, point.imageId);
    }

    fn(std::move(fb));
  }
}
}  // namespace generator
