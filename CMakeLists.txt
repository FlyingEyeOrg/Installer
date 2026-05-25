cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(installer LANGUAGES C CXX)

# 包含自定义模块
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(setup_embedding)

find_package(fmt CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(WinElement CONFIG REQUIRED)

# 包含所有源代码 CPP 文件
file(GLOB_RECURSE CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# 打印文件列表（调试用）
message(STATUS "Found CPP files: ${CPP_FILES}")

add_executable(${PROJECT_NAME} ${CPP_FILES})
# Debug 显示控制台，Release 纯 GUI（用 main() 作为入口点）
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
)
target_link_options(${PROJECT_NAME} PRIVATE
    $<$<NOT:$<CONFIG:Debug>>:/ENTRY:mainCRTStartup>
)
message(STATUS "控制台窗口: Debug=ON, Release=OFF")

# 文件嵌入宏定义配置
setup_embedding_config(${PROJECT_NAME})

# 包含所有资源文件
file(GLOB_RECURSE RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/*.rc")
file(GLOB_RECURSE ASSET_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/*.*")

# 资源文件变更，触发主函数重新编译
set_source_files_properties(src/main.cpp PROPERTIES
    OBJECT_DEPENDS "${ASSET_FILES}"
)

# 添加资源依赖，将会设置 exe 图标
target_sources(${PROJECT_NAME} PRIVATE ${RC_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    fmt::fmt-header-only
    zstd::libzstd
    WinElement::winelement
    kernel32
)

# 包含头文件目录
target_include_directories(${PROJECT_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)