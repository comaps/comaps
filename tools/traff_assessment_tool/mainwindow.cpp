#include "traff_assessment_tool/mainwindow.hpp"

#include "traff_assessment_tool/map_widget.hpp"
#include "traff_assessment_tool/points_controller_delegate_base.hpp"
#include "traff_assessment_tool/traffic_drawer_delegate_base.hpp"
#include "traff_assessment_tool/traffic_model.hpp"
#include "traff_assessment_tool/traffic_panel.hpp"
#include "traff_assessment_tool/trafficmodeinitdlg.h"

#include "map/framework.hpp"

#include "drape_frontend/drape_api.hpp"

#include "routing/data_source.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/road_graph.hpp"

#include "routing_common/car_model.hpp"

#include "storage/country_parent_getter.hpp"

#include "traffxml/traff_model_xml.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include <QApplication>
#include <QClipboard>
#include <QDockWidget>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStandardPaths>

#include <cerrno>
#include <cstring>
#include <memory>
#include <vector>

namespace traffxml
{
namespace
{
class TrafficDrawerDelegate : public TrafficDrawerDelegateBase
{
  static constexpr char const * kEncodedLineId = "encodedPath";
  static constexpr char const * kDecodedLineId = "decodedPath";
  static constexpr char const * kGoldenLineId = "goldenPath";

public:
  explicit TrafficDrawerDelegate(Framework & framework)
    : m_framework(framework)
    , m_drapeApi(m_framework.GetDrapeApi())
    , m_bm(framework.GetBookmarkManager())
  {
  }

  void SetViewportCenter(m2::PointD const & center) override
  {
    m_framework.SetViewportCenter(center);
  }

  void DrawDecodedSegments(std::vector<m2::PointD> const & points) override
  {
    CHECK(!points.empty(), ("Points must not be empty."));

    LOG(LINFO, ("Decoded segment", points));
    m_drapeApi.AddLine(kDecodedLineId,
                       df::DrapeApiLineData(points, dp::Color(0, 0, 255, 255))
                       .Width(3.0f).ShowPoints(true /* markPoints */));
  }

  void DrawEncodedSegment(std::vector<m2::PointD> const & points) override
  {
    LOG(LINFO, ("Encoded segment", points));
    m_drapeApi.AddLine(kEncodedLineId,
                       df::DrapeApiLineData(points, dp::Color(255, 0, 0, 255))
                       .Width(3.0f).ShowPoints(true /* markPoints */));
  }

  void DrawGoldenPath(std::vector<m2::PointD> const & points) override
  {
    m_drapeApi.AddLine(kGoldenLineId,
                       df::DrapeApiLineData(points, dp::Color(255, 127, 36, 255))
                       .Width(4.0f).ShowPoints(true /* markPoints */));
  }

  void ClearGoldenPath() override
  {
    m_drapeApi.RemoveLine(kGoldenLineId);
  }

  void ClearAllPaths() override
  {
    m_drapeApi.Clear();
  }

  void VisualizePoints(std::vector<m2::PointD> const & points) override
  {
    auto editSession = m_bm.GetEditSession();
    editSession.SetIsVisible(UserMark::Type::DEBUG_MARK, true);
    for (auto const & p : points)
      editSession.CreateUserMark<DebugMarkPoint>(p);
  }

  void ClearAllVisualizedPoints() override
  {
    m_bm.GetEditSession().ClearGroup(UserMark::Type::DEBUG_MARK);
  }

private:
  Framework & m_framework;
  df::DrapeApi & m_drapeApi;
  BookmarkManager & m_bm;
};

bool PointsMatch(m2::PointD const & a, m2::PointD const & b)
{
  auto constexpr kToleranceDistanceM = 1.0;
  return mercator::DistanceOnEarth(a, b) < kToleranceDistanceM;
}

class PointsControllerDelegate : public PointsControllerDelegateBase
{
public:
  explicit PointsControllerDelegate(Framework & framework)
    : m_framework(framework)
    , m_dataSource(const_cast<DataSource &>(GetDataSource()), nullptr /* numMwmIDs */)
    , m_roadGraph(m_dataSource, routing::IRoadGraph::Mode::ObeyOnewayTag,
                  std::make_unique<routing::CarModelFactory>(storage::CountryParentGetter{}))
  {
  }

