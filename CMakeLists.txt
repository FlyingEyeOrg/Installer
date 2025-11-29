cmake_minimum_required(VERSION 3.10)

project(installer)

find_package(OpenGL REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

file(GLOB_RECURSE CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# 打印文件列表（调试用）
message(STATUS "Found CPP files: ${CPP_FILES}")

# 定义可执行文件
add_executable(${CMAKE_PROJECT_NAME} ${CPP_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui::imgui)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)