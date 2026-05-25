cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(installer LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(setup_embedding)

find_package(fmt CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(WinElement CONFIG REQUIRED)

# 将非入口源文件编译为静态库，便于测试链接
file(GLOB LIB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/file.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/tar.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zstd_compression.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/installer_context.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/installer_controller.cpp"
)

add_library(installer_lib STATIC ${LIB_SOURCES})
setup_embedding_config(installer_lib)
target_include_directories(installer_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(installer_lib PUBLIC
    fmt::fmt-header-only
    zstd::libzstd
    kernel32
)

# 主可执行文件
add_executable(${PROJECT_NAME} src/main.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
)
target_link_options(${PROJECT_NAME} PRIVATE
    $<$<NOT:$<CONFIG:Debug>>:/ENTRY:mainCRTStartup>
)

file(GLOB_RECURSE RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/*.rc")
file(GLOB_RECURSE ASSET_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/*.*")

set_source_files_properties(src/main.cpp PROPERTIES
    OBJECT_DEPENDS "${ASSET_FILES}"
)
target_sources(${PROJECT_NAME} PRIVATE ${RC_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE
    installer_lib
    WinElement::winelement
)

# 测试（默认关闭，通过 -DBUILD_TESTING=ON 启用）
option(BUILD_TESTING "Build tests" OFF)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