  std::vector<m2::PointD> GetAllJunctionPointsInViewport() const override
  {
    std::vector<m2::PointD> points;
    auto const & rect = m_framework.GetCurrentViewport();
    auto const pushPoint = [&points, &rect](m2::PointD const & point)
    {
      if (!rect.IsPointInside(point))
        return;
      for (auto const & p : points)
      {
        if (PointsMatch(point, p))
          return;
      }
      points.push_back(point);
    };

    auto const pushFeaturePoints = [&pushPoint](FeatureType & ft)
    {
      if (ft.GetGeomType() != feature::GeomType::Line)
        return;

      /// @todo Transported (railway=rail) are also present here :)
      auto const roadClass = ftypes::GetHighwayClass(feature::TypesHolder(ft));
      if (roadClass == ftypes::HighwayClass::Undefined ||
          roadClass == ftypes::HighwayClass::Pedestrian)
      {
        return;
      }
      ft.ForEachPoint(pushPoint, scales::GetUpperScale());
    };

    GetDataSource().ForEachInRect(pushFeaturePoints, rect, scales::GetUpperScale());
    return points;
  }

  std::pair<std::vector<FeaturePoint>, m2::PointD> GetCandidatePoints(
      m2::PointD const & p) const override
  {
    auto constexpr kInvalidIndex = std::numeric_limits<size_t>::max();

    std::vector<FeaturePoint> points;
    m2::PointD pointOnFt;
    indexer::ForEachFeatureAtPoint(GetDataSource(), [&points, &p, &pointOnFt](FeatureType & ft)
    {
      if (ft.GetGeomType() != feature::GeomType::Line)
        return;

      ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

      auto minDistance = std::numeric_limits<double>::max();
      auto bestPointIndex = kInvalidIndex;
      for (size_t i = 0; i < ft.GetPointsCount(); ++i)
      {
        auto const & fp = ft.GetPoint(i);
        auto const distance = mercator::DistanceOnEarth(fp, p);
        if (PointsMatch(fp, p) && distance < minDistance)
        {
          bestPointIndex = i;
          minDistance = distance;
        }
      }

      if (bestPointIndex != kInvalidIndex)
      {
        points.emplace_back(ft.GetID(), bestPointIndex);
        pointOnFt = ft.GetPoint(bestPointIndex);
      }
    }, p);

    return std::make_pair(points, pointOnFt);
  }

  std::vector<m2::PointD> GetReachablePoints(m2::PointD const & p) const override
  {
    routing::FeaturesRoadGraph::EdgeListT edges;
    m_roadGraph.GetOutgoingEdges(geometry::PointWithAltitude(p, geometry::kDefaultAltitudeMeters),
                                 edges);

    std::vector<m2::PointD> points;
    for (auto const & e : edges)
      points.push_back(e.GetEndJunction().GetPoint());
    return points;
  }

  ClickType CheckClick(m2::PointD const & clickPoint,
                       m2::PointD const & lastClickedPoint,
                       std::vector<m2::PointD> const & reachablePoints) const override
  {
    // == Comparison is safe here since |clickPoint| is adjusted by GetFeaturesPointsByPoint
    // so to be equal the closest feature's one.
    if (clickPoint == lastClickedPoint)
      return ClickType::Remove;
    for (auto const & p : reachablePoints)
    {
      if (PointsMatch(clickPoint, p))
        return ClickType::Add;
    }
    return ClickType::Miss;
  }

private:
  DataSource const & GetDataSource() const { return m_framework.GetDataSource(); }

