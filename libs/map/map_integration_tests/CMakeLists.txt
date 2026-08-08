project(map_integration_tests)

set(SRC
  interactive_search_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    base
    map
    search
    search_tests_support
)
