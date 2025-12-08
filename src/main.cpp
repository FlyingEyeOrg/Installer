#include <iostream>
#include <fstream>
#include <string>
#include <fmt/core.h>

#include <Windows.h>

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

int wmain()
{
    MessageBoxA(nullptr, "Hello", "Hello", MB_OK);
    MessageBox(nullptr, L"你好", L"你好", MB_OK);

    // 计算数据大小
    size_t size = _binary_src_assets_helloworld_txt_end -
                  _binary_src_assets_helloworld_txt_start;

    std::cout << "Hello World!" << std::endl;

    // 直接使用数据
    std::string content(_binary_src_assets_helloworld_txt_start, size);
    fmt::print(content);

    MessageBoxA(nullptr, content.c_str(), "提示", MB_OK);

    return 0;
}