  Framework & m_framework;
  routing::MwmDataSource m_dataSource;
  routing::FeaturesRoadGraph m_roadGraph;
};
}  // namespace


MainWindow::MainWindow(Framework & framework)
  : m_framework(framework)
{
  m_mapWidget = new MapWidget(m_framework, this /* parent */);

  m_layout = new QHBoxLayout();
  m_layout->addWidget(m_mapWidget);

  auto * window = new QWidget();
  window->setLayout(m_layout);
  window->setGraphicsEffect(nullptr);

  setCentralWidget(window);

  setWindowTitle(tr("TraFF Assessment Tool"));
  setWindowIcon(QIcon(":/ui/logo.png"));

  QMenu * fileMenu = new QMenu("File", this);
  menuBar()->addMenu(fileMenu);

  fileMenu->addAction("Open sample", QKeySequence("Ctrl+O"), this, &MainWindow::OnOpenTrafficSample);
  fileMenu->addAction("Purge expired messages", QKeySequence("Ctrl+P"), this, &MainWindow::OnPurgeExpiredMessages);
  fileMenu->addAction("Clear TraFF cache", QKeySequence("Ctrl+D"), this, &MainWindow::OnClearCache);

  m_closeTrafficSampleAction = fileMenu->addAction("Close sample", QKeySequence("Ctrl+W"), this, &MainWindow::OnCloseTrafficSample);
  m_saveTrafficSampleAction = fileMenu->addAction("Save sample", QKeySequence("Ctrl+S"), this, &MainWindow::OnSaveTrafficSample);

  fileMenu->addSeparator();

  m_framework.GetTrafficManager().SetEnabled(true);

#ifdef openlr_obsolete
  m_goldifyMatchedPathAction = fileMenu->addAction("Goldify", QKeySequence("Ctrl+G"), [this] { m_trafficModel->GoldifyMatchedPath(); });
  m_startEditingAction = fileMenu->addAction("Edit", QKeySequence("Ctrl+E"),
                                             [this] {
                                               m_trafficModel->StartBuildingPath();
                                               m_mapWidget->SetMode(MapWidget::Mode::TrafficMarkup);
                                               m_commitPathAction->setEnabled(true /* enabled */);
                                               m_cancelPathAction->setEnabled(true /* enabled */);
                                             });
  m_commitPathAction = fileMenu->addAction("Accept path",
                                           QKeySequence("Ctrl+A"),
                                           [this] {
                                             m_trafficModel->CommitPath();
                                             m_mapWidget->SetMode(MapWidget::Mode::Normal);
                                           });
  m_cancelPathAction = fileMenu->addAction("Revert path",
                                           QKeySequence("Ctrl+R"),
                                           [this] {
                                             m_trafficModel->RollBackPath();
                                             m_mapWidget->SetMode(MapWidget::Mode::Normal);
                                           });
  m_ignorePathAction = fileMenu->addAction("Ignore path",
                                           QKeySequence("Ctrl+I"),
                                           [this] {
                                             m_trafficModel->IgnorePath();
                                             m_mapWidget->SetMode(MapWidget::Mode::Normal);
                                           });

  m_goldifyMatchedPathAction->setEnabled(false /* enabled */);
  m_closeTrafficSampleAction->setEnabled(false /* enabled */);
  m_saveTrafficSampleAction->setEnabled(false /* enabled */);
  m_startEditingAction->setEnabled(false /* enabled */);
  m_commitPathAction->setEnabled(false /* enabled */);
  m_cancelPathAction->setEnabled(false /* enabled */);
  m_ignorePathAction->setEnabled(false /* enabled */);
#endif
}

void MainWindow::CreateTrafficPanel(bool hasMessages)
{
  if (!m_trafficModel)
  {
    m_trafficModel = new TrafficModel(m_framework);

    connect(m_mapWidget, &MapWidget::TrafficMarkupClick,
            m_trafficModel, &TrafficModel::OnClick);
    connect(m_trafficModel, &TrafficModel::EditingStopped,
            this, &MainWindow::OnPathEditingStop);
    connect(m_trafficModel, &TrafficModel::SegmentSelected,
            [](int segmentId) { QApplication::clipboard()->setText(QString::number(segmentId)); });
  }

  if (!m_dockWidget)
  {
    m_dockWidget = new QDockWidget(tr("Messages"), this);
    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, m_dockWidget);

    m_trafficPanel = new TrafficPanel(m_trafficModel, m_dockWidget);
    m_dockWidget->setWidget(m_trafficPanel);
    m_trafficModel->SetTrafficPanel(m_trafficPanel);

    m_dockWidget->adjustSize();
    m_dockWidget->setMinimumWidth(400);
  }
  if (hasMessages)
  {
    m_trafficPanel->SetStatus(true);
    m_trafficPanel->GetTimer().Resume();
  }
  m_dockWidget->show();
}

void MainWindow::DestroyTrafficPanel()
{
  LOG(LINFO, ("enter"));
  removeDockWidget(m_dockWidget);
  delete m_dockWidget;
  m_dockWidget = nullptr;

  delete m_trafficModel;
  m_trafficModel = nullptr;

  m_mapWidget->SetMode(MapWidget::Mode::Normal);
}

