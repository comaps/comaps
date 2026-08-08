project(srtm_coverage_checker)

set(SRC
  srtm_coverage_checker.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    base
    generator
    geometry
    indexer
    platform
    routing
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    gflags::gflags
)
