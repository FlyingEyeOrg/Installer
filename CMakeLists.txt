cmake_minimum_required(VERSION 3.30)

project(installer LANGUAGES C CXX)

# 包含自定义模块
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(fmt CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)

# 包含所有源代码 CPP 文件
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

# 包含所有资源文件
file(GLOB_RECURSE RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/*.rc")
file(GLOB_RECURSE ASSET_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/*.*")

# 资源文件变更，触发主函数重新编译
set_source_files_properties(src/main.cpp PROPERTIES
    OBJECT_DEPENDS "${ASSET_FILES}"
)

# 添加资源依赖
target_sources(${PROJECT_NAME} PRIVATE ${RC_FILES})

# 转换为 Unix 风格路径（MinGW 兼容）
string(REPLACE "\\" "/" CMAKE_CURRENT_SOURCE_DIR_UNIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

# 设置汇编宏定义，传递资源文件路径
target_compile_definitions(${PROJECT_NAME} PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR_UNIX_PATH}"
)

# 链接必要的 Windows 库
target_link_libraries(${PROJECT_NAME} PRIVATE 
    fmt::fmt-header-only
    zstd::libzstd
    comctl32      # 通用控件库
    user32        # 用户界面函数
    gdi32         # 图形设备接口
    kernel32      # 核心系统函数
)

# 包含头文件目录
target_include_directories(${PROJECT_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)