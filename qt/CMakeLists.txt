project(desktop)

QT6_ADD_RESOURCES(RES_SOURCES res/resources.qrc)

set(SRC
  about.cpp
  about.hpp
  bookmark_dialog.cpp
  bookmark_dialog.hpp
  build_style/build_common.cpp
  build_style/build_common.h
  build_style/build_drules.cpp
  build_style/build_drules.h
  build_style/build_phone_pack.cpp
  build_style/build_phone_pack.h
  build_style/build_skins.cpp
  build_style/build_skins.h
  build_style/build_statistics.cpp
  build_style/build_statistics.h
  build_style/build_style.cpp
  build_style/build_style.h
  build_style/run_tests.cpp
  build_style/run_tests.h
  create_feature_dialog.cpp
  create_feature_dialog.hpp
  draw_widget.cpp
  draw_widget.hpp
  editor_dialog.cpp
  editor_dialog.hpp
  html_processor.cpp
  html_processor.hpp
  info_dialog.cpp
  info_dialog.hpp
  main.cpp
  mainwindow.cpp
  mainwindow.hpp
  mwms_borders_selection.cpp
  mwms_borders_selection.hpp
  osm_auth_dialog.cpp
  osm_auth_dialog.hpp
  place_page_dialog_common.cpp
  place_page_dialog_common.hpp
  place_page_dialog_developer.cpp
  place_page_dialog_developer.hpp
  place_page_dialog_user.cpp
  place_page_dialog_user.hpp
  preferences_dialog.cpp
  preferences_dialog.hpp
  popup_menu_holder.cpp
  popup_menu_holder.hpp
  routing_settings_dialog.cpp
  routing_settings_dialog.hpp
  routing_turns_visualizer.cpp
  routing_turns_visualizer.hpp
  ruler.cpp
  ruler.hpp
  screenshoter.cpp
  screenshoter.hpp
  search_panel.cpp
  search_panel.hpp
  selection.hpp
  update_dialog.cpp
  update_dialog.hpp
)

omim_add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE ${RES_SOURCES} ${SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)

target_link_libraries(${PROJECT_NAME}
  generator  # For borders::LoadBorders
  qt_common
  map
  gflags::gflags
  location_service
)

if (BUILD_DESIGNER)
  target_link_libraries(${PROJECT_NAME} generator)
  set(BUNDLE_NAME "CoMaps-Designer")
  set(BUNDLE_DISPLAY_NAME "CoMaps Designer")
  set(BUNDLE_ICON designer.icns)
else()
  set(BUNDLE_NAME "CoMaps")
  set(BUNDLE_DISPLAY_NAME "CoMaps Desktop")
  set(BUNDLE_ICON mac.icns)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${BUNDLE_NAME})

set(BUNDLE_EXECUTABLE ${BUNDLE_NAME})

set(BUNDLE_FOLDER ${CMAKE_BINARY_DIR}/${BUNDLE_NAME}.app)
set(RESOURCES_FOLDER ${BUNDLE_FOLDER}/Contents/Resources)
set(DATA_DIR ${OMIM_ROOT}/data)

file(MAKE_DIRECTORY ${RESOURCES_FOLDER})
function(copy_resources)
  foreach(file ${ARGN})
    if(EXISTS ${DATA_DIR}/${file})
      file(COPY ${DATA_DIR}/${file} DESTINATION ${RESOURCES_FOLDER})
    endif()
  endforeach()
endfunction()

set(DATA_FILES
  countries-strings
  fonts
  symbols
  welcome.html
  categories.txt
  categories_cuisines.txt
  classificator.txt
  colors.txt
  copyright.html
  countries.txt
  countries_meta.txt
  drules_proto_default_light.bin
  drules_proto_default_dark.bin
  drules_proto_outdoors_light.bin
  drules_proto_outdoors_dark.bin
  drules_proto_vehicle_light.bin
  drules_proto_vehicle_dark.bin
  editor.config
  packed_polygons.bin
  patterns.txt
  subtypes.csv
  transit_colors.txt
  types.txt
  mapcss-mapping.csv
  World.mwm
  WorldCoasts.mwm
)

copy_resources(${DATA_FILES})

include(GNUInstallDirs)

