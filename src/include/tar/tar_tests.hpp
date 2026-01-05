#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <vector>

#include "fmt/color.h"
#include "fmt/format.h"
#include "fmt/ostream.h"
#include "tar.hpp"

namespace tar_tests {

#undef max
#undef min

namespace fs = std::filesystem;

// 格式化工具函数
void print_section(const std::string& title) {
    fmt::print(fg(fmt::color::light_blue) | fmt::emphasis::bold, "\n{}\n",
               title);
    fmt::print("{}\n", std::string(title.length(), '='));
}

void print_subsection(const std::string& title) {
    fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold, "  {}\n", title);
}

void print_success(const std::string& message) {
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "  ✓ {}\n",
               message);
}

void print_error(const std::string& message) {
    fmt::print(fg(fmt::color::red) | fmt::emphasis::bold, "  ✗ {}\n", message);
}

void print_info(const std::string& message) {
    fmt::print(fg(fmt::color::light_gray), "  {}\n", message);
}

void print_warning(const std::string& message) {
    fmt::print(fg(fmt::color::yellow), "  ⚠ {}\n", message);
}

// 测试工具函数
void test_utils() {
    print_section("测试工具函数");

    // 测试 parse_octal
    {
        const char octal_data[] = "777    ";
        auto result = tar::parse_octal(octal_data, sizeof(octal_data));
        print_info(
            fmt::format("parse_octal(\"777\") = {} (0{:o})", result, result));
        assert(result == 0777);
    }

    // 测试 write_octal
    {
        char buffer[12] = {0};
        tar::write_octal(0644, buffer, sizeof(buffer));
        std::string octal_str;
        for (int i = 0; i < 12; ++i) {
            if (buffer[i] == 0) break;
            octal_str.push_back(buffer[i]);
        }
        print_info(fmt::format("write_octal(0644) = \"{}\"", octal_str));
    }

    // 测试 calculate_checksum
    {
        tar::header hdr = {};
        std::strcpy(hdr.name, "test.txt");
        std::strcpy(hdr.magic, "ustar");
        auto checksum = tar::calculate_checksum(hdr);
        print_info(fmt::format("calculate_checksum = {}", checksum));
    }

    print_success("工具函数测试通过");
}

// 创建测试文件
void create_test_file(const fs::path& path, const std::string& content) {
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error(
            fmt::format("无法创建测试文件: {}", path.string()));
    }
    file << content;
}

// 验证文件内容
bool verify_file_content(const fs::path& path, const std::string& expected) {
    std::ifstream file(path);
    if (!file) return false;

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return content == expected;
}

