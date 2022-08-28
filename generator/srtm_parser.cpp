#include "generator/srtm_parser.hpp"

#include "platform/platform.hpp"

#include "coding/endianness.hpp"
#include "coding/zip_reader.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>

namespace generator
{
namespace
{
int constexpr kArcSecondsInDegree = 60 * 60;
size_t constexpr kSrtmTileSize = (kArcSecondsInDegree + 1) * (kArcSecondsInDegree + 1) * 2;

struct UnzipMemDelegate : public ZipFileReader::Delegate
{
  explicit UnzipMemDelegate(std::vector<uint8_t> & buffer) : m_buffer(buffer), m_completed(false)
  {
    m_buffer.reserve(kSrtmTileSize);
  }

  // ZipFileReader::Delegate overrides:
  void OnBlockUnzipped(size_t size, char const * data) override
  {
    m_buffer.insert(m_buffer.end(), data, data + size);
  }

  void OnStarted() override
  {
    m_buffer.clear();
    m_completed = false;
  }

  void OnCompleted() override { m_completed = true; }

  std::vector<uint8_t> & m_buffer;
  bool m_completed;
};

std::string GetSrtmContFileName(std::string const & dir, std::string const & base)
{
  return base::JoinPath(dir, base + ".SRTMGL1.hgt.zip");
}
}  // namespace

// SrtmTile ----------------------------------------------------------------------------------------
SrtmTile::SrtmTile(SrtmTile && rhs) : m_data(std::move(rhs.m_data)), m_valid(rhs.m_valid)
{
  rhs.Invalidate();
}

void SrtmTile::Init(std::string const & dir, ms::LatLon const & coord)
{
  Invalidate();

  std::string const base = GetBase(coord);
  std::string const cont = GetSrtmContFileName(dir, base);
  std::string file = base + ".hgt";

  // Original files are stored in zip archives. Alternatively, they can be loaded
  // from uncompressed files like "N34E012.hgt".
  static bool const loadFromZip = static_cast<bool>(std::ifstream(cont));
  if (loadFromZip)
  {
    UnzipMemDelegate delegate(m_data);
    try
    {
      ZipFileReader::UnzipFile(cont, file, delegate);
    }
    catch (ZipFileReader::LocateZipException const &)
    {
      // Sometimes packed file has different name. See N39E051 measure.
      file = base + ".SRTMGL1.hgt";

      ZipFileReader::UnzipFile(cont, file, delegate);
    }

    if (!delegate.m_completed)
    {
      LOG(LWARNING, ("Can't decompress SRTM file:", cont));
      Invalidate();
      return;
    }
  }
  else
  {
    m_data = base::ReadFile(base::JoinPath(dir, file));
  }

  if (m_data.size() != kSrtmTileSize)
  {
    LOG(LWARNING, ("Bad decompressed SRTM file size:", cont, m_data.size()));
    Invalidate();
    return;
  }

  m_valid = true;
}

// static
ms::LatLon SrtmTile::GetCoordInSeconds(ms::LatLon const & coord)
{
  double ln = coord.m_lon - static_cast<int>(coord.m_lon);
  if (ln < 0)
    ln += 1;
  double lt = coord.m_lat - static_cast<int>(coord.m_lat);
  if (lt < 0)
    lt += 1;
  lt = 1 - lt;  // from North to South

  return { kArcSecondsInDegree * lt, kArcSecondsInDegree * ln };
}

geometry::Altitude SrtmTile::GetHeight(ms::LatLon const & coord) const
{
  if (!IsValid())
    return geometry::kInvalidAltitude;

  auto const ll = GetCoordInSeconds(coord);

  return GetHeightRC(static_cast<size_t>(std::round(ll.m_lat)), static_cast<size_t>(std::round(ll.m_lon)));
}

geometry::Altitude SrtmTile::GetHeightRC(size_t row, size_t col) const
{
  size_t const ix = row * (kArcSecondsInDegree + 1) + col;
  CHECK_LESS(ix, Size(), (row, col));
  return ReverseByteOrder(Data()[ix]);
}

geometry::Altitude SrtmTile::GetTriangleHeight(ms::LatLon const & coord) const
{
  if (!IsValid())
    return geometry::kInvalidAltitude;

  auto const ll = GetCoordInSeconds(coord);

  m2::Point<int> const p1(static_cast<int>(std::round(ll.m_lon)), static_cast<int>(std::round(ll.m_lat)));

  auto p2 = p1;
  if (p2.x > ll.m_lon)
  {
    if (p2.x > 0)
      --p2.x;
  }
  else if (p2.x < ll.m_lon)
  {
    if (p2.x < kArcSecondsInDegree)
      ++p2.x;
  }

  auto p3 = p1;
  if (p3.y > ll.m_lat)
  {
    if (p3.y > 0)
      --p3.y;
  }
  else if (p3.y < ll.m_lat)
  {
    if (p3.y < kArcSecondsInDegree)
      ++p3.y;
  }

  // Approximate height from triangle p1, p2, p3.
  // p1.y == p2.y; p1.x == p3.x
  // https://stackoverflow.com/questions/36090269/finding-height-of-point-on-height-map-triangles
  int const det = (p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y);
  if (det == 0)
    return GetHeightRC(p1.y, p1.x);

  double const a1 = ((p2.y - p3.y) * (ll.m_lon - p3.x) + (p3.x - p2.x) * (ll.m_lat - p3.y)) / det;
  double const a2 = ((p3.y - p1.y) * (ll.m_lon - p3.x) + (p1.x - p3.x) * (ll.m_lat - p3.y)) / det;
  double const a3 = 1 - a1 - a2;

  return static_cast<geometry::Altitude>(std::round(a1 * GetHeightRC(p1.y, p1.x) +
                                                    a2 * GetHeightRC(p2.y, p2.x) +
                                                    a3 * GetHeightRC(p3.y, p3.x)));
}

// static
std::string SrtmTile::GetPath(std::string const & dir, std::string const & base)
{
  return GetSrtmContFileName(dir, base);
}

// static
SrtmTile::LatLonKey SrtmTile::GetKey(ms::LatLon const & coord)
{
  ms::LatLon center{floor(coord.m_lat) + 0.5, floor(coord.m_lon) + 0.5};
  if (coord.m_lat < 0)
    center.m_lat -= 1.0;
  if (coord.m_lon < 0)
    center.m_lon -= 1.0;

  return {static_cast<int32_t>(center.m_lat), static_cast<int32_t>(center.m_lon)};
}

// static
std::string SrtmTile::GetBase(ms::LatLon const & coord)
{
  auto key = GetKey(coord);
  std::ostringstream ss;
  if (coord.m_lat < 0)
  {
    ss << "S";
    key.first = -key.first;
  }
  else
    ss << "N";

  ss << std::setw(2) << std::setfill('0') << key.first;

  if (coord.m_lon < 0)
  {
    ss << "W";
    key.second = -key.second;
  }
  else
    ss << "E";

  ss << std::setw(3) << key.second;
  return ss.str();
}

geometry::Altitude * SrtmTile::DataForTests(size_t & sz)
{
  m_valid = true;
  sz = kArcSecondsInDegree + 1;
  m_data.resize(kSrtmTileSize, 0);
  return reinterpret_cast<geometry::Altitude *>(m_data.data());
}

void SrtmTile::Invalidate()
{
  m_data.clear();
  m_data.shrink_to_fit();
  m_valid = false;
}

// SrtmTileManager ---------------------------------------------------------------------------------
SrtmTile const & SrtmTileManager::GetTile(ms::LatLon const & coord)
{
  auto res = m_tiles.emplace(SrtmTile::GetKey(coord), SrtmTile());
  if (res.second)
  {
    try
    {
      res.first->second.Init(m_dir, coord);
    }
    catch (RootException const & e)
    {
      std::string const base = SrtmTile::GetBase(coord);
      LOG(LINFO, ("Can't init SRTM tile:", base, "reason:", e.Msg()));
    }
  }
  return res.first->second;
}

void SrtmTileManager::Purge()
{
  MapT().swap(m_tiles);
}

}  // namespace generator
