cmake_minimum_required(VERSION 3.30)

project(installer)

find_package(fmt CONFIG REQUIRED)

file(GLOB_RECURSE CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# 打印文件列表（调试用）
message(STATUS "Found CPP files: ${CPP_FILES}")

# 根据构建类型决定是否启用控制台
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "构建类型: Debug - 启用控制台窗口")
    # Debug 版本：显示控制台窗口（便于调试输出）
    add_executable(${PROJECT_NAME} ${CPP_FILES})
else()
    message(STATUS "构建类型: ${CMAKE_BUILD_TYPE} - 禁用控制台窗口")
    # Release 版本：纯 GUI 应用程序
    add_executable(${PROJECT_NAME} WIN32 ${CPP_FILES})
endif()

# 链接必要的 Windows 库
target_link_libraries(${PROJECT_NAME} PRIVATE 
    fmt::fmt
    comctl32      # 通用控件库
    user32        # 用户界面函数
    gdi32         # 图形设备接口
    kernel32      # 核心系统函数
)