// 测试内存流读取功能
void test_memory_stream() {
    print_section("测试内存流读取功能");

    const fs::path test_dir = "test_memory_stream";
    const fs::path extract_dir = test_dir / "extracted";

    // 清理之前的测试目录
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);

    // 创建测试文件
    const fs::path test_file1 = test_dir / "mem1.txt";
    const fs::path test_file2 = test_dir / "mem2.txt";
    create_test_file(test_file1, "内存流测试文件1");
    create_test_file(test_file2, "内存流测试文件2");

    try {
        // 打包文件到内存
        print_subsection("打包文件到内存");
        tar::writer w;
        w.add_file(test_file1, "memory_file1.txt");
        w.add_file(test_file2, "memory_file2.txt");

        // 获取内存数据
        auto data = w.get_vector();
        print_info(fmt::format("内存数据大小: {} bytes", data.size()));

        // 测试从 vector<char> 读取
        print_subsection("测试从 vector<char> 读取");
        {
            tar::reader r1(data);
            fs::create_directories(extract_dir / "from_vector");
            r1.extract_all(extract_dir / "from_vector");

            if (fs::exists(extract_dir / "from_vector" / "memory_file1.txt") &&
                fs::exists(extract_dir / "from_vector" / "memory_file2.txt")) {
                print_success("从 vector<char> 解压成功");
            } else {
                print_error("从 vector<char> 解压失败");
            }
        }

        // 测试从 string 读取
        print_subsection("测试从 string 读取");
        {
            auto str_data = w.get_data();
            tar::reader r2(str_data);
            fs::create_directories(extract_dir / "from_string");
            r2.extract_all(extract_dir / "from_string");

            if (fs::exists(extract_dir / "from_string" / "memory_file1.txt") &&
                fs::exists(extract_dir / "from_string" / "memory_file2.txt")) {
                print_success("从 string 解压成功");
            } else {
                print_error("从 string 解压失败");
            }
        }

        // 测试从 istream 读取
        print_subsection("测试从 istream 读取");
        {
            auto stream = std::make_unique<std::istringstream>(w.get_data());
            tar::reader r3(std::move(stream));
            fs::create_directories(extract_dir / "from_stream");
            r3.extract_all(extract_dir / "from_stream");

            if (fs::exists(extract_dir / "from_stream" / "memory_file1.txt") &&
                fs::exists(extract_dir / "from_stream" / "memory_file2.txt")) {
                print_success("从 istream 解压成功");
            } else {
                print_error("从 istream 解压失败");
            }
        }

        // 测试从原始指针读取
        print_subsection("测试从原始指针读取");
        {
            auto data_vec = w.get_vector();
            tar::reader r4(data_vec.data(), data_vec.size());
            fs::create_directories(extract_dir / "from_raw_ptr");
            r4.extract_all(extract_dir / "from_raw_ptr");

            if (fs::exists(extract_dir / "from_raw_ptr" / "memory_file1.txt") &&
                fs::exists(extract_dir / "from_raw_ptr" / "memory_file2.txt")) {
                print_success("从原始指针解压成功");
            } else {
                print_error("从原始指针解压失败");
            }
        }

        // 测试列表功能
        print_subsection("测试内存流列表功能");
        {
            tar::reader r5(data);
            std::cout << "\n内存压缩包内容列表:\n";
            r5.list();
        }

    } catch (const std::exception& e) {
        print_error(fmt::format("错误: {}", e.what()));
    }

    print_success("内存流测试完成");
}

// 测试 reader 类的 set_source 方法
void test_reader_set_source() {
    print_section("测试 reader 的 set_source 方法");

    const fs::path test_dir = "test_reader_set_source";

    // 清理之前的测试目录
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);

    // 创建测试文件
    const fs::path test_file = test_dir / "test.txt";
    create_test_file(test_file, "set_source 测试内容");

    try {
        // 创建内存中的 tar 数据
        print_subsection("创建内存中的 tar 数据");
        tar::writer w;
        w.add_file(test_file, "test_in_tar.txt");
        auto data = w.get_vector();

        // 测试默认构造函数和 set_source
        print_subsection("测试默认构造函数和 set_source");
        tar::reader r;
        assert(!r.is_open());
        print_success("默认构造函数创建未打开的 reader");

        r.set_source(data);
        assert(r.is_open());
        print_success("set_source 成功打开 reader");

        // 测试移动语义
        print_subsection("测试移动语义");
        tar::reader r2 = std::move(r);
        assert(r2.is_open());
        assert(!r.is_open());
        print_success("移动构造函数工作正常");

        tar::reader r3;
        r3 = std::move(r2);
        assert(r3.is_open());
        assert(!r2.is_open());
        print_success("移动赋值运算符工作正常");

        // 测试 close 方法
        print_subsection("测试 close 方法");
        r3.close();
        assert(!r3.is_open());
        print_success("close 方法工作正常");

        // 测试从不同源切换
        print_subsection("测试从不同源切换");
        r3.set_source(data);
        r3.set_source(w.get_data());  // 切换到另一个源
        assert(r3.is_open());
        print_success("成功切换数据源");

    } catch (const std::exception& e) {
        print_error(fmt::format("错误: {}", e.what()));
    }

    print_success("reader set_source 测试完成");
}

