project(opening_hours)

set(SRC
  opening_hours.hpp
  opening_hours.cpp
  opening_hours_parsers.hpp
  opening_hours_parsers_terminals.cpp
  parse_opening_hours.hpp
  parse_opening_hours.cpp
  parse_years.cpp
  parse_weekdays.cpp
  parse_weeks.cpp
  parse_timespans.cpp
  parse_months.cpp
  rules_evaluation_private.hpp
  rules_evaluation.hpp
  rules_evaluation.cpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME} INTERFACE .)

if (MSVC)
  # Boost Spirit templates require enormous memory with MSVC /Od /RTC1 /Zi debug settings.
  # Strip incompatible/memory-heavy flags, then add /O2 to override /Od.
  string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
  string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
  target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:/O2>)
endif()

omim_add_test_subdirectory(opening_hours_tests)
omim_add_test_subdirectory(opening_hours_integration_tests)
omim_add_test_subdirectory(opening_hours_supported_features_tests)
