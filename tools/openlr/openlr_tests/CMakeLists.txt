project(openlr_tests)

set(SRC decoded_path_test.cpp)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    base
    generator_tests_support
    indexer
    openlr
    platform
    platform_tests_support
)