// 测试便捷函数的内存版本
void test_memory_convenience_functions() {
    print_section("测试内存便捷函数");

    const fs::path test_dir = "test_memory_convenience";

    // 清理之前的测试目录
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);

    // 创建测试文件
    const fs::path test_file = test_dir / "conv_test.txt";
    create_test_file(test_file, "便捷函数测试内容");

    try {
        // 创建内存中的 tar 数据
        print_subsection("创建内存中的 tar 数据");
        tar::writer w;
        w.add_file(test_file, "conv_file.txt");
        auto data = w.get_vector();
        auto str_data = w.get_data();

        // 测试从内存提取
        print_subsection("测试从内存提取");
        fs::create_directories(test_dir / "extracted1");
        tar::extract_archive_from_memory(data, test_dir / "extracted1");

        if (fs::exists(test_dir / "extracted1" / "conv_file.txt")) {
            print_success("从 vector<char> 提取成功");
        } else {
            print_error("从 vector<char> 提取失败");
        }

        fs::create_directories(test_dir / "extracted2");
        tar::extract_archive_from_memory(str_data, test_dir / "extracted2");

        if (fs::exists(test_dir / "extracted2" / "conv_file.txt")) {
            print_success("从 string 提取成功");
        } else {
            print_error("从 string 提取失败");
        }

        // 测试从内存列表
        print_subsection("测试从内存列表");
        std::cout << "\nvector<char> 压缩包内容:\n";
        tar::list_archive_from_memory(data);

        std::cout << "\nstring 压缩包内容:\n";
        tar::list_archive_from_memory(str_data);

    } catch (const std::exception& e) {
        print_error(fmt::format("错误: {}", e.what()));
    }

    print_success("内存便捷函数测试完成");
}

// 测试单个文件打包（内存流版本）
void test_single_file() {
    print_section("测试单个文件打包（内存流）");

    const fs::path test_dir = "test_single_file";
    const fs::path archive_path = test_dir / "test.tar";
    const fs::path extract_dir = test_dir / "extracted";

    // 清理之前的测试目录
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);

    // 创建测试文件
    const fs::path test_file = test_dir / "hello.txt";
    create_test_file(test_file, "Hello, Tar Archive!");

    try {
        // 打包文件到内存
        print_subsection("打包单个文件到内存");
        tar::writer w;
        w.add_file(test_file);

        // 在 finish 之前获取数据大小
        auto data_size_before_finish = w.size();
        print_info(fmt::format("finish 前的数据大小: {} bytes",
                               data_size_before_finish));

        // 保存到文件
        print_subsection("将内存数据写入文件");
        w.write_to_file(archive_path);

        // 在 finish 之后获取数据大小
        auto data_size_after_finish = w.size();
        print_info(fmt::format("finish 后的数据大小: {} bytes",
                               data_size_after_finish));

        // 列出压缩包内容
        print_subsection("列出压缩包内容");
        tar::list_archive(archive_path);

        // 从内存读取并解压
        print_subsection("从内存读取并解压");
        fs::create_directories(extract_dir / "from_memory");
        auto data = w.get_data();
        tar::extract_archive_from_memory(data, extract_dir / "from_memory");

        // 从文件读取并解压
        print_subsection("从文件读取并解压");
        fs::create_directories(extract_dir / "from_file");
        tar::extract_archive(archive_path, extract_dir / "from_file");

        // 验证文件
        print_subsection("验证文件");
        const fs::path extracted_memory_file =
            extract_dir / "from_memory" / "hello.txt";
        const fs::path extracted_file_file =
            extract_dir / "from_file" / "hello.txt";

        bool memory_ok =
            fs::exists(extracted_memory_file) &&
            verify_file_content(extracted_memory_file, "Hello, Tar Archive!");
        bool file_ok =
            fs::exists(extracted_file_file) &&
            verify_file_content(extracted_file_file, "Hello, Tar Archive!");

        if (memory_ok && file_ok) {
            print_success("文件验证通过（内存和文件）");
        } else {
            if (!memory_ok) print_error("内存解压验证失败");
            if (!file_ok) print_error("文件解压验证失败");
        }

        // 测试获取不同格式的数据
        print_subsection("测试数据获取接口");
        std::string str_data = w.get_data();
        std::vector<char> vec_data = w.get_vector();

        if (str_data.size() == data_size_after_finish &&
            vec_data.size() == data_size_after_finish) {
            print_success("数据获取接口测试通过");
        } else {
            print_error(fmt::format(
                "数据大小不匹配: str={}, vec={}, expected={}", str_data.size(),
                vec_data.size(), data_size_after_finish));
        }

    } catch (const std::exception& e) {
        print_error(fmt::format("错误: {}", e.what()));
    }

    print_success("单个文件测试完成");
}

