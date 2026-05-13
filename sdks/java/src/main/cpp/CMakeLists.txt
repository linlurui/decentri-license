cmake_minimum_required(VERSION 3.15)
project(DecentriLicenseJNI)

# Set JAVA_HOME if not set
if(NOT DEFINED ENV{JAVA_HOME})
    set(ENV{JAVA_HOME} "/opt/homebrew/opt/openjdk@17")
endif()

# Find JNI
find_package(JNI REQUIRED)

# Create the JNI library
add_library(decenlicense_jni SHARED decenlicense_jni.cpp)

# Set include directories
target_include_directories(decenlicense_jni
    PRIVATE
        ${JNI_INCLUDE_DIRS}
        ../../../../../dl-core/include
)

# Link with the core library and JNI
target_link_libraries(decenlicense_jni
    PRIVATE
        /Volumes/project/decentri-license-issuer/dl-core/build/libdecentrilicense.dylib
        ${JNI_LIBRARIES}
)

# Set compile options
target_compile_options(decenlicense_jni PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

# Install rules
install(TARGETS decenlicense_jni
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)