project(routes_builder_tool)

set(SRC
  routes_builder_tool.cpp
  utils.cpp
  utils.hpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    routes_builder
    routing_api
  PRIVATE
    base
    geometry
    platform
    routing
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    gflags::gflags
)
