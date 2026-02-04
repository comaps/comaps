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
std::string_view const kSequence = "sequence";

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

struct PanoramaxSequence
{
  std::string sequenceId;
  std::vector<PanoramaxPoint> points;
};

// Read a length-prefixed string from a binary stream
bool ReadString(std::ifstream & file, std::string & str)
{
  uint32_t length;
  file.read(reinterpret_cast<char*>(&length), sizeof(length));
  if (file.fail() || length > 100000)  // Sanity check
    return false;

  if (length > 0)
  {
    str.resize(length);
    file.read(&str[0], length);
  }
  else
  {
    str.clear();
  }
  return !file.fail();
}

// Load version 1 format (flat list of points, no sequences)
bool LoadPanoramaxPointsV1(std::ifstream & file, uint64_t pointCount, std::vector<PanoramaxSequence> & sequences)
{
  // In V1, each point is its own sequence (no grouping)
  sequences.reserve(static_cast<size_t>(pointCount));

  for (uint64_t i = 0; i < pointCount; ++i)
  {
    PanoramaxPoint point;

    file.read(reinterpret_cast<char*>(&point.lat), sizeof(point.lat));
    file.read(reinterpret_cast<char*>(&point.lon), sizeof(point.lon));

    // Read image_id (length-prefixed string)
    if (!ReadString(file, point.imageId))
    {
      LOG(LERROR, ("Error reading panoramax point", i));
      return false;
    }

    // Create a single-point sequence using image_id as sequence_id
    PanoramaxSequence seq;
    seq.sequenceId = point.imageId;
    seq.points.push_back(std::move(point));
    sequences.push_back(std::move(seq));
  }

  return true;
}

// Load version 2 format (points grouped by sequence)
bool LoadPanoramaxPointsV2(std::ifstream & file, uint64_t sequenceCount, std::vector<PanoramaxSequence> & sequences)
{
  sequences.reserve(static_cast<size_t>(sequenceCount));

  for (uint64_t s = 0; s < sequenceCount; ++s)
  {
    PanoramaxSequence seq;

    // Read sequence_id
    if (!ReadString(file, seq.sequenceId))
    {
      LOG(LERROR, ("Error reading sequence_id", s));
      return false;
    }

    // Read point count in this sequence
    uint64_t pointCount;
    file.read(reinterpret_cast<char*>(&pointCount), sizeof(pointCount));
    if (file.fail())
    {
      LOG(LERROR, ("Error reading point count for sequence", s));
      return false;
    }

    seq.points.reserve(static_cast<size_t>(pointCount));

    // Read points
    for (uint64_t p = 0; p < pointCount; ++p)
    {
      PanoramaxPoint point;

      file.read(reinterpret_cast<char*>(&point.lat), sizeof(point.lat));
      file.read(reinterpret_cast<char*>(&point.lon), sizeof(point.lon));

      if (!ReadString(file, point.imageId))
      {
        LOG(LERROR, ("Error reading image_id for sequence", s, "point", p));
        return false;
      }

      seq.points.push_back(std::move(point));
    }

    sequences.push_back(std::move(seq));
  }

  return true;
}

bool LoadPanoramaxData(std::string const & filePath, std::vector<PanoramaxSequence> & sequences)
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
    file.read(reinterpret_cast<char*>(&version), sizeof(version));

    if (version == 1)
    {
      // V1 format: version (4) + point_count (8) + points
      uint64_t pointCount;
      file.read(reinterpret_cast<char*>(&pointCount), sizeof(pointCount));
      LOG(LINFO, ("Loading panoramax v1 file with", pointCount, "points"));
      return LoadPanoramaxPointsV1(file, pointCount, sequences);
    }
    else if (version == 2)
    {
      // V2 format: version (4) + total_point_count (8) + sequence_count (8) + sequences
      uint64_t totalPointCount;
      uint64_t sequenceCount;
      file.read(reinterpret_cast<char*>(&totalPointCount), sizeof(totalPointCount));
      file.read(reinterpret_cast<char*>(&sequenceCount), sizeof(sequenceCount));
      LOG(LINFO, ("Loading panoramax v2 file with", totalPointCount, "points in", sequenceCount, "sequences"));
      return LoadPanoramaxPointsV2(file, sequenceCount, sequences);
    }
    else
    {
      LOG(LERROR, ("Unsupported panoramax file version", version));
      return false;
    }
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
  m_sequenceType = c.GetTypeByPath({kPanoramax, kSequence});
}

void PanoramaxFeaturesGenerator::GeneratePanoramax(std::string const & countryName,
                                                    FeaturesCollectFn const & fn) const
{
  auto const panoramaxPath = GetPanoramaxFilePath(countryName, m_panoramaxDir);

  std::vector<PanoramaxSequence> sequences;
  if (!LoadPanoramaxData(panoramaxPath, sequences))
  {
    LOG(LWARNING, ("Can't load panoramax data for", countryName));
    return;
  }

  size_t totalPoints = 0;
  size_t lineFeatures = 0;

  for (auto const & seq : sequences)
    totalPoints += seq.points.size();

  LOG(LINFO, ("Generating panoramax features for", countryName, ":", totalPoints, "points in", sequences.size(), "sequences"));

  for (auto const & seq : sequences)
  {
    // Generate point features for each image
    for (auto const & point : seq.points)
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

    // Generate line feature for sequences with 2+ points
    if (seq.points.size() >= 2)
    {
      feature::FeatureBuilder fb;

      // Build point sequence for the line
      feature::FeatureBuilder::PointSeq linePoints;
      linePoints.reserve(seq.points.size());

      for (auto const & point : seq.points)
      {
        m2::PointD const mercatorPoint = mercator::FromLatLon(point.lat, point.lon);
        linePoints.push_back(mercatorPoint);
      }

      fb.AssignPoints(std::move(linePoints));
      fb.AddType(m_sequenceType);
      fb.SetLinear();

      fn(std::move(fb));
      ++lineFeatures;
    }
  }

  LOG(LINFO, ("Generated", lineFeatures, "panoramax sequence lines for", countryName));
}
}  // namespace generator