if (PLATFORM_WIN)
  # Windows: flat layout — exe and data\ in the same directory.
  install(TARGETS ${PROJECT_NAME} DESTINATION .)

  function(install_resources)
    foreach(file ${ARGN})
      if(EXISTS ${DATA_DIR}/${file})
        if(IS_DIRECTORY ${DATA_DIR}/${file})
          install(DIRECTORY ${DATA_DIR}/${file} DESTINATION data)
        else()
          install(FILES ${DATA_DIR}/${file} DESTINATION data)
        endif()
      endif()
    endforeach()
  endfunction()
else()
  install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

  function(install_resources)
    foreach(file ${ARGN})
      if(EXISTS ${DATA_DIR}/${file})
        if(IS_DIRECTORY ${DATA_DIR}/${file})
          install(DIRECTORY ${DATA_DIR}/${file} DESTINATION ${CMAKE_INSTALL_DATADIR}/comaps/data)
        else()
          install(FILES ${DATA_DIR}/${file} DESTINATION ${CMAKE_INSTALL_DATADIR}/comaps/data)
        endif()
      endif()
    endforeach()
  endfunction()
endif()

install_resources(${DATA_FILES})

if (PLATFORM_LINUX)
  install(FILES ${OMIM_ROOT}/packaging/app.comaps.comaps.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo/)
  install(FILES ${OMIM_ROOT}/qt/res/linux/app.comaps.comaps.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications/)
  if (BUILD_DESIGNER)
    install(FILES ${OMIM_ROOT}/qt/res/designer.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps/ RENAME comaps.png)
    install(FILES ${OMIM_ROOT}/qt/res/designer-scalable.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps/ RENAME comaps.svg)
  else()
    install(FILES ${OMIM_ROOT}/qt/res/logo.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps/ RENAME comaps.png)
    install(FILES ${OMIM_ROOT}/qt/res/logo-scalable.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps/ RENAME comaps.svg)
  endif()
endif()

if (NOT PLATFORM_LINUX)
  # On Linux, ICU data is loaded from the shared library.
  copy_resources(icudt75l.dat)
endif()

if (PLATFORM_MAC)
  if (BUILD_DESIGNER)
    file(COPY ${PROJECT_SOURCE_DIR}/res/mac/designer.icns DESTINATION ${RESOURCES_FOLDER})
  else ()
    file(COPY ${PROJECT_SOURCE_DIR}/res/mac/mac.icns DESTINATION ${RESOURCES_FOLDER})
  endif ()
endif()

if (PLATFORM_MAC)
  if (NOT APP_VERSION)
    set(BUNDLE_VERSION "UNKNOWN")
  else()
    set(BUNDLE_VERSION ${APP_VERSION})
  endif()

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/res/mac/Info.plist.in ${CMAKE_CURRENT_BINARY_DIR}/generated/Info.plist)

  set_target_properties(${PROJECT_NAME}
    PROPERTIES
      OUTPUT_NAME ${BUNDLE_NAME}
      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/generated/Info.plist
  )
endif()

if (PLATFORM_WIN)
  if (BUILD_DESIGNER)
    target_sources(${PROJECT_NAME} PRIVATE res/windows/designer.rc)
  else()
    target_sources(${PROJECT_NAME} PRIVATE res/windows/windows.rc)
  endif()

  # Install ICU data file next to the data directory.
  install_resources(icudt75l.dat)

  # CPack: ZIP package for standalone Windows distribution.
  set(CPACK_GENERATOR ZIP NSIS)
  set(CPACK_PACKAGE_NAME "CoMaps")
  set(CPACK_NSIS_DISPLAY_NAME "CoMaps")
  set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
  set(CPACK_NSIS_PACKAGE_NAME "CoMaps")
  set(CPACK_NSIS_UNINSTALL_NAME "Uninstall CoMaps")
  if (BUILD_DESIGNER)
    set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/res/windows/designer.ico")
  else()
    set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/res/windows/windows.ico")
  endif()
  include(CPack)
endif()

