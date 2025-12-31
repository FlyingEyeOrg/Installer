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
#include "tar_archive.hpp"
#include "zstd_compression.hpp"

EMBED_UNIX_PATH(hello_txt, "src/assets/helloworld.txt")

/*
std::cout << "tar..." << std::endl;
    tar_archive::create_from_directory(
        R"(e:\users\lanxf01\Desktop\软件项目\知识库开发\中文目录测试.tar)",
        R"(e:\users\lanxf01\Desktop\软件项目\知识库开发\中文目录测试)");

    const auto bytes = file::read_all_bytes(
        R"(e:\users\lanxf01\Desktop\软件项目\知识库开发\中文目录测试.tar)");

    auto span = std::as_bytes(std::span(bytes));

    std::cout << "compress file ..." << std::endl;
    auto compresed_data = zstd_compression::compress(span);
    std::cout << "compress complected" << std::endl;

    file::write_all_bytes(
        R"(e:\users\lanxf01\Desktop\软件项目\知识库开发\中文目录测试.tar.zstd)",
        compresed_data.value());

    std::cout << "decompress file ..." << std::endl;
    auto data = zstd_compression::decompress(
        std::as_bytes(std::span(compresed_data.value())));
    std::cout << "compress complected" << std::endl;

    file::write_all_bytes(
        R"(e:\users\lanxf01\Desktop\软件项目\知识库开发\中文目录测试.zstd.tar)",
        data.value());

    tar_archive::extract(
        R"(e:\users\lanxf01\Desktop\软件项目\知识库开发\中文目录测试.zstd.tar)",
        R"(e:\users\lanxf01\Desktop\软件项目\知识库开发\tmp)");
*/

#include "memory_stream/memory_chunk_tests.hpp"
#include "memory_stream/memory_stream.hpp"
#include "memory_stream/memory_stream_tests.hpp"


int main() {
    memory_chunk_tests::run_all_tests();
    memory_stream_tests::run_all_tests();
    return 0;
}

/*
// 1. 创建流缓冲区
    stream_buffer<int> buffer;  // 每个块1024个int

    fmt::print("=== 基本操作示例 ===\n");

    // 2. 写入数据
    // 方法1: push_back 写入单个元素
    for (int i = 0; i < 10; ++i) {
        buffer.push_back(i);  // 写入单个元素
    }
    fmt::print("写入10个元素后，缓冲区大小: {}\n", buffer.size());

    // 方法2: write 写入范围
    std::vector<int> data = {10, 20, 30, 40, 50};
    buffer.write(data.begin(), data.end());  // 批量写入
    fmt::print("批量写入5个元素后，缓冲区大小: {}\n", buffer.size());

    // 3. 读取数据
    fmt::print("\n=== 读取操作 ===\n");

    // peek: 查看但不删除
    int peek_value;
    if (buffer.peek(peek_value)) {
        fmt::print("peek 查看第一个元素: {}\n", peek_value);
    }
    fmt::print("peek 后缓冲区大小: {}\n", buffer.size());

    // read: 读取到容器
    std::vector<int> read_data(5);
    std::size_t read_count = buffer.read(read_data.begin(), 5);
    fmt::print("读取了 {} 个元素: {}\n", read_count, read_data);
    fmt::print("读取后缓冲区大小: {}\n", buffer.size());

    // pop_front: 逐个读取
    fmt::print("\n逐个读取剩余元素:\n");
    int value;
    std::vector<int> remaining_values;
    while (buffer.pop_front(value)) {
        remaining_values.push_back(value);
    }
    fmt::print("剩余元素: {}\n", remaining_values);
    fmt::print("缓冲区大小: {}\n", buffer.size());
*/