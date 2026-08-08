project(mwm_diff_tests)

set(SRC
  diff_tests.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    base
    coding
    mwm_diff
    platform
)
