#include <Windows.h>
#include <fmt/core.h>
#include <fmt/ranges.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <streambuf>
#include <string>
#include <string_view>
#include <vector>

#include "embed_assets.hpp"
#include "file.hpp"
#include "string_convertor.hpp"
#include "zstd_compression.hpp"

// EMBED_UNIX_PATH(hello_txt, "src/assets/helloworld.txt")
// EMBED_UNIX_PATH(net8_0_windows, "src/assets/net8.0-windows.tar")
EMBED_UNIX_PATH(net8_0_windows_zstd, "src/assets/net8.0-windows.tar.zstd")

#include "tar/tar.hpp"
#include "tar/tar_tests.hpp"

int main() {
    // auto bytes = net8_0_windows_zstd_resource::data();

    // auto decompressed = zstd_compression::decompress(std::span<const
    // std::byte>(
    //     reinterpret_cast<const std::byte*>(bytes.data()), bytes.size()));

    // file::write_all_bytes(
    //     R"(e:\users\lanxf01\Desktop\demo\Launcher\bin\Debug\net8.0-windows-1.tar)",
    //     decompressed.value());

    // tar::writer w;
    // w.add_directory(
    //     R"(e:\users\lanxf01\Desktop\demo\Launcher\bin\Debug\net8.0-windows)",
    //     "net8.0-windows");
    // auto bytes = w.get_vector();

    // auto compressed = zstd_compression::compress(std::span<const std::byte>(
    //     reinterpret_cast<const std::byte*>(bytes.data()), bytes.size()));

    // file::write_all_bytes(
    //     R"(e:\users\lanxf01\Desktop\demo\Launcher\bin\Debug\net8.0-windows.tar.zstd)",
    //     compressed.value());

    // auto resource_span = net8_0_windows_resource::bytes();
    // tar::reader r(reinterpret_cast<const char*>(
    //                   resource_span.data()),  // 转换为 const char*
    //               resource_span.size()        // 获取数据大小
    // );

    // r.extract_all(
    //     R"(e:\users\lanxf01\Desktop\demo\Launcher\bin\Debug\net8.0-windows1)");

    // tar::create_archive_from_directory(
    //     R"(e:\users\lanxf01\Desktop\demo\Launcher\bin\Debug\net8.0-windows.tar)",
    //     R"(e:\users\lanxf01\Desktop\demo\Launcher\bin\Debug\net8.0-windows)");

    tar::extract_archive(
        R"(e:\users\lanxf01\Desktop\demo\Launcher\bin\Debug\net8.0-windows-1.tar)",
        R"(e:\users\lanxf01\Desktop\demo\Launcher\bin\Debug\net8.0-windows1)");
    return 0;
}
