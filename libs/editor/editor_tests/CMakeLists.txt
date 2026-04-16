project(editor_tests)

set(SRC
  config_loader_test.cpp
  editor_config_test.cpp
  editor_notes_test.cpp
  feature_matcher_test.cpp
  feature_type_to_osm_test.cpp
  match_by_geometry_test.cpp
  new_feature_categories_test.cpp
  opening_hours_ui_test.cpp
  osm_editor_test.cpp
  osm_editor_test.hpp
  ui2oh_test.cpp
  xml_feature_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    base
    editor
    generator_tests_support
    indexer
    platform
    storage
  PRIVATE
    editor_tests_support
    geometry
    i18n
    platform_tests_support
    search
)
