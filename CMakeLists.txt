cmake_minimum_required(VERSION 3.30)

project(installer)

find_package(fmt CONFIG REQUIRED)

# 查找 objcopy
find_program(OBJCOPY objcopy)

# 要嵌入的文件
set(EMBED_FILES 
    src/assets/helloworld.txt
)

# 为每个文件生成 .o 文件
foreach(file ${EMBED_FILES})
    get_filename_component(name ${file} NAME_WE)
    set(obj_file ${CMAKE_CURRENT_BINARY_DIR}/${name}.o)  
    
    add_custom_command(
        OUTPUT ${obj_file}
        COMMAND ${OBJCOPY} -I binary -O pe-x86-64 -B i386:x86-64
                ${file} ${obj_file}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
        DEPENDS ${file}
        COMMENT "嵌入文件: ${file} -> ${obj_file}"
    )
    list(APPEND EMBED_OBJS ${obj_file})
endforeach()

file(GLOB_RECURSE CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# 打印文件列表（调试用）
message(STATUS "Found CPP files: ${CPP_FILES}")

# 根据构建类型决定是否启用控制台
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "构建类型: Debug - 启用控制台窗口")
    # Debug 版本：显示控制台窗口（便于调试输出）
    add_executable(${PROJECT_NAME} ${CPP_FILES} ${EMBED_OBJS})

    if(MSVC)
        # 明确设置控制台程序
        set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE FALSE)
        target_link_options(${PROJECT_NAME} PRIVATE "/SUBSYSTEM:CONSOLE")
    endif()
else()
    message(STATUS "构建类型: ${CMAKE_BUILD_TYPE} - 禁用控制台窗口")
    # Release 版本：纯 GUI 应用程序
    add_executable(${PROJECT_NAME} WIN32 ${CPP_FILES} ${EMBED_OBJS})
endif()

# 链接必要的 Windows 库
target_link_libraries(${PROJECT_NAME} PRIVATE 
    fmt::fmt-header-only
    comctl32      # 通用控件库
    user32        # 用户界面函数
    gdi32         # 图形设备接口
    kernel32      # 核心系统函数
)
