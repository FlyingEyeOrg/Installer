# ============================================================================
# ObjectConvert.cmake - 二进制文件嵌入模块
# 
# 功能：将二进制文件转换为目标文件(.o)，以便嵌入到可执行文件中
# 通过 objcopy 工具将二进制数据转换为链接器可识别的目标文件格式
# ============================================================================

# convert_file_to_obj 函数
# 参数：
#   OUTPUT_VAR - 输出变量名，用于接收生成的目标文件列表
#   ARGN       - 可变参数列表，包含所有要嵌入的二进制文件路径
# 返回值：
#   通过 OUTPUT_VAR 参数返回生成的目标文件列表
# 示例：
#   convert_file_to_obj(EMBED_OBJS 
#       assets/icon.ico
#       assets/data.bin
#   )
#   add_executable(myapp main.cpp ${EMBED_OBJS})
function(convert_file_to_obj OUTPUT_VAR)
    # 查找 objcopy 工具
    # REQUIRED 表示如果找不到该工具，则配置阶段会报错
    find_program(OBJCOPY objcopy REQUIRED)
    find_program(NM nm REQUIRED)
    
    # 初始化目标文件列表变量
    set(obj_files "")
    
    # 遍历所有传入的二进制文件
    # ARGN 包含函数调用时除第一个参数外的所有剩余参数
    foreach(file ${ARGN})
        # 获取不带扩展名的文件名
        # 例如：assets/icon.ico -> icon
        get_filename_component(name ${file} NAME_WE)
        
        # 在构建目录中生成对应的目标文件名
        # 例如：${CMAKE_CURRENT_BINARY_DIR}/icon.o
        set(obj_file ${CMAKE_CURRENT_BINARY_DIR}/obj/${name}.o)
        
        # 添加自定义构建命令，用于生成目标文件
        add_custom_command(
            # 指定输出文件
            OUTPUT ${obj_file}
            
            # 执行 objcopy 命令转换文件格式：
            # -I binary     : 输入格式为原始二进制
            # -O pe-x86-64  : 输出格式为 Windows PE 64位
            # -B i386:x86-64: 架构为 x86-64
            COMMAND ${OBJCOPY} -I binary -O pe-x86-64 -B i386:x86-64
                    ${file} ${obj_file}
            COMMAND ${NM} ${obj_file}
            
            # 设置工作目录为源代码目录，确保相对路径正确
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
            
            # 指定依赖文件，当源文件更新时会重新生成
            DEPENDS ${file}
            
            # 构建时的提示信息
            COMMENT "嵌入文件: ${file} -> ${obj_file}"
        )

        # 将生成的目标文件添加到列表中
        list(APPEND obj_files ${obj_file})
    endforeach()

    # 将结果返回给调用者
    # PARENT_SCOPE 将变量设置到父作用域
    set(${OUTPUT_VAR} ${obj_files} PARENT_SCOPE)
endfunction()