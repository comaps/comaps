project(gtfs_converter)

omim_add_executable(${PROJECT_NAME} gtfs_converter.cpp)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    base
    generator
    platform
    world_feed
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    gflags::gflags
)
