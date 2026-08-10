#include "traffic_model.hpp"

#ifdef openlr_obsolete
#include "openlr/openlr_model_xml.hpp"
#endif

#include "drape_frontend/drape_api.hpp"

#include "indexer/data_source.hpp"

#include "map/framework.hpp"

#include "base/assert.hpp"
#include "base/scope_guard.hpp"

#include <QItemSelection>
#include <QMessageBox>

namespace traffxml
{

constexpr static dp::Color kColorFrom(0x309302ff);
constexpr static dp::Color kColorAt(0x1a5ec1ff);
constexpr static dp::Color kColorVia(0xf19721ff);
constexpr static dp::Color kColorNotVia(0x8c5678ff);
constexpr static dp::Color kColorTo(0xe42300ff);
constexpr static dp::Color kColorDecoded(0x4070ffff);
constexpr static std::string const kDecodedLineId = "decodedPath";

#ifdef openlr_obsolete
namespace
{
void RemovePointFromPull(m2::PointD const & toBeRemoved, std::vector<m2::PointD> & pool)
{
  pool.erase(
      remove_if(begin(pool), end(pool),
                [&toBeRemoved](m2::PointD const & p) { return p.EqualDxDy(toBeRemoved, 1e-6); }),
      end(pool));
}

std::vector<m2::PointD> GetReachablePoints(m2::PointD const & srcPoint,
                                           std::vector<m2::PointD> const path,
                                           PointsControllerDelegateBase const & pointsDelegate,
                                           size_t const lookbackIndex)
{
  auto reachablePoints = pointsDelegate.GetReachablePoints(srcPoint);
  if (lookbackIndex < path.size())
  {
    auto const & toBeRemoved = path[path.size() - lookbackIndex - 1];
    RemovePointFromPull(toBeRemoved, reachablePoints);
  }
  return reachablePoints;
}
}  // namespace

namespace impl
{
// static
size_t const RoadPointCandidate::kInvalidId = std::numeric_limits<size_t>::max();

/// This class denotes a "non-deterministic" feature point.
/// I.e. it is a set of all pairs <FeatureID, point index>
/// located at a specified coordinate.
/// Only one point at a time is considered active.
RoadPointCandidate::RoadPointCandidate(std::vector<FeaturePoint> const & points,
                                       m2::PointD const & coord)
  : m_coord(coord)
  , m_points(points)
{
  LOG(LDEBUG, ("Candidate points:", points));
}

void RoadPointCandidate::ActivateCommonPoint(RoadPointCandidate const & rpc)
{
  for (auto const & fp1 : m_points)
  {
    for (auto const & fp2 : rpc.m_points)
    {
      if (fp1.first == fp2.first)
      {
        SetActivePoint(fp1.first);
        return;
      }
    }
  }
  CHECK(false, ("One common feature id should exist."));
}

FeaturePoint const & RoadPointCandidate::GetPoint() const
{
  CHECK_NOT_EQUAL(m_activePointIndex, kInvalidId, ("No point is active."));
  return m_points[m_activePointIndex];
}

m2::PointD const & RoadPointCandidate::GetCoordinate() const
{
  return m_coord;
}

void RoadPointCandidate::SetActivePoint(FeatureID const & fid)
{
  for (size_t i = 0; i < m_points.size(); ++i)
  {
    if (m_points[i].first == fid)
    {
      m_activePointIndex = i;
      return;
    }
  }
  CHECK(false, ("One point should match."));
}
}  // namespace impl
#endif

QVariant GetCountryAndRoadRef(TraffMessage const & message)
{
  std::string result = "";
  if (message.m_location)
  {
    if (message.m_location.value().m_country)
      result += message.m_location.value().m_country.value();
    if (message.m_location.value().m_roadRef)
    {
      if (!result.empty())
        result += '\n';
      result += message.m_location.value().m_roadRef.value();
    }
    if (message.m_location.value().m_fuzziness)
    {
      if (!result.empty())
        result += '\n';
      switch (message.m_location.value().m_fuzziness.value())
      {
        case traffxml::Fuzziness::LowRes:
          result += "LowRes";
          break;
        case traffxml::Fuzziness::MediumRes:
          result += "MediumRes";
          break;
        case traffxml::Fuzziness::EndUnknown:
          result += "EndUnknown";
          break;
        case traffxml::Fuzziness::StartUnknown:
          result += "StartUnknown";
          break;
        case traffxml::Fuzziness::ExtentUnknown:
          result += "ExtentUnknown";
          break;
        default:
          result += "(invalid)";
          break;
      }
    }
  }
  return QString::fromStdString(result);
}

/**
 * @brief Returns a descriptive text for a point.
 *
 * The result is the junction name (if any), followed by the junction number or kilometric point (if any).
 *
 * @param point
 * @return
 */
std::string GetPointDetail(Point const & point)
{
  std::string result = point.m_junctionName ? point.m_junctionName.value() : "";
  std::string junctionRefOrKmp = point.m_junctionRef ? point.m_junctionRef.value() :
                                                       point.m_distance ? std::format("km {0:.0f}", point.m_distance.value()) : "";
  if (!junctionRefOrKmp.empty())
  {
    if (result.empty())
      result = junctionRefOrKmp;
    else
      result += " (" + junctionRefOrKmp + ")";
  }
  return result;
}

/**
 * @brief Returns a descriptive text for the location on the road.
 *
 * The result is one or two junctions, with an arrow to indicate direction where applicable.
 *
 * @param location
 * @return
 */
std::string GetLocationDetail(TraffLocation const & location)
{
  if (location.m_at)
    return GetPointDetail(location.m_at.value());

  std::string nameFrom = location.m_from ? GetPointDetail(location.m_from.value()) : "";
  std::string nameTo = location.m_from ? GetPointDetail(location.m_to.value()) : "";

  if (nameFrom == nameTo)
    return nameFrom;
  else if (!nameFrom.empty())
  {
    if (nameTo.empty())
      return nameFrom + ((location.m_directionality == Directionality::OneDirection) ? " →" : " ↔");
    else
      return nameFrom
          + ((location.m_directionality == Directionality::OneDirection) ? " → " : " ↔ ")
          + nameTo;
  }
  else if (!nameTo.empty())
    return ((location.m_directionality == Directionality::OneDirection) ? "→ " : "↔ ") + nameTo;
  else if (location.m_via)
    return GetPointDetail(location.m_via.value());
  return "";
}

/**
 * @brief Returns a description for the events in the message.
 *
 * @param message
 * @return
 */
std::string GetEventText(TraffMessage const & message)
{
  std::string result = "";
  for (auto const & event : message.m_events)
  {
    if (!result.empty())
      result += ", ";
    result += DebugPrint(event.m_type);
    // TODO quantifiers (format "q = {}")
    // TODO supplementary information (not in struct yet)
    if (event.m_length)
      result += std::format(" for {0:d} m", event.m_length.value());
    if (event.m_speed)
      result += std::format(", speed {0:d} km/h", event.m_speed.value());
  }
  return result;
}

QVariant GetDescription(TraffMessage const & message)
{
  std::string result = "";
  if (message.m_cancellation)
  {
    result = "Cancellation";
  }
  else
  {
    if (message.m_location)
    {
      std::string direction = "";
      if (message.m_location.value().m_directionality == Directionality::BothDirections)
        direction = "both directions";
      else
      {
        // TODO determine bearing and convert it to a string (northbound, southeastbound etc.)
        /*
        std::optional<Point> loc1 = message.m_location.value().m_from;
        std::optional<Point> loc2 = message.m_location.value().m_to;
        if (loc2 && !loc1)
          loc1 = message.m_location.value().m_at;
        else if (loc1 && !loc2)
          loc2 = message.m_location.value().m_at;
        if (loc1 && loc2)
        {
          ms::LatLon c1 = loc1.value().m_coordinates;
          ms::LatLon c2 = loc2.value().m_coordinates;
          // TODO figure out bearing (as string)
        }
         */
      }
      if (message.m_location.value().m_roadName)
      {
        // roadName, town, direction
        if (message.m_location.value().m_town)
          result += message.m_location.value().m_town.value() + ", ";
        // TODO territory?
        result += message.m_location.value().m_roadName.value();
        if (!direction.empty())
          result += ", " + direction;
      }
      else if (message.m_location.value().m_origin && message.m_location.value().m_destination)
        // origin–destination with arrow
        result += message.m_location.value().m_origin.value()
            + ((message.m_location.value().m_directionality == Directionality::BothDirections) ? " ↔ " : " → ")
            + message.m_location.value().m_destination.value();
      else if (!message.m_location.value().m_origin && !message.m_location.value().m_destination)
        // direction, if available
        result += direction;
      else if (message.m_location.value().m_directionality == Directionality::OneDirection)
      {
        // unidirectional, one endpoint; replacce the other with direction
        if (message.m_location.value().m_origin)
          result += message.m_location.value().m_origin.value() + " → " + direction;
        else
          result += direction + " → " + message.m_location.value().m_destination.value();
      }
      else
        // direction, if available (no meaningful way to use origin or destination)
        result += direction;

      auto locationDetail = GetLocationDetail(message.m_location.value());
      if (!locationDetail.empty())
      {
        if (!result.empty())
          result += "\n";
        result += locationDetail;
      }
    }
    auto eventText = GetEventText(message);
    if (!eventText.empty())
    {
      if (!result.empty())
        result += "\n";
      result += eventText;
    }
    // TODO start/end date
  }
  if (!result.empty())
    result += "\n";
  //result += message.m_id.substr(0, message.m_id.find(':'));
  result += message.m_id;
  result += "\n" + DebugPrint(message.m_updateTime);
  // add an extra line to get bigger rows (default is too small)
  result += "\n";
  return QString::fromStdString(result);
}

// TrafficModel -------------------------------------------------------------------------------------
TrafficModel::TrafficModel(Framework & framework,
                           QObject * parent)
  : QAbstractTableModel(parent)
  , m_framework(framework)
  , m_dataSource(framework.GetDataSource())
  , m_drawerDelegate(std::make_unique<TrafficDrawerDelegate>(framework))
  , m_pointsDelegate(std::make_unique<PointsControllerDelegate>(framework))
  , m_drapeApi(m_framework.GetDrapeApi())
{
  framework.GetTrafficManager().SetTrafficUpdateCallbackFn([this, &framework](bool final) {
    /*
     * If final is true, this indicates the queue has been emptied and no further updates are
     * imminent. Such updates should always be processed. If final is false, we can optimize by
     * selectively skipping updates.
     */
    GetPlatform().RunTask(Platform::Thread::Gui, [this, &framework, final]()
    {
      beginResetModel();
      auto const messageCache = framework.GetTrafficManager().GetMessageCache();
      m_messages.clear();
      m_messages.reserve(messageCache.size());

      for (auto & entry : messageCache)
        m_messages.push_back(std::move(entry.second));

      endResetModel();

      // clear markers
      auto editSession = m_framework.GetBookmarkManager().GetEditSession();
      editSession.ClearGroup(UserMark::Type::COLORED);
      editSession.SetIsVisible(UserMark::Type::COLORED, false);
      m_drapeApi.Clear();

      // restore status bar
      if (final)
        m_trafficPanel->SetStatus(false, m_messages.size());

      LOG(LINFO, ("Messages:", m_messages.size()));
    });
  });
}

// TODO(mgsergio): Check if a path was committed, or commit it.
bool TrafficModel::SaveSampleAs(std::string const & fileName) const
{
  CHECK(!fileName.empty(), ("Can't save to an empty file."));

  pugi::xml_document result;
  result.reset(m_template);
  auto root = result.document_element();

#ifdef openlr_obsolete
  for (auto const & sc : m_segments)
  {
    auto segment = root.append_child("Segment");
    segment.append_copy(sc.GetPartnerXMLSegment());

    if (sc.GetStatus() == SegmentCorrespondence::Status::Ignored)
    {
      segment.append_child("Ignored").text() = true;
    }
    if (sc.HasMatchedPath())
    {
      auto node = segment.append_child("Route");
      openlr::PathToXML(sc.GetMatchedPath(), node);
    }
    if (sc.HasFakePath())
    {
      auto node = segment.append_child("FakeRoute");
      openlr::PathToXML(sc.GetFakePath(), node);
    }
    if (sc.HasGoldenPath())
    {
      auto node = segment.append_child("GoldenRoute");
      openlr::PathToXML(sc.GetGoldenPath(), node);
    }
  }
#endif

  result.save_file(fileName.data(), "  " /* indent */);
  return true;
}

int TrafficModel::rowCount(const QModelIndex & parent) const
{
  return static_cast<int>(m_messages.size());
}

int TrafficModel::columnCount(const QModelIndex & parent) const { return 2; }

QVariant TrafficModel::data(const QModelIndex & index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (index.row() >= rowCount())
    return QVariant();

  if (role != Qt::DisplayRole && role != Qt::EditRole)
    return QVariant();

#ifdef openlr_obsolete
  if (index.column() == 0)
    return m_segments[index.row()].GetPartnerSegmentId();

  if (index.column() == 1)
    return static_cast<int>(m_segments[index.row()].GetStatus());

  if (index.column() == 2)
    return m_segments[index.row()].GetPositiveOffset();

  if (index.column() == 3)
    return m_segments[index.row()].GetNegativeOffset();
#endif
  switch (index.column())
  {
  case 0:
    return GetCountryAndRoadRef(m_messages[index.row()]);
  case 1:
    return GetDescription(m_messages[index.row()]);
  default:
    return QVariant();
  }

  UNREACHABLE();
}

QVariant TrafficModel::headerData(int section, Qt::Orientation orientation,
                                 int role /* = Qt::DisplayRole */) const
{
  /*
   * Qt seems buggy here. Initially, we seem to get called with Qt::Vertical for a horizontal
   * header, i.e. a row of column headers, and Qt::Horizontal for a vertical header (column of row
   * headers). Using the intuitively correct value will result in incorrect behavior and a lot of
   * head-scratching if you use just one type of header.
   * However, this (presumed) bug does not seem to be consistent, as updates call us with
   * Qt::Vertical and a row number (which can be beyond the number of columns).
   */
  if (orientation == Qt::Horizontal && role != Qt::DisplayRole)
    return QVariant();

  switch (section)
  {
  case 0: return "Road ref"; break;
  case 1: return "Description"; break;
  }
  return QVariant();
}

void TrafficModel::OnItemSelected(QItemSelection const & selected, QItemSelection const &)
{
  ASSERT(!selected.empty(), ());
  ASSERT(!m_messages.empty(), ());

  auto const row = selected.front().top();

  auto editSession = m_framework.GetBookmarkManager().GetEditSession();
  editSession.ClearGroup(UserMark::Type::COLORED);
  m_drapeApi.Clear();

  if (static_cast<size_t>(row) >= m_messages.size())
  {
    editSession.SetIsVisible(UserMark::Type::COLORED, false);
    return;
  }

  auto message = &m_messages[row];
  if (!message->m_location)
  {
    editSession.SetIsVisible(UserMark::Type::COLORED, false);
    return;
  }

  m2::RectD rect;

  editSession.SetIsVisible(UserMark::Type::COLORED, true);

  for (auto & [coords, color] : {
       std::pair{message->m_location.value().m_from, kColorFrom},
       std::pair{message->m_location.value().m_at, kColorAt},
       std::pair{message->m_location.value().m_via, kColorVia},
       std::pair{message->m_location.value().m_notVia, kColorNotVia},
       std::pair{message->m_location.value().m_to, kColorTo}
       })
    if (coords)
    {
      auto point = mercator::FromLatLon(coords.value().m_coordinates);
      rect.Add(point);
      auto mark = editSession.CreateUserMark<ColoredMarkPoint>(point);
      mark->SetColor(color);
    }

  for (auto & [mwmId, coloring] : message->m_decoded) {
    FeaturesLoaderGuard g(m_dataSource, mwmId);
    for (auto & [rsid, sg]: coloring) {
      /*
       * Not the most efficient way: even if we get multiple segments of the same feature (which
       * is typically the case), we fetch the feature from scratch for each segment, parse its
       * geometry, retrieve two points from it and add one line per segment.
       * It would be more efficient to group by feature, determine its segment range, then fetch
       * the feature, parse its geometries, retrieve all points in range and add one line for the
       * entire set of points.
       * However, since we do this for just one TraFF message at a time, performance is less of a
       * concern here, and implementing the above improvements would make the code more complex.
       */
      auto f = g.GetOriginalFeatureByIndex(rsid.m_fid);
      f->ParseGeometry(FeatureType::BEST_GEOMETRY);
      std::vector<m2::PointD> points = {
          f->GetPoint(rsid.m_idx),
          f->GetPoint(rsid.m_idx + 1)
      };
      m_drapeApi.AddLine(
          kDecodedLineId + ", " + DebugPrint(mwmId) + ", " + DebugPrint(rsid),
          df::DrapeApiLineData(points, kColorDecoded).Width(3.0f));
    }
  }

  if (rect.IsValid())
  {
    rect.Scale(1.5);
    m_framework.ShowRect(rect, 15 /* maxScale */, true /* animation */, true /* useVisibleViewport */);
  }
}

Qt::ItemFlags TrafficModel::flags(QModelIndex const & index) const
{
  if (!index.isValid())
    return Qt::ItemIsEnabled;

  return QAbstractItemModel::flags(index);
}

#ifdef openlr_obsolete
void TrafficModel::GoldifyMatchedPath()
{
  if (!m_currentSegment->HasMatchedPath())
  {
    QMessageBox::information(nullptr /* parent */, "Error",
                             "The selected segment does not have a matched path");
    return;
  }

  if (!StartBuildingPathChecks())
    return;

  m_currentSegment->SetGoldenPath(m_currentSegment->GetMatchedPath());
  m_goldenPath.clear();
  m_drawerDelegate->DrawGoldenPath(GetPoints(m_currentSegment->GetGoldenPath()));
}

void TrafficModel::StartBuildingPath()
{
  if (!StartBuildingPathChecks())
    return;

  m_currentSegment->SetGoldenPath({});

  m_buildingPath = true;
  m_drawerDelegate->ClearGoldenPath();
  m_drawerDelegate->VisualizePoints(m_pointsDelegate->GetAllJunctionPointsInViewport());
}

void TrafficModel::PushPoint(m2::PointD const & coord, std::vector<FeaturePoint> const & points)
{
  impl::RoadPointCandidate point(points, coord);
  if (!m_goldenPath.empty())
    m_goldenPath.back().ActivateCommonPoint(point);
  m_goldenPath.push_back(point);
}

void TrafficModel::PopPoint()
{
  CHECK(!m_goldenPath.empty(), ("Attempt to pop point from an empty path."));
  m_goldenPath.pop_back();
}

void TrafficModel::CommitPath()
{
  CHECK(m_currentSegment, ("No segments selected"));

  if (!m_buildingPath)
    MYTHROW(TrafficModelError, ("Path building is not started"));

  SCOPE_GUARD(guard, [this] { emit EditingStopped(); });

  m_buildingPath = false;
  m_drawerDelegate->ClearAllVisualizedPoints();

  if (m_goldenPath.size() == 1)
  {
    LOG(LDEBUG, ("Golden path is empty"));
    return;
  }

  CHECK_GREATER(m_goldenPath.size(), 1, ("Path cannot consist of only one point"));

  // Activate last point. Since no more points will be availabe we link it to the same
  // feature as the previous one was linked to.
  m_goldenPath.back().ActivateCommonPoint(m_goldenPath[GetPointsCount() - 2]);

  openlr::Path path;
  for (size_t i = 1; i < GetPointsCount(); ++i)
  {
    auto const prevPoint = m_goldenPath[i - 1];
    auto point = m_goldenPath[i];

    // The start and the end of the edge should lie on the same feature.
    point.ActivateCommonPoint(prevPoint);

    auto const & prevFt = prevPoint.GetPoint();
    auto const & ft = point.GetPoint();

    path.push_back(Edge::MakeReal(
        ft.first, prevFt.second < ft.second /* forward */, base::checked_cast<uint32_t>(prevFt.second),
        geometry::PointWithAltitude(prevPoint.GetCoordinate(), 0 /* altitude */),
        geometry::PointWithAltitude(point.GetCoordinate(), 0 /* altitude */)));
  }

  m_currentSegment->SetGoldenPath(path);
  m_goldenPath.clear();
}

void TrafficModel::RollBackPath()
{
  CHECK(m_currentSegment, ("No segments selected"));
  CHECK(m_buildingPath, ("No path building is in progress."));

  m_buildingPath = false;

  // TODO(mgsergio): Add a method for common visual manipulations.
  m_drawerDelegate->ClearAllVisualizedPoints();
  m_drawerDelegate->ClearGoldenPath();
  if (m_currentSegment->HasGoldenPath())
    m_drawerDelegate->DrawGoldenPath(GetPoints(m_currentSegment->GetGoldenPath()));

  m_goldenPath.clear();
  emit EditingStopped();
}

void TrafficModel::IgnorePath()
{
  CHECK(m_currentSegment, ("No segments selected"));

  if (m_currentSegment->HasGoldenPath())
  {
    auto const btn =
        QMessageBox::question(nullptr /* parent */, "Override warning",
                              "The selected segment has a golden path. Do you want to discard it?");
    if (btn == QMessageBox::No)
      return;
  }

  m_buildingPath = false;

  // TODO(mgsergio): Add a method for common visual manipulations.
  m_drawerDelegate->ClearAllVisualizedPoints();
  m_drawerDelegate->ClearGoldenPath();

  m_currentSegment->Ignore();
  m_goldenPath.clear();
  emit EditingStopped();
}

size_t TrafficModel::GetPointsCount() const
{
  return m_goldenPath.size();
}

m2::PointD const & TrafficModel::GetPoint(size_t const index) const
{
  return m_goldenPath[index].GetCoordinate();
}

m2::PointD const & TrafficModel::GetLastPoint() const
{
  CHECK(!m_goldenPath.empty(), ("Attempt to get point from an empty path."));
  return m_goldenPath.back().GetCoordinate();
}

std::vector<m2::PointD> TrafficModel::GetGoldenPathPoints() const
{
  std::vector<m2::PointD> coordinates;
  for (auto const & roadPoint : m_goldenPath)
    coordinates.push_back(roadPoint.GetCoordinate());
  return coordinates;
}

// TODO(mgsergio): Draw the first point when the path size is 1.
void TrafficModel::HandlePoint(m2::PointD clickPoint, Qt::MouseButton const button)
{
  if (!m_buildingPath)
    return;

  auto const currentPathLength = GetPointsCount();
  auto const lastClickedPoint = currentPathLength != 0
      ? GetLastPoint()
      : m2::PointD::Zero();

  auto const & p = m_pointsDelegate->GetCandidatePoints(clickPoint);
  auto const & candidatePoints = p.first;
  clickPoint = p.second;
  if (candidatePoints.empty())
    return;

  auto reachablePoints = GetReachablePoints(clickPoint, GetGoldenPathPoints(), *m_pointsDelegate,
                                            0 /* lookBackIndex */);
  auto const & clickablePoints = currentPathLength != 0
      ? GetReachablePoints(lastClickedPoint, GetGoldenPathPoints(), *m_pointsDelegate,
                           1 /* lookbackIndex */)
      // TODO(mgsergio): This is not quite correct since view port can change
      // since first call to visualize points. But it's ok in general.
      : m_pointsDelegate->GetAllJunctionPointsInViewport();

  using ClickType = PointsControllerDelegateBase::ClickType;
  switch (m_pointsDelegate->CheckClick(clickPoint, lastClickedPoint, clickablePoints))
  {
  case ClickType::Add:
    // TODO(mgsergio): Think of refactoring this with if (accumulator.empty)
    // instead of pushing point first ad then removing last selection.
    PushPoint(clickPoint, candidatePoints);

    if (currentPathLength > 0)
    {
      // TODO(mgsergio): Should I remove lastClickedPoint from clickablePoints
      // as well?
      RemovePointFromPull(lastClickedPoint, reachablePoints);
      m_drawerDelegate->DrawGoldenPath(GetGoldenPathPoints());
    }

    m_drawerDelegate->ClearAllVisualizedPoints();
    m_drawerDelegate->VisualizePoints(reachablePoints);
    m_drawerDelegate->VisualizePoints({clickPoint});
    break;
  case ClickType::Remove:                       // TODO(mgsergio): Rename this case.
    if (button == Qt::MouseButton::LeftButton)  // RemovePoint
    {
      m_drawerDelegate->ClearAllVisualizedPoints();
      m_drawerDelegate->ClearGoldenPath();

      PopPoint();
      if (m_goldenPath.empty())
      {
        m_drawerDelegate->VisualizePoints(m_pointsDelegate->GetAllJunctionPointsInViewport());
      }
      else
      {
        m_drawerDelegate->VisualizePoints(GetReachablePoints(
            GetLastPoint(), GetGoldenPathPoints(), *m_pointsDelegate, 1 /* lookBackIndex */));
      }

      if (GetPointsCount() > 1)
        m_drawerDelegate->DrawGoldenPath(GetGoldenPathPoints());
    }
    else if (button == Qt::MouseButton::RightButton)
    {
      CommitPath();
    }
    break;
  case ClickType::Miss:
    // TODO(mgsergio): This situation should be handled by checking candidatePoitns.empty() above.
    // Not shure though if all cases are handled by that check.
    return;
  }
}

bool TrafficModel::StartBuildingPathChecks() const
{
  CHECK(m_currentSegment, ("A segment should be selected before path building is started."));

  if (m_buildingPath)
    MYTHROW(TrafficModelError, ("Path building already in progress."));

  if (m_currentSegment->HasGoldenPath())
  {
    auto const btn = QMessageBox::question(
        nullptr /* parent */, "Override warning",
        "The selected segment already has a golden path. Do you want to override?");
    if (btn == QMessageBox::No)
      return false;
  }

  return true;
}
#endif
}  // namespace traffxml
