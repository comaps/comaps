project(feature_segments_checker)

set(SRC feature_segments_checker.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    base
    generator
    geometry
    indexer
    routing_common
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    gflags::gflags
)
