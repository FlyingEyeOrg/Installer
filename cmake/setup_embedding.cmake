# setup_embedding.cmake
# 设置资源嵌入的通用配置

function(setup_embedding_config TARGET_NAME)
    # 转换为 Unix 风格路径（MinGW 兼容）
    string(REPLACE "\\" "/" CMAKE_CURRENT_SOURCE_DIR_UNIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
    
    # 确保路径以斜杠结尾
    if(NOT CMAKE_CURRENT_SOURCE_DIR_UNIX_PATH MATCHES "/$")
        set(CMAKE_CURRENT_SOURCE_DIR_UNIX_PATH "${CMAKE_CURRENT_SOURCE_DIR_UNIX_PATH}/")
    endif()
    
    # 设置汇编宏定义，传递资源文件路径
    target_compile_definitions(${TARGET_NAME} PRIVATE
        CMAKE_SOURCE_DIR_UNIX_PATH="${CMAKE_CURRENT_SOURCE_DIR_UNIX_PATH}"
    )
    
    # 可选：记录日志
    message(STATUS "Setup embedding for target: ${TARGET_NAME}")
    message(STATUS "  Source dir: ${CMAKE_CURRENT_SOURCE_DIR_UNIX_PATH}")
endfunction()