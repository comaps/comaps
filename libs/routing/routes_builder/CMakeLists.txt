project(routes_builder)

set(SRC
  data_source_storage.cpp
  data_source_storage.hpp
  routes_builder.cpp
  routes_builder.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    base
    geometry
    indexer
    platform
    routing
    routing_common
    storage
    traffic
  PRIVATE
    coding
)

omim_add_tool_subdirectory(routes_builder_tool)
