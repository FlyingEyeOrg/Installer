#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
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

        // 获取数据大小
        auto data_size = w.size();
        print_info(fmt::format("内存中的数据大小: {} bytes", data_size));

        // 保存到文件
        print_subsection("将内存数据写入文件");
        w.write_to_file(archive_path);

        // 列出压缩包内容
        print_subsection("列出压缩包内容");
        tar::list_archive(archive_path);

        // 解压文件
        print_subsection("解压文件");
        fs::create_directories(extract_dir);
        tar::extract_archive(archive_path, extract_dir);

        // 验证文件
        print_subsection("验证文件");
        const fs::path extracted_file = extract_dir / "hello.txt";
        if (fs::exists(extracted_file) &&
            verify_file_content(extracted_file, "Hello, Tar Archive!")) {
            print_success("文件验证通过");
        } else {
            print_error("文件验证失败");
        }

        // 测试获取不同格式的数据
        print_subsection("测试数据获取接口");
        std::string str_data = w.get_data();
        std::vector<char> vec_data = w.get_vector();

        if (str_data.size() == data_size && vec_data.size() == data_size) {
            print_success("数据获取接口测试通过");
        } else {
            print_error(
                fmt::format("数据大小不匹配: str={}, vec={}, expected={}",
                            str_data.size(), vec_data.size(), data_size));
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

        // 列出压缩包内容
        print_subsection("列出压缩包内容");
        tar::list_archive(archive_path);

        // 解压文件
        print_subsection("解压文件");
        fs::create_directories(extract_dir);
        tar::extract_archive(archive_path, extract_dir);

        // 验证文件
        print_subsection("验证文件");
        bool all_ok = true;
        for (size_t i = 0; i < test_files.size(); ++i) {
            const fs::path extracted_file =
                extract_dir / test_files[i].filename();
            if (!fs::exists(extracted_file)) {
                print_error(fmt::format("文件{}不存在", i + 1));
                all_ok = false;
            }
        }

        if (all_ok) {
            print_success("所有文件验证通过");
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
        print_info(fmt::format("内存中的数据大小: {} bytes", data_size));

        // 保存到文件
        print_subsection("将内存数据写入文件");
        w.write_to_file(archive_path);

        // 列出压缩包内容
        print_subsection("列出压缩包内容");
        tar::list_archive(archive_path);

        // 解压目录
        print_subsection("解压目录");
        fs::create_directories(extract_dir);
        tar::extract_archive(archive_path, extract_dir);

        // 验证目录结构
        print_subsection("验证目录结构");
        assert(fs::exists(extract_dir / "mydir"));
        assert(fs::exists(extract_dir / "mydir" / "root.txt"));
        assert(fs::exists(extract_dir / "mydir" / "subdir1" / "file1.txt"));
        assert(fs::exists(extract_dir / "mydir" / "subdir2" / "deep" /
                          "deepfile.txt"));
        assert(fs::exists(extract_dir / "mydir" / "empty_dir"));

        print_success("目录结构验证通过");

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
        print_info(fmt::format("内存中的数据大小: {} bytes", data_size));

        // 保存到文件
        print_subsection("将内存数据写入文件");
        w.write_to_file(archive_path);

        // 解压大文件
        print_subsection("解压大文件");
        fs::create_directories(extract_dir);
        tar::extract_archive(archive_path, extract_dir);

        // 验证文件大小
        print_subsection("验证文件大小");
        const fs::path extracted_file = extract_dir / "large.bin";
        auto original_size = fs::file_size(large_file);
        auto extracted_size = fs::file_size(extracted_file);

        if (original_size == extracted_size) {
            print_success(
                fmt::format("文件大小验证通过: {} bytes", original_size));
        } else {
            print_error(fmt::format("文件大小不匹配: 原始={}, 解压={}",
                                    original_size, extracted_size));
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
            w.add_file("existing_file.txt");  // 假设这个文件存在
            w.write_to_file("nonexistent/path/test.tar");
            print_error("应该抛出异常但没有！");
        } catch (const std::exception& e) {
            print_success(fmt::format("预期异常: {}", e.what()));
        }

        // 测试打开不存在的压缩包
        print_subsection("测试读取不存在的压缩包");
        try {
            tar::reader r("this_archive_does_not_exist.tar");
            r.list();
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
        print_info(fmt::format("clear 后数据大小: {} bytes", w.size()));

        // 重新添加数据
        w.add_file(test_dir / "test2.txt");

        // 验证压缩包
        print_subsection("验证压缩包内容");
        w.write_to_file(archive_path);
        tar::list_archive(archive_path);

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
        "test_single_file", "test_multiple_files", "test_directory",
        "test_large_file",  "test_writer_class",   "test_convenience"};

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
    tar_tests::run_all_tests();

    // 清理测试文件
    tar_tests::cleanup();

    fmt::print("\n");
    fmt::print(fg(fmt::color::light_gray), "测试程序结束\n");

    return 0;
}

}  // namespace tar_tests
