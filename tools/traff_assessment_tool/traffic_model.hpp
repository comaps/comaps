#pragma once

#include "mainwindow.hpp"
#include "points_controller_delegate_base.hpp"
#ifdef openlr_obsolete
#include "segment_correspondence.hpp"
#endif
#include "traffic_drawer_delegate_base.hpp"
#include "traffic_panel.hpp"

#ifdef openlr_obsolete
#include "openlr/decoded_path.hpp"
#endif
#include "traffxml/traff_model.hpp"

#include "indexer/data_source.hpp"

#include "base/exception.hpp"

#include <pugixml.hpp>

#include <memory>
#include <string>
#include <vector>

#include <QAbstractTableModel>


class QItemSelection;
class Selection;

DECLARE_EXCEPTION(TrafficModelError, RootException);

namespace traffxml
{
#ifdef openlr_obsolete
namespace impl
{
/// This class denotes a "non-deterministic" feature point.
/// I.e. it is a set of all pairs <FeatureID, point index>
/// located at a specified coordinate.
/// Only one point at a time is considered active.
class RoadPointCandidate
{
public:
  RoadPointCandidate(std::vector<openlr::FeaturePoint> const & points,
                     m2::PointD const & coord);

  void ActivateCommonPoint(RoadPointCandidate const & rpc);
  openlr::FeaturePoint const & GetPoint() const;
  m2::PointD const & GetCoordinate() const;

private:
  static size_t const kInvalidId;

  void SetActivePoint(FeatureID const & fid);

  m2::PointD m_coord = m2::PointD::Zero();
  std::vector<openlr::FeaturePoint> m_points;

  size_t m_activePointIndex = kInvalidId;
};
}  // namespace impl
#endif

/// This class is used to map sample ids to real data
/// and change sample evaluations.
class TrafficModel : public QAbstractTableModel
{
  Q_OBJECT

public:
  // TODO(mgsergio): Check we are on the right mwm. I.e. right mwm version and everything.
  TrafficModel(Framework & framework,
               QObject * parent = Q_NULLPTR);

  bool SaveSampleAs(std::string const & fileName) const;
  void SetTrafficPanel(TrafficPanel * panel) { m_trafficPanel = panel; }

  int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

  QVariant data(const QModelIndex & index, int role) const Q_DECL_OVERRIDE;

  Qt::ItemFlags flags(QModelIndex const & index) const Q_DECL_OVERRIDE;

  bool IsBuildingPath() const { return m_buildingPath; }
#ifdef openlr_obsolete
  void GoldifyMatchedPath();
  void StartBuildingPath();
  void PushPoint(m2::PointD const & coord,
                 std::vector<FeaturePoint> const & points);
  void PopPoint();
  void CommitPath();
  void RollBackPath();
  void IgnorePath();

  size_t GetPointsCount() const;
  m2::PointD const & GetPoint(size_t const index) const;
  m2::PointD const & GetLastPoint() const;
  std::vector<m2::PointD> GetGoldenPathPoints() const;
#endif

public slots:
  void OnItemSelected(QItemSelection const & selected, QItemSelection const &);
  void OnClick(m2::PointD const & clickPoint, Qt::MouseButton const button)
  {
#ifdef openlr_obsolete
    HandlePoint(clickPoint, button);
#endif
  }

signals:
  void EditingStopped();
  void SegmentSelected(int segmentId);

private:
#ifdef openlr_obsolete
  void HandlePoint(m2::PointD clickPoint, Qt::MouseButton const button);
  bool StartBuildingPathChecks() const;
#endif

  Framework & m_framework;
  DataSource const & m_dataSource;
  df::DrapeApi & m_drapeApi;
  TrafficPanel * m_trafficPanel = nullptr;
#ifdef openlr_obsolete
  std::vector<SegmentCorrespondence> m_segments;
  // Non-owning pointer to an element of m_segments.
  SegmentCorrespondence * m_currentSegment = nullptr;
#endif
  std::vector<TraffMessage> m_messages;

  /**
   * Non-owning pointer to an element of m_messages.
   */
  TraffMessage * m_message = nullptr;

  std::unique_ptr<TrafficDrawerDelegateBase> m_drawerDelegate;
  std::unique_ptr<PointsControllerDelegateBase> m_pointsDelegate;

  bool m_buildingPath = false;
#ifdef openlr_obsolete
  std::vector<impl::RoadPointCandidate> m_goldenPath;
#endif

  // Clone this document and add things to its clone when saving sample.
  pugi::xml_document m_template;
};
}  // namespace traffxml
