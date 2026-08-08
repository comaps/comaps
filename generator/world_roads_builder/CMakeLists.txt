project(world_roads_builder)

set(SRC
  world_roads_builder.cpp
  world_roads_builder.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    generator
    geometry
    indexer
    routing
    routing_common
  PRIVATE
    base
)

omim_add_tool_subdirectory(world_roads_builder_tool)
