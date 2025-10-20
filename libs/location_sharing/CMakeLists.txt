project(location_sharing)

set(SRC
  location_sharing_types.cpp
  location_sharing_types.hpp
  crypto_util.cpp
  crypto_util.hpp
  location_sharing_session.cpp
  location_sharing_session.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    base
    platform
)

# Link OpenSSL on Linux (not Android - Android will use Java crypto)
if (${PLATFORM_LINUX} AND NOT ANDROID)
  find_package(OpenSSL REQUIRED)
  target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

# Link CommonCrypto/Security on Apple platforms
if (APPLE)
  find_library(SECURITY_FRAMEWORK Security)
  find_library(COMMONCRYPTO_FRAMEWORK CommonCrypto)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${SECURITY_FRAMEWORK})
endif()

# Link BCrypt on Windows
if (WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE bcrypt)
endif()

omim_add_test_subdirectory(location_sharing_tests)
