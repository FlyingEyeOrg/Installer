#include <iostream>
#include <fstream>
#include <string>
#include <fmt/core.h>

#include <Windows.h>

#include "string_convertor.hpp"
#include "tar_archive.hpp"

#include <filesystem>

/*
0000000000000009 D _binary_src_assets_helloworld_txt_end
0000000000000009 A _binary_src_assets_helloworld_txt_size
0000000000000000 D _binary_src_assets_helloworld_txt_start
*/
// 声明外部符号（注意路径中的斜杠被转换成了下划线）
extern "C"
{
    extern const char _binary_src_assets_helloworld_txt_start[];
    extern const char _binary_src_assets_helloworld_txt_end[];
}

int main()
{

    std::filesystem::path file_path = "e:\\users\\lanxf01\\Desktop\\软件项目\\知识库开发\\中文目录测试.tar";
    std::filesystem::path source_dir = "e:\\users\\lanxf01\\Desktop\\软件项目\\知识库开发\\中文目录测试";
    std::filesystem::path out_dir = "e:\\users\\lanxf01\\Desktop\\软件项目\\知识库开发\\中文目录测试\\tmp";

    std::ofstream file(file_path, std::ios::binary);

    const auto gbk_string = StringConvertor::UTF8ToGBK(file_path.string());

    mtar_t tar;
    /* Open archive for writing */
    auto result = mtar_open(&tar, gbk_string.c_str(), "w");
    std::cout << "result: " << result << std::endl;

    auto write_result = mtar_write_dir_header(&tar, StringConvertor::UTF8ToGBK("中文目录测试/目录").c_str());
    std::cout << "write_result: " << write_result << std::endl;

    auto dir_string = StringConvertor::UTF8ToGBK("中文目录测试/目录/test1.txt");
    write_result = mtar_write_file_header(&tar, dir_string.c_str(), dir_string.length());
    std::cout << "write_result: " << write_result << std::endl;

    write_result = mtar_write_data(&tar, "hello wrold!", strlen("hello wrold!"));
    std::cout << "write_result: " << write_result << std::endl;

    mtar_finalize(&tar);

    /* Close archive */
    mtar_close(&tar);

    // TarArchive::CreateTarFromDirectory(file_path, source_dir);
    // TarArchive::ExtractTarArchive(file_path, out_dir);

    // 计算数据大小
    size_t size = _binary_src_assets_helloworld_txt_end -
                  _binary_src_assets_helloworld_txt_start;

    // 直接使用数据
    std::string content(_binary_src_assets_helloworld_txt_start, size);
    fmt::print(content);

    return 0;
}