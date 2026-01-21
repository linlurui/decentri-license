cmake_minimum_required(VERSION 3.15)

# Add project definition
project(decenlicense_c)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

set(DECENLICENSE_C_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../dl-core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../dl-core/third-party/asio
    ${OPENSSL_INCLUDE_DIR}
)

# Find the core library
find_library(DECENTRILICENSE_CORE_LIBRARY 
    NAMES decentrilicense
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../../dl-core/build
    NO_DEFAULT_PATH
)

# Create validation wizard executable
add_executable(validation_wizard example/validation_wizard.c)

# Include dirs for validation wizard
target_include_directories(validation_wizard PRIVATE ${DECENLICENSE_C_INCLUDE_DIRS})

# Link validation wizard directly with the dl-core library (exports the real C API) and OpenSSL
target_link_libraries(validation_wizard
    PRIVATE
        ${DECENTRILICENSE_CORE_LIBRARY}
        ${OPENSSL_LIBRARIES}
)

# Install rules
install(TARGETS validation_wizard
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES decenlicense_c.h
    DESTINATION include
)