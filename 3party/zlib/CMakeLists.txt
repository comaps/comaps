# Minimal zlib build for platforms that don't provide a system zlib (Windows).
# Sources come from the zlib bundled in boost/beast test externals.
set(ZLIB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../boost/libs/beast/test/extern/zlib-1.2.12")

add_library(bundled_zlib STATIC
  ${ZLIB_SRC_DIR}/adler32.c
  ${ZLIB_SRC_DIR}/compress.c
  ${ZLIB_SRC_DIR}/crc32.c
  ${ZLIB_SRC_DIR}/deflate.c
  ${ZLIB_SRC_DIR}/gzclose.c
  ${ZLIB_SRC_DIR}/gzlib.c
  ${ZLIB_SRC_DIR}/gzread.c
  ${ZLIB_SRC_DIR}/gzwrite.c
  ${ZLIB_SRC_DIR}/infback.c
  ${ZLIB_SRC_DIR}/inffast.c
  ${ZLIB_SRC_DIR}/inflate.c
  ${ZLIB_SRC_DIR}/inftrees.c
  ${ZLIB_SRC_DIR}/trees.c
  ${ZLIB_SRC_DIR}/uncompr.c
  ${ZLIB_SRC_DIR}/zutil.c
)

target_include_directories(bundled_zlib PUBLIC ${ZLIB_SRC_DIR})

# Suppress warnings in third-party code.
if (MSVC)
  target_compile_options(bundled_zlib PRIVATE /W0)
else()
  target_compile_options(bundled_zlib PRIVATE -w)
endif()

# Create the ZLIB::ZLIB target that find_package(ZLIB) would normally provide.
add_library(ZLIB::ZLIB ALIAS bundled_zlib)

# Set variables so downstream find_package(ZLIB) calls are satisfied.
set(ZLIB_FOUND TRUE CACHE BOOL "" FORCE)
set(ZLIB_LIBRARIES bundled_zlib CACHE STRING "" FORCE)
set(ZLIB_INCLUDE_DIRS ${ZLIB_SRC_DIR} CACHE STRING "" FORCE)
set(ZLIB_INCLUDE_DIR ${ZLIB_SRC_DIR} CACHE PATH "" FORCE)
set(ZLIB_LIBRARY bundled_zlib CACHE STRING "" FORCE)