// 测试多个文件打包（内存流版本）
void test_multiple_files() {
    print_section("测试多个文件打包（内存流）");

    const fs::path test_dir = "test_multiple_files";
    const fs::path archive_path = test_dir / "test.tar";
    const fs::path extract_dir = test_dir / "extracted";

    // 清理之前的测试目录
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);

    // 创建多个测试文件
    std::vector<fs::path> test_files = {
        test_dir / "file1.txt", test_dir / "file2.txt", test_dir / "file3.txt"};

    create_test_file(test_files[0], "这是文件1的内容");
    create_test_file(test_files[1], "这是文件2的内容");
    create_test_file(test_files[2],
                     "这是文件3的内容，稍微长一点的内容用于测试");

    try {
        // 打包多个文件到内存
        print_subsection("打包多个文件到内存");
        tar::writer w;
        for (const auto& file : test_files) {
            w.add_file(file);
        }

        // 获取数据并验证大小
        auto data_size = w.size();
        print_info(fmt::format("内存中的数据大小: {} bytes", data_size));

        // 保存到文件
        print_subsection("将内存数据写入文件");
        w.write_to_file(archive_path);

        // 重新获取 finish 后的数据大小
        data_size = w.size();
        print_info(fmt::format("finish 后的数据大小: {} bytes", data_size));

        // 列出压缩包内容
        print_subsection("列出压缩包内容");
        tar::list_archive(archive_path);

        // 从内存解压
        print_subsection("从内存解压");
        fs::create_directories(extract_dir / "from_memory");
        auto data = w.get_data();
        tar::extract_archive_from_memory(data, extract_dir / "from_memory");

        // 从文件解压
        print_subsection("从文件解压");
        fs::create_directories(extract_dir / "from_file");
        tar::extract_archive(archive_path, extract_dir / "from_file");

        // 验证文件
        print_subsection("验证文件");
        bool all_memory_ok = true;
        bool all_file_ok = true;

        for (size_t i = 0; i < test_files.size(); ++i) {
            const fs::path extracted_memory_file =
                extract_dir / "from_memory" / test_files[i].filename();
            const fs::path extracted_file_file =
                extract_dir / "from_file" / test_files[i].filename();

            if (!fs::exists(extracted_memory_file)) {
                print_error(fmt::format("内存解压文件{}不存在", i + 1));
                all_memory_ok = false;
            }
            if (!fs::exists(extracted_file_file)) {
                print_error(fmt::format("文件解压文件{}不存在", i + 1));
                all_file_ok = false;
            }
        }

        if (all_memory_ok && all_file_ok) {
            print_success("所有文件验证通过（内存和文件）");
        }

    } catch (const std::exception& e) {
        print_error(fmt::format("错误: {}", e.what()));
    }

    print_success("多个文件测试完成");
}

