project(routing_quality)

set(SRC
  waypoints.cpp
  waypoints.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    geometry
    routes_builder
  PRIVATE
    base
    routing
)

add_subdirectory(api)
omim_add_tool_subdirectory(routing_quality_tool)

omim_add_test_subdirectory(routing_quality_tests)
