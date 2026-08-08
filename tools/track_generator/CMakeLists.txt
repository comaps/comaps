project(track_generator_tool)

set(SRC
  track_generator_tool.cpp
  utils.cpp
  utils.hpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    routing
  PRIVATE
    base
    coding
    geometry
    kml
    platform
    routes_builder
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    expat::expat
    gflags::gflags
)