// 测试目录打包（内存流版本）
void test_directory() {
    print_section("测试目录打包（内存流）");

    const fs::path test_dir = "test_directory";
    const fs::path archive_path = test_dir / "test.tar";
    const fs::path extract_dir = test_dir / "extracted";

    // 创建测试目录结构
    const fs::path source_dir = test_dir / "source";

    // 清理之前的测试目录
    fs::remove_all(test_dir);

    // 创建复杂的目录结构
    fs::create_directories(source_dir / "subdir1");
    fs::create_directories(source_dir / "subdir2" / "deep");

    // 创建多个测试文件
    create_test_file(source_dir / "root.txt", "根目录文件");
    create_test_file(source_dir / "subdir1" / "file1.txt", "子目录1的文件");
    create_test_file(source_dir / "subdir2" / "deep" / "deepfile.txt",
                     "深层文件");

    // 创建空目录
    fs::create_directories(source_dir / "empty_dir");

    try {
        // 打包整个目录到内存
        print_subsection("打包整个目录到内存");
        tar::writer w;
        std::string name = "mydir";
        w.add_directory(source_dir, name);

        // 获取数据大小
        auto data_size = w.size();
        print_info(fmt::format("finish 前的数据大小: {} bytes", data_size));

        // 保存到文件
        print_subsection("将内存数据写入文件");
        w.write_to_file(archive_path);

        // 重新获取 finish 后的数据大小
        data_size = w.size();
        print_info(fmt::format("finish 后的数据大小: {} bytes", data_size));

        // 列出压缩包内容
        print_subsection("列出压缩包内容");
        tar::list_archive(archive_path);

        // 从内存解压
        print_subsection("从内存解压");
        fs::create_directories(extract_dir / "from_memory");
        auto data = w.get_vector();
        tar::extract_archive_from_memory(data, extract_dir / "from_memory");

        // 从文件解压
        print_subsection("从文件解压");
        fs::create_directories(extract_dir / "from_file");
        tar::extract_archive(archive_path, extract_dir / "from_file");

        // 验证目录结构
        print_subsection("验证目录结构");
        bool memory_ok =
            fs::exists(extract_dir / "from_memory" / "mydir") &&
            fs::exists(extract_dir / "from_memory" / "mydir" / "root.txt") &&
            fs::exists(extract_dir / "from_memory" / "mydir" / "subdir1" /
                       "file1.txt") &&
            fs::exists(extract_dir / "from_memory" / "mydir" / "subdir2" /
                       "deep" / "deepfile.txt") &&
            fs::exists(extract_dir / "from_memory" / "mydir" / "empty_dir");

        bool file_ok =
            fs::exists(extract_dir / "from_file" / "mydir") &&
            fs::exists(extract_dir / "from_file" / "mydir" / "root.txt") &&
            fs::exists(extract_dir / "from_file" / "mydir" / "subdir1" /
                       "file1.txt") &&
            fs::exists(extract_dir / "from_file" / "mydir" / "subdir2" /
                       "deep" / "deepfile.txt") &&
            fs::exists(extract_dir / "from_file" / "mydir" / "empty_dir");

        if (memory_ok && file_ok) {
            print_success("目录结构验证通过（内存和文件）");
        } else {
            if (!memory_ok) print_error("内存解压目录结构验证失败");
            if (!file_ok) print_error("文件解压目录结构验证失败");
        }

    } catch (const std::exception& e) {
        print_error(fmt::format("错误: {}", e.what()));
    }

    print_success("目录测试完成");
}

