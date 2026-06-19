#include "generator/borders.hpp"

#include "generator/tesselator.hpp"

#include "platform/platform.hpp"

#include "storage/country_decl.hpp"

#include "indexer/scales.hpp"

#include "coding/files_container.hpp"
#include "coding/geometry_coding.hpp"
#include "coding/point_coding.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/varint.hpp"

#include "geometry/mercator.hpp"
#include "geometry/simplification.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <vector>

#include "defines.hpp"

namespace borders
{
namespace
{

template <class ToDo>
void ForEachCountry(std::string const & baseDir, ToDo && toDo)
{
  std::string const bordersDir = base::JoinPath(baseDir, BORDERS_DIR);
  CHECK(Platform::IsFileExistsByFullPath(bordersDir), ("Cannot read borders directory", bordersDir));

  Platform::FilesList files;
  Platform::GetFilesByExt(bordersDir, BORDERS_EXTENSION, files);
  for (std::string file : files)
  {
    PolygonsList polygons;
    if (LoadBorders(bordersDir + file, polygons))
    {
      base::GetNameWithoutExt(file);
      toDo(std::move(file), std::move(polygons));
    }
  }
}

class PackedBordersGenerator
{
public:
  explicit PackedBordersGenerator(std::string const & baseDir) : m_writer(baseDir + PACKED_POLYGONS_FILE) {}

  void operator()(std::string name, PolygonsList && borders)
  {
    LOG(LDEBUG, ("[BORDERS_GENERATOR] Processing country:", name, "with", borders.size(), "borders"));
    
    // calc rect
    m2::RectD rect;
    for (m2::RegionD const & border : borders)
      rect.Add(border.GetRect());

    // store polygon info
    m_polys.push_back(storage::CountryDef(name, rect));
    size_t const countryIndex = m_polys.size() - 1;
    LOG(LDEBUG, ("[BORDERS_GENERATOR] Assigned country index:", countryIndex, "to country:", name));

    // Store simplified polygons for later triangle generation
    std::vector<std::vector<m2::PointD>> simplifiedPolygons;
    for (m2::RegionD const & border : borders)
    {
      std::vector<m2::PointD> const & in = border.Data();
      std::vector<m2::PointD> out;

      /// @todo Figure out best scale level
      SimplifyDefault(in.begin(), in.end(), math::Pow2(scales::GetEpsilonForSimplify(9)), out);

      simplifiedPolygons.push_back(out);
    }

    // Compute proper coding params from first point of first polygon
    serial::GeometryCodingParams cp;
    if (!simplifiedPolygons.empty() && !simplifiedPolygons[0].empty())
      cp.SetBasePoint(simplifiedPolygons[0][0]);

    // Store data for triangle generation phase
    m_triangleData.emplace_back(TriangleGenerationData{countryIndex, simplifiedPolygons, cp});
  }

  void WritePolygonsInfo()
  {
    for (size_t i = 0; i < m_polys.size(); i++)
    {
      auto w = m_writer.GetWriter(strings::to_string(i));
      if (!w)
      {
        LOG(LERROR, ("[BORDERS_GENERATOR] Failed to create writer for polygon tag", i));
        continue;
      }
      
      // Use the proper coding params stored in triangle data
      serial::GeometryCodingParams const & cp = m_triangleData[i].m_cp;
      
      WriteVarUint(w, 1U); // Write 1 polygon for simplicity
      if (!m_triangleData[i].m_simplifiedPolygons.empty() && !m_triangleData[i].m_simplifiedPolygons[0].empty())
        serial::SaveOuterPath(m_triangleData[i].m_simplifiedPolygons[0], cp, *w);
    }
    
    for (auto const & data : m_triangleData)
    {
      GenerateAndSaveTriangles(data.m_simplifiedPolygons, data.m_cp, data.m_countryIndex);
    }
    
    auto w = m_writer.GetWriter(PACKED_POLYGONS_INFO_TAG);
    if (!w)
    {
      LOG(LERROR, ("[BORDERS_GENERATOR] Failed to create writer for info tag"));
      return;
    }
    rw::Write(*w, m_polys);
  }

private:
  FilesContainerW m_writer;

  std::vector<storage::CountryDef> m_polys;
  
  struct TriangleGenerationData
  {
    size_t m_countryIndex;
    std::vector<std::vector<m2::PointD>> m_simplifiedPolygons;
    serial::GeometryCodingParams m_cp;
  };
  
  std::vector<TriangleGenerationData> m_triangleData;