if (BUILD_DESIGNER)
  file(COPY ${OMIM_ROOT}/data/symbols/mdpi/light/ DESTINATION ${OMIM_ROOT}/data/symbols/mdpi/design/)
  file(COPY ${OMIM_ROOT}/data/symbols/hdpi/light/ DESTINATION ${OMIM_ROOT}/data/symbols/hdpi/design/)
  file(COPY ${OMIM_ROOT}/data/symbols/xhdpi/light/ DESTINATION ${OMIM_ROOT}/data/symbols/xhdpi/design/)
  file(COPY ${OMIM_ROOT}/data/symbols/xxhdpi/light/ DESTINATION ${OMIM_ROOT}/data/symbols/xxhdpi/design/)
  file(COPY ${OMIM_ROOT}/data/symbols/6plus/light/ DESTINATION ${OMIM_ROOT}/data/symbols/6plus/design/)
  file(COPY ${OMIM_ROOT}/data/symbols/xxxhdpi/light/ DESTINATION ${OMIM_ROOT}/data/symbols/xxxhdpi/design/)
  file(COPY_FILE ${OMIM_ROOT}/data/drules_proto_default_light.bin ${OMIM_ROOT}/data/drules_proto_default_design.bin)
  file(COPY_FILE ${OMIM_ROOT}/data/colors.txt ${OMIM_ROOT}/data/colors_design.txt)
  file(COPY_FILE ${OMIM_ROOT}/data/patterns.txt ${OMIM_ROOT}/data/patterns_design.txt)

  copy_resources(
    colors_design.txt
    drules_proto_default_design.bin
    mapcss-dynamic.txt
    mapcss-mapping.csv
    patterns_design.txt
    symbols/mdpi/design
    symbols/hdpi/design
    symbols/xhdpi/design
    symbols/xxhdpi/design
    symbols/xxxhdpi/design
    symbols/6plus/design
  )

  file(COPY ${OMIM_ROOT}/tools/python/recalculate_geom_index.py DESTINATION ${RESOURCES_FOLDER})
  file(COPY ${OMIM_ROOT}/tools/python/generate_styles_override.py DESTINATION ${RESOURCES_FOLDER})
  file(COPY ${OMIM_ROOT}/tools/kothic DESTINATION ${RESOURCES_FOLDER})
  file(COPY ${OMIM_ROOT}/tools/python/stylesheet/drules_info.py DESTINATION ${RESOURCES_FOLDER}/kothic/src/)
  file(COPY ${OMIM_ROOT}/tools/python/stylesheet/ DESTINATION ${RESOURCES_FOLDER}/kothic/src/)
  file(COPY ${OMIM_ROOT}/3party/protobuf/protobuf-3.3.0-py2.7.egg DESTINATION ${RESOURCES_FOLDER}/kothic/)

  # Generate DMG
  install(DIRECTORY ${DATA_DIR}/styles DESTINATION .)
  set(BUNDLES
    ${CMAKE_BINARY_DIR}/generator_tool.app
    ${CMAKE_BINARY_DIR}/skin_generator_tool.app
    ${CMAKE_BINARY_DIR}/style_tests.app
  )
  install(CODE "
    foreach(BUNDLE ${BUNDLE_FOLDER} ${BUNDLES})
      execute_process(
        COMMAND \"${QT_PATH}/bin/macdeployqt\" \"\${BUNDLE}\"
      )
    endforeach()
    foreach(BUNDLE ${BUNDLES})
      execute_process(
        COMMAND cp -rf \"\${BUNDLE}\" \"${RESOURCES_FOLDER}\"
      )
    endforeach()
  ")
  install(TARGETS ${PROJECT_NAME} DESTINATION .)

  set(CPACK_GENERATOR DragNDrop)
  set(CPACK_DMG_FORMAT UDZO)
  set(CPACK_DMG_VOLUME_NAME ${BUNDLE_NAME})
  set(CPACK_PACKAGE_ICON ${PROJECT_SOURCE_DIR}/res/mac/designer.icns)
  include(CPack)
elseif (BUILD_STANDALONE)
  add_custom_command(TARGET desktop POST_BUILD
    COMMAND "${QT_PATH}/bin/macdeployqt" ${BUNDLE_FOLDER}
    COMMAND ${Python3_EXECUTABLE} "${OMIM_ROOT}/tools/macdeployqtfix/macdeployqtfix.py" -q -nl ${BUNDLE_FOLDER}/Contents/MacOS/${BUNDLE_NAME} ${QT_PATH}
    COMMAND echo "Fixing Qt dependencies finished."
    VERBATIM
    COMMENT "Fixing Qt dependencies for standalone desktop app"
  )
endif()

add_subdirectory(qt_common)
