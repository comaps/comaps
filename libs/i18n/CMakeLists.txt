project(i18n)

set(SRC
  duration.cpp
  duration.hpp
  localisation.cpp
  localisation.hpp
  localisation_translation.cpp
  localisation_translation.hpp
  string_utf8_multilang.cpp
  string_utf8_multilang.hpp
  transliteration.cpp
  transliteration.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    base
    coding
    platform
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    ICU::i18n # For transliteration.
)

omim_add_test_subdirectory(i18n_tests)
