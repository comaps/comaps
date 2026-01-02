project(monocypher)

add_library(monocypher STATIC
  monocypher.c
  monocypher.h
  monocypher-ed25519.c
  monocypher-ed25519.h
)

target_include_directories(monocypher PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(monocypher PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    UNITY_BUILD OFF
)
