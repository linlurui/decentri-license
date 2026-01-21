cmake_minimum_required(VERSION 3.10)
project(DecentriLicenseValidationWizard)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找必要的包
find_package(PkgConfig REQUIRED)

# 添加头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

# 添加JSON库
# 假设使用nlohmann/json，可以通过包管理器安装
# 或者将其作为子模块包含在项目中
find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp)
if(NOT NLOHMANN_JSON_INCLUDE_DIR)
    message(FATAL_ERROR "nlohmann/json not found. Please install it or set NLOHMANN_JSON_INCLUDE_DIR")
endif()
include_directories(${NLOHMANN_JSON_INCLUDE_DIR})

# 添加可执行文件
add_executable(validation_wizard validation_wizard.cpp)

# 设置库搜索路径
set(DECENTRILICENSE_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib)

# 链接必要的库
target_link_libraries(validation_wizard
    ${DECENTRILICENSE_LIB_DIR}/libdecentrilicense.dylib  # 直接链接到dylib文件
)

# 设置运行时库搜索路径
set_target_properties(validation_wizard PROPERTIES
    BUILD_RPATH "${DECENTRILICENSE_LIB_DIR}"
    INSTALL_RPATH "${DECENTRILICENSE_LIB_DIR}"
)

# 设置输出目录
set_target_properties(validation_wizard PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)