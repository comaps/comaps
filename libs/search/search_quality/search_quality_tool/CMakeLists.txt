project(search_quality_tool)

set(SRC search_quality_tool.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    base
    geometry
    indexer
    platform
    search
    search_quality
    search_tests_support
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    gflags::gflags
)
