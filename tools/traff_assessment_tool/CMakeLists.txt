project(traff_assessment_tool)

set(SRC
  location_mark_point.cpp
  location_mark_point.hpp
  main.cpp
  mainwindow.cpp
  mainwindow.hpp
  map_widget.cpp
  map_widget.hpp
  points_controller_delegate_base.hpp
  resumable_timer.cpp
  resumable_timer.hpp
  traffic_drawer_delegate_base.hpp
  traffic_model.cpp
  traffic_model.hpp
  traffic_panel.cpp
  traffic_panel.hpp
  trafficmodeinitdlg.cpp
  trafficmodeinitdlg.h
  trafficmodeinitdlg.ui
)

omim_add_executable(${PROJECT_NAME} ${SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES AUTOUIC ON AUTOMOC ON)

target_link_libraries(${PROJECT_NAME}
  qt_common
  map
  gflags::gflags
  traffxml
)
