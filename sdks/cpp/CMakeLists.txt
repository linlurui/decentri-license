cmake_minimum_required(VERSION 3.15)
project(DecentriLicenseSDKExamples)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSON REQUIRED nlohmann_json)

# Add DecentriLicense library
add_subdirectory(../../dl-core build)

# Enhanced validator example
add_executable(enhanced_validator
    example/enhanced_validator.cpp
)

# Link libraries
target_link_libraries(enhanced_validator
    decentrilicense
    OpenSSL::SSL
    OpenSSL::Crypto
    ${JSON_LIBRARIES}
)

# Include directories
target_include_directories(enhanced_validator
    PRIVATE
        ../../dl-core/include
        ${JSON_INCLUDE_DIRS}
)

# Compiler definitions
target_compile_definitions(enhanced_validator
    PRIVATE
        ASIO_STANDALONE
        USE_GMSSL
)