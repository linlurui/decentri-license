cmake_minimum_required(VERSION 3.15)
project(DecentriLicense)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Debug output disabled for production
# add_definitions(-DDECENTRILICENSE_DEBUG)

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

# Testing disabled for production build

# Platform-specific package finding
if(APPLE)
    # Find Security framework for macOS
    find_library(SECURITY_FRAMEWORK Security)
elseif(WIN32)
    # Windows libraries are handled later
elseif(UNIX AND NOT APPLE)
    # Find libsecret for Linux
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBSECRET REQUIRED libsecret-1)
endif()

# Define the library
add_library(decentrilicense SHARED
    src/crypto_utils.cpp
    src/decentrilicense_client.cpp
    src/election_manager.cpp
    src/network_manager.cpp
    src/token_manager.cpp
    src/environment_checker.cpp
    src/state_chain_storage.cpp
    src/device_key_manager.cpp
    src/decenlicense_c.cpp
)

# Test executables removed for production build

# Set include directories
target_include_directories(decentrilicense
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third-party/asio>
    PRIVATE
        src
)

# Link libraries
target_link_libraries(decentrilicense
    PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
)

# Platform-specific linking
if(APPLE)
    target_link_libraries(decentrilicense PRIVATE ${SECURITY_FRAMEWORK})
elseif(WIN32)
    target_link_libraries(decentrilicense PRIVATE crypt32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(decentrilicense PRIVATE ${LIBSECRET_LIBRARIES})
    target_include_directories(decentrilicense PRIVATE ${LIBSECRET_INCLUDE_DIRS})
endif()

# Compiler definitions
target_compile_definitions(decentrilicense
    PRIVATE
        ASIO_STANDALONE
        USE_GMSSL
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(decentrilicense PRIVATE _WIN32_WINNT=0x0601)
    target_compile_definitions(decentrilicense PRIVATE _WINSOCK_DEPRECATED_NO_WARNINGS)
    target_link_libraries(decentrilicense PRIVATE ws2_32 wsock32)
endif()

# Installation rules
install(TARGETS decentrilicense
    EXPORT DecentriLicenseTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include
)

install(FILES
    include/simple_token.h
    include/state_chain_storage.h
    include/decentrilicense/device_key_manager.hpp
    include/decentrilicense/root_key.hpp
    include/decentrilicense/crypto_utils.hpp
    include/decentrilicense/token_manager.hpp
    include/decentrilicense/decentrilicense_client.hpp
    include/decentrilicense/election_manager.hpp
    include/decentrilicense/network_manager.hpp
    include/decentrilicense/environment_checker.hpp
    include/decenlicense_c.h
    DESTINATION include/decentrilicense
)

# Export targets
install(EXPORT DecentriLicenseTargets
    FILE DecentriLicenseTargets.cmake
    NAMESPACE DecentriLicense::
    DESTINATION lib/cmake/DecentriLicense
)