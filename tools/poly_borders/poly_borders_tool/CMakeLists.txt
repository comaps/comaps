project(poly_borders_tool)

set(SRC poly_borders_tool.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    base
    platform
    poly_borders
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    gflags::gflags
)
