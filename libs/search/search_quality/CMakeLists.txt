project(search_quality)

set(SRC
  helpers.cpp
  helpers.hpp
  helpers_json.cpp
  helpers_json.hpp
  matcher.cpp
  matcher.hpp
  sample.cpp
  sample.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    base
    cppjansson
    geometry
    search_tests_support  # TestSearchEngine in helpers.cpp
  PRIVATE
    coding
    i18n
    indexer
    platform
    search
)

if (NOT SKIP_QT_GUI)
  omim_add_tool_subdirectory(assessment_tool)
endif()

omim_add_tool_subdirectory(features_collector_tool)
omim_add_tool_subdirectory(samples_generation_tool)
omim_add_tool_subdirectory(search_quality_tool)

omim_add_test_subdirectory(search_quality_tests)
