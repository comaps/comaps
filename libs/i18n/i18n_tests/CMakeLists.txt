project(i18n_tests)

set(SRC
  duration_tests.cpp
  string_utf8_multilang_tests.cpp
  transliteration_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    base
    i18n
    platform
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    utf8cpp::utf8cpp
)
