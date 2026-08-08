project(style_tests)

set(SRC
  classificator_tests.cpp
  dashes_test.cpp
  helpers.hpp
  style_symbols_consistency_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    base
    indexer
  PRIVATE
    coding
    drape
    drape_frontend
    platform
)