// 测试大文件（内存流版本）
void test_large_file() {
    print_section("测试大文件打包（内存流）");

    const fs::path test_dir = "test_large_file";
    const fs::path archive_path = test_dir / "large.tar";
    const fs::path extract_dir = test_dir / "extracted";

    // 清理之前的测试目录
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);

    // 创建大文件 (1MB)
    const fs::path large_file = test_dir / "large.bin";
    print_subsection("创建1MB测试文件");

    {
        std::ofstream file(large_file, std::ios::binary);
        std::mt19937 rng(12345);
        std::uniform_int_distribution<unsigned char> dist(0, 255);

        const size_t size = 1024 * 1024;  // 1MB
        std::vector<char> buffer(4096);

        for (size_t written = 0; written < size; written += buffer.size()) {
            for (auto& c : buffer) {
                c = static_cast<char>(dist(rng));
            }
            file.write(buffer.data(), std::min(buffer.size(), size - written));
        }
    }

    try {
        // 打包大文件到内存
        print_subsection("打包大文件到内存");
        tar::writer w;
        w.add_file(large_file);

        // 获取数据大小
        auto data_size = w.size();
        print_info(fmt::format("finish 前的数据大小: {} bytes", data_size));

        // 保存到文件
        print_subsection("将内存数据写入文件");
        w.write_to_file(archive_path);

        // 重新获取 finish 后的数据大小
        data_size = w.size();
        print_info(fmt::format("finish 后的数据大小: {} bytes", data_size));

        // 从内存解压
        print_subsection("从内存解压大文件");
        fs::create_directories(extract_dir / "from_memory");
        auto data = w.get_data();
        tar::extract_archive_from_memory(data, extract_dir / "from_memory");

        // 从文件解压
        print_subsection("从文件解压大文件");
        fs::create_directories(extract_dir / "from_file");
        tar::extract_archive(archive_path, extract_dir / "from_file");

        // 验证文件大小
        print_subsection("验证文件大小");
        const fs::path extracted_memory_file =
            extract_dir / "from_memory" / "large.bin";
        const fs::path extracted_file_file =
            extract_dir / "from_file" / "large.bin";

        auto original_size = fs::file_size(large_file);
        auto memory_size = fs::file_size(extracted_memory_file);
        auto file_size = fs::file_size(extracted_file_file);

        bool memory_ok = original_size == memory_size;
        bool file_ok = original_size == file_size;

        if (memory_ok && file_ok) {
            print_success(fmt::format(
                "文件大小验证通过: 原始={}, 内存解压={}, 文件解压={}",
                original_size, memory_size, file_size));
        } else {
            if (!memory_ok)
                print_error(fmt::format("内存解压大小不匹配: 原始={}, 解压={}",
                                        original_size, memory_size));
            if (!file_ok)
                print_error(fmt::format("文件解压大小不匹配: 原始={}, 解压={}",
                                        original_size, file_size));
        }

    } catch (const std::exception& e) {
        print_error(fmt::format("错误: {}", e.what()));
    }

    print_success("大文件测试完成");
}

// 测试错误处理（内存流版本）
void test_error_handling() {
    print_section("测试错误处理（内存流）");

    try {
        // 测试不存在的文件
        print_subsection("测试打包不存在的文件");
        try {
            tar::writer w;
            w.add_file("this_file_does_not_exist.txt");
            print_error("应该抛出异常但没有！");
        } catch (const std::exception& e) {
            print_success(fmt::format("预期异常: {}", e.what()));
        }

        // 测试不存在的目录
        print_subsection("测试打包不存在的目录");
        try {
            tar::writer w;
            w.add_directory("this_dir_does_not_exist");
            print_error("应该抛出异常但没有！");
        } catch (const std::exception& e) {
            print_success(fmt::format("预期异常: {}", e.what()));
        }

        // 测试写入不存在的目录
        print_subsection("测试写入不存在的目录");
        try {
            tar::writer w;
            // 先添加一个文件
            const fs::path temp_file = "temp_test_file.txt";
            create_test_file(temp_file, "test");
            w.add_file(temp_file);
            fs::remove(temp_file);

            w.write_to_file("nonexistent/path/test.tar");
            print_error("应该抛出异常但没有！");
        } catch (const std::exception& e) {
            print_success(fmt::format("预期异常: {}", e.what()));
        }

        // 修正：明确指定路径构造函数
        print_subsection("测试读取不存在的压缩包");
        try {
            tar::reader r(fs::path("this_archive_does_not_exist.tar"));
            r.list();
            print_error("应该抛出异常但没有！");
        } catch (const std::exception& e) {
            print_success(fmt::format("预期异常: {}", e.what()));
        }

        // 测试从无效的内存数据读取
        print_subsection("测试从无效的内存数据读取");
        try {
            std::vector<char> invalid_data = {0, 1, 2, 3, 4, 5};
            tar::reader r(invalid_data);
            r.extract_all("test");
            print_error("应该抛出异常但没有！");
        } catch (const std::exception& e) {
            print_success(fmt::format("预期异常: {}", e.what()));
        }

        // 测试空的 reader
        print_subsection("测试空的 reader");
        try {
            tar::reader r;
            r.extract_all("test");
            print_error("应该抛出异常但没有！");
        } catch (const std::exception& e) {
            print_success(fmt::format("预期异常: {}", e.what()));
        }

    } catch (...) {
        print_error("意外错误！");
    }

    print_success("错误处理测试完成");
}

