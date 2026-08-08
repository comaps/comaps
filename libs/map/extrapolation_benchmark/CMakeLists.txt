project(extrapolation_benchmark)

set(SRC extrapolation_benchmark.cpp
)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    base
    geometry
    map
    platform
    routing
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    gflags::gflags
)
