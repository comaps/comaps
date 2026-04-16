project(i18n)

set(SRC
  localisation.cpp
  localisation.hpp
  localisation_translation.cpp
  localisation_translation.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    storage
  PRIVATE
    base
    coding
    platform
)
