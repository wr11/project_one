cmake_minimum_required(VERSION 3.15)

# 设置默认版本（如果未定义）
if(NOT DEFINED PROJECT_VERSION)
    set(PROJECT_VERSION "1.0.0")
endif()

# 在 Windows 上，如果没有指定生成器，尝试自动检测编译器
if(WIN32 AND NOT CMAKE_GENERATOR)
    # 检查是否有 gcc
    find_program(GCC_FOUND NAMES gcc)
    if(GCC_FOUND)
        set(CMAKE_GENERATOR "MinGW Makefiles" CACHE STRING "CMake generator")
        message(STATUS "Auto-detected MinGW, using MinGW Makefiles generator")
    endif()
endif()

project(GearEngine 
    VERSION ${PROJECT_VERSION}
    LANGUAGES C
    DESCRIPTION "A server engine using libuv, lua, and libbson"
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 添加 CMake 辅助函数路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# 加载 mongo-c-driver 辅助函数（为 libbson 提供）
include(cmake/mongo-cmake-helpers.cmake)

# 修复 libbson 构建问题
include(cmake/fix-libbson.cmake)

# 设置输出目录
# 注意：可执行文件的输出目录在各自的 CMakeLists.txt 中单独设置
# 库文件仍然输出到 build/lib
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 配置 libbson 选项
if(NOT DEFINED ENABLE_STATIC)
    set(ENABLE_STATIC ON)
endif()
if(NOT DEFINED ENABLE_SHARED)
    set(ENABLE_SHARED ON)
endif()

# 设置 libbson 需要的变量（如果 libbson 需要这些）
# 优先使用真实的 mongo-c-driver 目录（如果存在）
if(EXISTS "${CMAKE_SOURCE_DIR}/mongo-c-driver")
    set(mongo-c-driver_SOURCE_DIR "${CMAKE_SOURCE_DIR}/mongo-c-driver" CACHE PATH "mongo-c-driver source directory")
    message(STATUS "Using mongo-c-driver from: ${mongo-c-driver_SOURCE_DIR}")
else()
    set(mongo-c-driver_SOURCE_DIR "${CMAKE_SOURCE_DIR}/libbson" CACHE PATH "mongo-c-driver source directory")
    message(STATUS "Using libbson directory as mongo-c-driver fallback: ${mongo-c-driver_SOURCE_DIR}")
endif()

if(NOT DEFINED mongo-c-driver_VERSION_PRERELEASE)
    set(mongo-c-driver_VERSION_PRERELEASE "")
endif()
if(NOT DEFINED mongo-c-driver_VERSION_FULL)
    set(mongo-c-driver_VERSION_FULL ${PROJECT_VERSION})
endif()
if(NOT DEFINED mongo-c-driver_BINARY_DIR)
    set(mongo-c-driver_BINARY_DIR ${CMAKE_BINARY_DIR}/libbson)
endif()

# 设置包含目录（必须在 add_subdirectory 之前）
include_directories(
    ${CMAKE_SOURCE_DIR}/libuv/include
    ${CMAKE_SOURCE_DIR}/libbson/src/bson
    ${CMAKE_SOURCE_DIR}/lua/src
)

# 添加子目录
add_subdirectory(libuv)
add_subdirectory(libbson)
add_subdirectory(src)
add_subdirectory(client)
# Add benchmark subdirectory if it exists
if(EXISTS "${CMAKE_SOURCE_DIR}/../benchmark/CMakeLists.txt")
    add_subdirectory(../benchmark benchmark)
endif()