// 测试 writer 类的所有方法（内存流版本）
void test_writer_class() {
    print_section("测试 writer 类（内存流）");

    const fs::path test_dir = "test_writer_class";
    const fs::path archive_path = test_dir / "writer_test.tar";

    // 清理之前的测试目录
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);

    // 创建测试文件和目录
    create_test_file(test_dir / "test1.txt", "测试文件1");
    create_test_file(test_dir / "test2.txt", "测试文件2");
    fs::create_directories(test_dir / "test_dir");
    create_test_file(test_dir / "test_dir" / "nested.txt", "嵌套文件");

    try {
        // 测试手动添加文件
        print_subsection("测试手动添加文件");
        tar::writer w;
        w.add_file(test_dir / "test1.txt", "custom_name.txt");
        w.add_directory(test_dir / "test_dir", "custom_dir");

        // 测试数据获取
        print_subsection("测试数据获取");
        auto data_size = w.size();
        print_info(fmt::format("数据大小: {} bytes", data_size));
        assert(!w.empty());

        // 测试 clear 方法
        print_subsection("测试 clear 方法");
        w.clear();
        assert(w.empty());
        assert(w.size() == 0);
        print_info(fmt::format("clear 后数据大小: {} bytes", w.size()));

        // 重新添加数据
        w.add_file(test_dir / "test2.txt");
        assert(!w.empty());
        assert(w.size() > 0);

        // 验证压缩包
        print_subsection("验证压缩包内容");
        w.write_to_file(archive_path);
        assert(fs::exists(archive_path));

        // 测试 finish 后的数据大小
        auto finished_size = w.size();
        print_info(fmt::format("finish 后数据大小: {} bytes", finished_size));

        // 从内存和文件读取并比较
        print_subsection("比较内存和文件读取结果");
        auto memory_data = w.get_data();
        tar::reader r_memory(memory_data);
        tar::reader r_file(archive_path);

        // 测试内存和文件读取的一致性
        std::cout << "\n内存数据内容:\n";
        r_memory.list();

        std::cout << "\n文件数据内容:\n";
        r_file.list();

        print_success("writer 类测试完成");

    } catch (const std::exception& e) {
        print_error(fmt::format("错误: {}", e.what()));
    }

    print_success("writer 类测试完成");
}

// 测试便捷函数
void test_convenience_functions() {
    print_section("测试便捷函数");

    const fs::path test_dir = "test_convenience";
    const fs::path archive1 = test_dir / "archive1.tar";
    const fs::path archive2 = test_dir / "archive2.tar";
    const fs::path source_dir = test_dir / "source";

    // 清理之前的测试目录
    fs::remove_all(test_dir);
    fs::create_directories(test_dir);
    fs::create_directories(source_dir);

    // 创建测试文件
    create_test_file(source_dir / "file1.txt", "文件1");
    create_test_file(source_dir / "file2.txt", "文件2");

    try {
        // 测试 create_archive
        print_subsection("测试 create_archive");
        std::vector<fs::path> files = {source_dir / "file1.txt",
                                       source_dir / "file2.txt"};
        tar::create_archive(archive1, files);

        if (fs::exists(archive1)) {
            print_success(
                fmt::format("create_archive 成功: {}", archive1.string()));
            tar::list_archive(archive1);
        } else {
            print_error("create_archive 失败");
        }

        // 测试 create_archive_from_directory
        print_subsection("测试 create_archive_from_directory");
        tar::create_archive_from_directory(archive2, source_dir, "mydir");

        if (fs::exists(archive2)) {
            print_success(fmt::format("create_archive_from_directory 成功: {}",
                                      archive2.string()));
            tar::list_archive(archive2);
        } else {
            print_error("create_archive_from_directory 失败");
        }

        // 测试 extract_archive
        print_subsection("测试 extract_archive");
        const fs::path extract_dir = test_dir / "extracted";
        tar::extract_archive(archive1, extract_dir);

        if (fs::exists(extract_dir / "file1.txt") &&
            fs::exists(extract_dir / "file2.txt")) {
            print_success("extract_archive 成功");
        } else {
            print_error("extract_archive 失败");
        }

        // 测试 list_archive
        print_subsection("测试 list_archive");
        tar::list_archive(archive2);

    } catch (const std::exception& e) {
        print_error(fmt::format("错误: {}", e.what()));
    }

    print_success("便捷函数测试完成");
}

