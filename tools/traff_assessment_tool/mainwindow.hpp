#pragma once

#include "base/string_utils.hpp"

#include "traff_assessment_tool/traffic_panel.hpp"

#include <string>

#include <QMainWindow>

class Framework;
class QHBoxLayout;

namespace traffxml
{
class MapWidget;
class TrafficModel;
class WebView;
}

namespace df
{
class DrapeApi;
}

class QDockWidget;

namespace traffxml
{
class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(Framework & framework);

private:
  void CreateTrafficPanel(bool hasMessages);
  void DestroyTrafficPanel();

  /**
   * Called when the user requests to open a sample file.
   */
  void OnOpenTrafficSample();

  /**
   * Called when the user requests to purge expired messages.
   */
  void OnPurgeExpiredMessages();

  /**
   * Called when the user requests to clear the cache.
   */
  void OnClearCache();
  void OnCloseTrafficSample();
  void OnSaveTrafficSample();
  void OnPathEditingStop();

  Framework & m_framework;

  traffxml::TrafficModel * m_trafficModel = nullptr;
  QDockWidget * m_dockWidget = nullptr;
  TrafficPanel * m_trafficPanel = nullptr;

#ifdef openlr_obsolete
  QAction * m_goldifyMatchedPathAction = nullptr;
#endif
  QAction * m_saveTrafficSampleAction = nullptr;
  QAction * m_closeTrafficSampleAction = nullptr;
#ifdef openlr_obsolete
  QAction * m_startEditingAction = nullptr;
  QAction * m_commitPathAction  = nullptr;
  QAction * m_cancelPathAction = nullptr;
  QAction * m_ignorePathAction = nullptr;
#endif

  traffxml::MapWidget * m_mapWidget = nullptr;
  QHBoxLayout * m_layout = nullptr;
};
}  // namespace traffxml
