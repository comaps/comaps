#pragma once

#include "geometry/point2d.hpp"

#include "map/routing_mark.hpp"

#include "qt/qt_common/map_widget.hpp"

#include <QMouseEvent>

namespace
{
class PointsController;
}  // namespace

class Framework;

namespace traffxml
{
class MapWidget : public qt::common::MapWidget
{
  Q_OBJECT

  using Base = qt::common::MapWidget;

public:
  enum class Mode
  {
    Normal,
    TrafficMarkup
  };

  MapWidget(Framework & framework, QWidget * parent);
  ~MapWidget() override = default;

  void ClearRoute();
  void SetMode(Mode const mode) { m_mode = mode; }

  QSize sizeHint() const override
  {
    return QSize(800, 600);
  }

signals:
  void TrafficMarkupClick(m2::PointD const & p, Qt::MouseButton const b);

protected:
  void mouseMoveEvent(QMouseEvent * e) override;
  void mousePressEvent(QMouseEvent * e) override;
  void mouseReleaseEvent(QMouseEvent * e) override;

private:
  /**
   * @brief Shows the menu from which the user can set route points.
   *
   * @param e The Qt mouse event
   * @param pt The point that was clicked
   * @todo what is the reference system for `pt`?
   */
  void ShowPlaceMenu(QMouseEvent * e, m2::PointD const & pt);

  /**
   * @brief Sets a routing point.
   *
   * @param pt The point in Mercator coordinates
   * @param type The type (`Start` of `Finish`)
   */
  void SubmitRoutingPoint(m2::PointD const & pt, RouteMarkType type);

  Mode m_mode = Mode::Normal;

  /** The position of the last mouse press event. */
  QPoint m_mousePressPosition;

  /**
   * Whether we are currently dragging, i.e. have moved more than the threshold since the last mouse
   * press event.
   */
  bool m_isDragging = false;
};
}  // namespace traffxml
