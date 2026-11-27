#include "traff_assessment_tool/map_widget.hpp"

#include "qt/qt_common/helpers.hpp"

#include "map/framework.hpp"
#include "map/place_page_info.hpp"
#include "map/routing_manager.hpp"

#include <QApplication>
#include <QtWidgets/QMenu>

namespace traffxml
{
MapWidget::MapWidget(Framework & framework, QWidget * parent)
  : Base(framework, false /* screenshotMode */, parent)
{
}

void MapWidget::mouseMoveEvent(QMouseEvent * e)
{
  Base::mouseMoveEvent(e);

  if (qt::common::IsLeftButton(e))
  {
    if (!m_isDragging
        && (e->pos() - m_mousePressPosition).manhattanLength() >= QApplication::startDragDistance())
      m_isDragging = true;
    e->accept();
  }
}

void MapWidget::mousePressEvent(QMouseEvent * e)
{
  Base::mousePressEvent(e);

  if (qt::common::IsLeftButton(e))
  {
    m_mousePressPosition = e->pos();
    m_isDragging = false;
  }
  else if (qt::common::IsRightButton(e))
    ShowInfoPopup(e, GetDevicePoint(e));

  if (m_mode == Mode::TrafficMarkup)
  {
    auto pt = GetDevicePoint(e);
    emit TrafficMarkupClick(m_framework.PtoG(pt), e->button());
  }
}

void MapWidget::mouseReleaseEvent(QMouseEvent * e)
{
  Base::mouseReleaseEvent(e);

  if (qt::common::IsLeftButton(e) && !m_isDragging)
    ShowPlaceMenu(e, GetDevicePoint(e));
}

void MapWidget::ClearRoute()
{
  auto & routingManager = m_framework.GetRoutingManager();

  routingManager.CloseRouting(true /* remove route points */);
}

void MapWidget::ShowPlaceMenu(QMouseEvent * e, m2::PointD const & pt)
{
  m2::PointD mercator = m_framework.PtoG(pt);
  QMenu menu;

  auto *routeFromAction = new QAction(tr("Route From"), this);
  connect(routeFromAction, &QAction::triggered,
          this, [this, mercator]() {
    SubmitRoutingPoint(mercator, RouteMarkType::Start);
  });
  menu.addAction(routeFromAction);

  auto *routeToAction = new QAction(tr("Route To"), this);
  connect(routeToAction, &QAction::triggered,
          this, [this, mercator]() {
    SubmitRoutingPoint(mercator, RouteMarkType::Finish);
  });
  menu.addAction(routeToAction);

  // TODO
  //place_page::Info const & info = m_framework.GetCurrentPlacePageInfo();
  menu.exec(e->globalPosition().toPoint());
}

void MapWidget::SubmitRoutingPoint(m2::PointD const & pt, RouteMarkType type)
{
  auto & routingManager = m_framework.GetRoutingManager();

  RouteMarkData point;
  point.m_pointType = type;
  point.m_isMyPosition = false;
  point.m_position = pt;

  routingManager.AddRoutePoint(std::move(point));

  if (routingManager.GetRoutePoints().size() >= 2)
  {
    routingManager.RoutingSession().SetGuidesForTests({});
    routingManager.BuildRoute();
  }
}
}  // namespace traffxml
