project(world_roads_builder_tool)

set(SRC
 world_roads_builder_tool.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    base
    generator
    indexer
    platform
    routing_common
    storage
    world_roads_builder
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    gflags::gflags
)
