project(routing_quality_tool)

set(SRC
  benchmark_results.cpp
  benchmark_results.hpp
  benchmark_stat.cpp
  benchmark_stat.hpp
  error_type_counter.cpp
  error_type_counter.hpp
  routing_quality_tool.cpp
  utils.cpp
  utils.hpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    base
    geometry
    platform
    routing
    routes_builder
    routing_api
  PRIVATE
    kml
    routing_quality
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    gflags::gflags
)