  void GenerateAndSaveTriangles(std::vector<std::vector<m2::PointD>> const & simplifiedPolygons,
                                serial::GeometryCodingParams const & cp, size_t countryIndex)
  {
    LOG(LDEBUG, ("[BORDERS_GENERATOR] GenerateAndSaveTriangles called for country index:", countryIndex));

    // Validation: Check that we have valid simplified polygons
    if (simplifiedPolygons.empty())
    {
      LOG(LWARNING, ("[BORDERS_GENERATOR] No simplified polygons for country index", countryIndex));
      return;
    }

    tesselator::PolygonsT polygons;
    for (auto const & polygon : simplifiedPolygons)
    {
      if (!polygon.empty())
      {
        polygons.push_back(polygon);
        LOG(LDEBUG, ("[BORDERS_GENERATOR] Polygon for country index", countryIndex, "points:", polygon.size()));
      }
    }

    LOG(LDEBUG, ("[BORDERS_GENERATOR] Number of polygons for country index", countryIndex, ":", polygons.size()));

    if (polygons.empty())
    {
      LOG(LWARNING, ("[BORDERS_GENERATOR] No suitable polygons for triangle generation in country index", countryIndex));
      return;
    }

    tesselator::TrianglesInfo trianglesInfo;
    int const trianglesCount = tesselator::TesselateInterior(polygons, trianglesInfo);

    LOG(LDEBUG, ("[BORDERS_GENERATOR] Triangles generated for country index", countryIndex, ":", trianglesCount));

    if (trianglesCount == 0)
    {
      LOG(LWARNING, ("[BORDERS_GENERATOR] No triangles generated for country index", countryIndex));
      return;
    }

    // Validation: Check that trianglesInfo has valid data
    if (trianglesInfo.IsEmpty())
    {
      LOG(LWARNING, ("[BORDERS_GENERATOR] Empty triangles info for country index", countryIndex));
      return;
    }

    // Prepare points info for serialization
    tesselator::PointsInfo pointsInfo;
    m2::PointU const basePoint = cp.GetBasePoint();
    m2::PointU const maxPoint = serial::pts::GetMaxPoint(cp);

    trianglesInfo.GetPointsInfo(
        basePoint, maxPoint,
        [coordBits = cp.GetCoordBits()](m2::PointD const & p) { return PointDToPointU(p, coordBits); },
        pointsInfo);

    // Validation: Check that we have points to serialize
    if (pointsInfo.m_points.empty())
    {
      LOG(LWARNING, ("[BORDERS_GENERATOR] No points in pointsInfo for country index", countryIndex));
      return;
    }

    // Save triangles using chain encoding
    serial::TrianglesChainSaver saver(cp);
    trianglesInfo.ProcessPortions(pointsInfo, saver);

    // Write triangles to "t" + countryIndex tag
    auto trgWriter = m_writer.GetWriter("t" + strings::to_string(countryIndex));
    if (!trgWriter)
    {
      LOG(LERROR, ("[BORDERS_GENERATOR] Failed to create writer for triangle tag t", countryIndex));
      return;
    }

    saver.Save(*trgWriter);
    LOG(LDEBUG, ("[BORDERS_GENERATOR] Generated", trianglesCount, "triangles for country index", countryIndex));
  };
};

bool ReadPolygon(std::istream & stream, Polygon & poly, std::string const & filename)
{
  std::string line, name;
  double lon, lat;

  // read ring id, fail if it's empty
  std::getline(stream, name);
  if (name.empty() || name == "END")
    return false;

  while (stream.good())
  {
    std::getline(stream, line);
    strings::Trim(line);

    if (line.empty())
      continue;

    if (line == "END")
      break;

    std::istringstream iss(line);
    iss >> lon >> lat;
    CHECK(!iss.fail(), ("Incorrect data in", filename));

    poly.AddPoint(mercator::FromLatLon(lat, lon));
  }

  // drop inner rings
  return name[0] != '!';
}
}  // namespace

bool CountryPolygons::Contains(m2::PointD const & point) const
{
  return m_polygons.ForAnyInRect(m2::RectD(point, point), [&](auto const & rgn)
  { return rgn.Contains(point, ContainsCompareFn(GetContainsEpsilon())); });
}

bool LoadBorders(std::string const & borderFile, PolygonsList & outBorders)
{
  std::ifstream stream(borderFile);
  std::string line;
  if (!std::getline(stream, line).good())  // skip title
  {
    LOG(LERROR, ("Polygon file is empty:", borderFile));
    return false;
  }

  Polygon currentPolygon;
  while (ReadPolygon(stream, currentPolygon, borderFile))
  {
    CHECK(currentPolygon.IsValid(), ("Invalid region in", borderFile));
    outBorders.emplace_back(std::move(currentPolygon));
    currentPolygon = {};
  }

  CHECK(!outBorders.empty(), ("No borders were loaded from", borderFile));
  return true;
}

bool GetBordersRect(std::string const & baseDir, std::string const & country, m2::RectD & bordersRect)
{
  auto const bordersFile = base::JoinPath(baseDir, BORDERS_DIR, country + BORDERS_EXTENSION);
  if (!Platform::IsFileExistsByFullPath(bordersFile))
  {
    LOG(LWARNING, ("File with borders does not exist:", bordersFile));
    return false;
  }

  PolygonsList borders;
  CHECK(LoadBorders(bordersFile, borders), ());
  bordersRect.MakeEmpty();
  for (auto const & border : borders)
    bordersRect.Add(border.GetRect());

  return true;
}

CountryPolygonsCollection LoadCountriesList(std::string const & baseDir)
{
  LOG(LINFO, ("Loading countries in", BORDERS_DIR, "folder in", baseDir));

  CountryPolygonsCollection countryPolygonsCollection;
  ForEachCountry(baseDir, [&](std::string name, PolygonsList && borders)
  {
    PolygonsTree polygons;
    for (Polygon & border : borders)
    {
      auto const rect = border.GetRect();
      polygons.Add(std::move(border), rect);
    }

    countryPolygonsCollection.Add(CountryPolygons(std::move(name), std::move(polygons)));
  });

  LOG(LINFO, ("Countries loaded:", countryPolygonsCollection.GetSize()));
  CHECK_NOT_EQUAL(countryPolygonsCollection.GetSize(), 0, (baseDir));
  return countryPolygonsCollection;
}

void GeneratePackedBorders(std::string const & baseDir)
{
  PackedBordersGenerator generator(baseDir);
  ForEachCountry(baseDir, generator);
  generator.WritePolygonsInfo();
}

void DumpBorderToPolyFile(std::string const & targetDir, storage::CountryId const & mwmName,
                          PolygonsList const & polygons)
{
  CHECK(!polygons.empty(), ());

  std::string const filePath = base::JoinPath(targetDir, mwmName + ".poly");
  std::ofstream poly(filePath);
  CHECK(poly.good(), ());

  // Used to have fixed precicion with 6 digits. And Alaska has 4 digits after comma :) Strange, but as is.
  poly << std::setprecision(6) << std::fixed;

  poly << mwmName << std::endl;
  size_t polygonId = 1;
  for (auto const & points : polygons)
  {
    poly << polygonId << std::endl;
    ++polygonId;
    for (auto const & point : points.Data())
    {
      ms::LatLon const ll = mercator::ToLatLon(point);
      poly << "    " << std::scientific << ll.m_lon << "    " << ll.m_lat << std::endl;
    }
    poly << "END" << std::endl;
  }

  poly << "END" << std::endl;
  poly.close();
}

void UnpackBorders(std::string const & baseDir, std::string const & targetDir)
{
  if (!Platform::IsFileExistsByFullPath(targetDir) && !Platform::MkDirChecked(targetDir))
    MYTHROW(FileSystemException, ("Unable to find or create directory", targetDir));

  std::string const packedFile = base::JoinPath(baseDir, PACKED_POLYGONS_FILE);
  LOG(LDEBUG, ("[BORDERS_UNPACK] Opening file:", packedFile));
  
  std::vector<storage::CountryDef> countries;
  FilesContainerR reader(packedFile);
  
  LOG(LDEBUG, ("[BORDERS_UNPACK] File size:", reader.GetFileSize()));
  LOG(LDEBUG, ("[BORDERS_UNPACK] Tags in file:"));
  reader.ForEachTagInfo([](FilesContainerBase::TagInfo const & info) {
    LOG(LDEBUG, ("[BORDERS_UNPACK]   Tag:", info.m_tag, "offset:", info.m_offset, "size:", info.m_size));
  });
  
  LOG(LDEBUG, ("[BORDERS_UNPACK] Checking if info tag exists..."));
  if (!reader.IsExist(PACKED_POLYGONS_INFO_TAG))
  {
    LOG(LERROR, ("[BORDERS_UNPACK] Info tag does not exist:", PACKED_POLYGONS_INFO_TAG));
    return;
  }
  
  ReaderSource<ModelReaderPtr> src(reader.GetReader(PACKED_POLYGONS_INFO_TAG));
  rw::Read(src, countries);
  
  LOG(LDEBUG, ("[BORDERS_UNPACK] Found", countries.size(), "countries"));

  for (size_t id = 0; id < countries.size(); id++)
  {
    storage::CountryId const mwmName = countries[id].m_countryId;
    LOG(LDEBUG, ("[BORDERS_UNPACK] Processing country", id, ":", mwmName));

    LOG(LDEBUG, ("[BORDERS_UNPACK] Trying to read tag:", strings::to_string(id)));
    if (!reader.IsExist(strings::to_string(id)))
    {
      LOG(LDEBUG, ("[BORDERS_UNPACK] Tag does not exist:", strings::to_string(id)));
      continue;
    }

    src = reader.GetReader(strings::to_string(id));

    auto const polygons = ReadPolygonsOfOneBorder(src);
    DumpBorderToPolyFile(targetDir, mwmName, polygons);
  }
}

CountryPolygonsCollection const & GetOrCreateCountryPolygonsTree(std::string const & baseDir)
{
  /// @todo Are there many different paths with polygons, that we have to store map?
  static std::mutex mutex;
  static ankerl::unordered_dense::map<std::string, CountryPolygonsCollection> countriesMap;

  std::lock_guard<std::mutex> lock(mutex);
  auto const it = countriesMap.find(baseDir);
  if (it != countriesMap.cend())
    return it->second;

  auto const eIt = countriesMap.emplace(baseDir, LoadCountriesList(baseDir));
  return eIt.first->second;
}
}  // namespace borders
