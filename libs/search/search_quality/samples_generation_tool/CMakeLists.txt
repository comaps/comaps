project(samples_generation_tool)

set(SRC samples_generation_tool.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    base
    geometry
    i18n
    indexer
    platform
    search
    search_quality
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    gflags::gflags
)