// 主测试函数，调用所有测试
void run_all_tests() {
    fmt::print(fg(fmt::color::light_green) | fmt::emphasis::bold, "\n{:*^50}\n",
               " 开始 Tar 库测试（内存流版本）");
    fmt::print("\n");

    try {
        // 测试1: 工具函数
        test_utils();

        // 测试2: 单个文件打包解压
        test_single_file();

        // 测试3: 多个文件打包解压
        test_multiple_files();

        // 测试4: 目录打包解压
        test_directory();

        // 测试5: 大文件处理
        test_large_file();

        // 测试6: 错误处理
        test_error_handling();

        // 测试7: writer 类
        test_writer_class();

        // 测试8: 便捷函数
        test_convenience_functions();

        // 测试9: 内存流读取功能
        test_memory_stream();

        // 测试10: reader 的 set_source 方法
        test_reader_set_source();

        // 测试11: 内存便捷函数
        test_memory_convenience_functions();

        fmt::print("\n");
        fmt::print(fg(fmt::color::light_green) | fmt::emphasis::bold,
                   "{:*^50}\n", " 所有测试完成 ");
        fmt::print(fg(fmt::color::green) | fmt::emphasis::bold,
                   "\n🎉 恭喜！所有测试都通过了！\n");

    } catch (const std::exception& e) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::bold,
                   "\n❌ 测试失败: {}\n", e.what());
        std::exit(1);
    } catch (...) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::bold,
                   "\n❌ 未知错误！\n");
        std::exit(1);
    }
}

// 清理函数
void cleanup() {
    fmt::print("\n");
    print_section("清理测试文件");

    std::vector<std::string> test_dirs = {
        "test_single_file",       "test_multiple_files",
        "test_directory",         "test_large_file",
        "test_writer_class",      "test_convenience",
        "test_memory_stream",     "test_reader_set_source",
        "test_memory_convenience"};

    int removed_count = 0;
    for (const auto& dir : test_dirs) {
        try {
            if (fs::exists(dir)) {
                fs::remove_all(dir);
                print_info(fmt::format("删除目录: {}", dir));
                removed_count++;
            }
        } catch (...) {
            print_warning(fmt::format("无法删除目录: {}", dir));
        }
    }

    // 清理临时文件
    try {
        if (fs::exists("temp_test_file.txt")) {
            fs::remove("temp_test_file.txt");
        }
    } catch (...) {
    }

    if (removed_count > 0) {
        print_success(fmt::format("清理完成，删除了 {} 个目录", removed_count));
    } else {
        print_info("没有需要清理的目录");
    }
}

// 主函数
int test_main() {
    fmt::print(fg(fmt::color::light_blue) | fmt::emphasis::bold, "{:*^50}\n",
               " Tar 库测试程序（内存流版本）");
    fmt::print("\n");

    // 运行所有测试
    run_all_tests();

    // 清理测试文件
    cleanup();

    fmt::print("\n");
    fmt::print(fg(fmt::color::light_gray), "测试程序结束\n");

    return 0;
}

}  // namespace tar_tests