void MainWindow::OnOpenTrafficSample()
{
  TrafficModeInitDlg dlg;
  dlg.exec();
  if (dlg.result() != QDialog::DialogCode::Accepted)
    return;

  pugi::xml_document document;
  LOG(LINFO, ("Attempting to load:", dlg.GetDataFilePath()));
  auto const load_result = document.load_file(dlg.GetDataFilePath().data());
  if (!load_result)
  {
    QMessageBox::critical(this, "Data loading error", QString::asprintf("Can't load file %s: %s", dlg.GetDataFilePath().data(), load_result.description()));
    LOG(LERROR, ("Can't load file", dlg.GetDataFilePath(), ":", load_result.description()));
    return;
  }

  std::setlocale(LC_ALL, "en_US.UTF-8");
  traffxml::TraffFeed feed;
  traffxml::TraffFeed shiftedFeed;
  bool hasMessages = false;
  if (traffxml::ParseTraff(document, std::nullopt /* dataSource */, feed))
  {
    hasMessages = !feed.empty();
    for (auto message : feed)
    {
      // `ShiftTimestamps()` will not change the message in `feed`, therefore construct a new feed
      message.ShiftTimestamps();
      shiftedFeed.push_back(message);
    }
    LOG(LINFO, ("TraFF data parsed successfully, pushing"));
    m_framework.GetTrafficManager().ReceiveFeed(shiftedFeed);
    LOG(LINFO, ("Push completed"));
  }
  else
  {
    QMessageBox::critical(this, "Data loading error", QString("An error occurred parsing the TraFF feed"));
    LOG(LWARNING, ("An error occurred parsing the TraFF feed"));
    return;
  }

  try
  {
    CreateTrafficPanel(hasMessages);
  }
  catch (TrafficModelError const & e)
  {
    QMessageBox::critical(this, "Data loading error", QString("Can't create traffic panel."));
    LOG(LERROR, (e.Msg()));
    return;
  }

#ifdef openlr_obsolete
  m_goldifyMatchedPathAction->setEnabled(true /* enabled */);
  m_closeTrafficSampleAction->setEnabled(true /* enabled */);
  m_saveTrafficSampleAction->setEnabled(true /* enabled */);
  m_startEditingAction->setEnabled(true /* enabled */);
  m_ignorePathAction->setEnabled(true /* enabled */);
#endif
}

void MainWindow::OnPurgeExpiredMessages()
{
  m_framework.GetTrafficManager().PurgeExpiredMessages();
}

void MainWindow::OnClearCache()
{
  m_trafficPanel->GetTimer().Reset();
  m_framework.GetTrafficManager().Clear();
}

void MainWindow::OnCloseTrafficSample()
{
  // TODO(mgsergio):
  // If not saved, ask a user if he/she wants to save.
  // OnSaveTrafficSample()

#ifdef openlr_obsolete
  m_goldifyMatchedPathAction->setEnabled(false /* enabled */);
  m_saveTrafficSampleAction->setEnabled(false /* enabled */);
  m_closeTrafficSampleAction->setEnabled(false /* enabled */);
  m_startEditingAction->setEnabled(false /* enabled */);
  m_commitPathAction->setEnabled(false /* enabled */);
  m_cancelPathAction->setEnabled(false /* enabled */);
  m_ignorePathAction->setEnabled(false /* enabled */);
#endif

  DestroyTrafficPanel();
}

void MainWindow::OnSaveTrafficSample()
{
  // TODO(mgsergio): Add default filename.
  auto const & fileName = QFileDialog::getSaveFileName(this, "Save sample");
  if (fileName.isEmpty())
    return;

  pugi::xml_document document;

  auto const messageCache = m_framework.GetTrafficManager().GetMessageCache();

  traffxml::GenerateTraff(messageCache, document);
  document.save_file(fileName.toStdString().data(), "  " /* indent */);

#ifdef openlr_obsolete
  if (!m_trafficModel->SaveSampleAs(fileName.toStdString()))
  {
    QMessageBox::critical(
        this, "Saving error",
        QString("Can't save file: ") + strerror(errno));
  }
#endif
}

void MainWindow::OnPathEditingStop()
{
#ifdef openlr_obsolete
  m_commitPathAction->setEnabled(false /* enabled */);
  m_cancelPathAction->setEnabled(false /* enabled */);
  m_cancelPathAction->setEnabled(false /* enabled */);
#endif
}
}  // namespace traffxml
