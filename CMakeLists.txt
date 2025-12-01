cmake_minimum_required(VERSION 3.30)

project(installer)

find_package(fmt CONFIG REQUIRED)

file(GLOB_RECURSE CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# 打印文件列表（调试用）
message(STATUS "Found CPP files: ${CPP_FILES}")

# 定义可执行文件
add_executable(${CMAKE_PROJECT_NAME} WIN32 ${CPP_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)
