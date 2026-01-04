#pragma once

#include <fmt/color.h>
#include <fmt/core.h>

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

#include "memory_stream/memory_stream.hpp"


class MemoryStreamTest {
   public:
    // 运行所有测试
    static void run_all_tests() {
        fmt::print(fg(fmt::color::light_blue) | fmt::emphasis::bold,
                   "=== 开始 Memory Stream 测试 ===\n\n");

        int total = 0;
        int passed = 0;

        auto run_test = [&](const std::string& name, auto test_func) {
            total++;
            print_test(name);
            try {
                test_func();
                print_passed();
                passed++;
            } catch (const std::exception& e) {
                print_failed(fmt::format("异常: {}", e.what()));
            } catch (...) {
                print_failed("未知异常");
            }
        };

        // 运行所有测试用例
        run_test("默认构造", test_default_construction);
        run_test("自定义构造", test_custom_construction);
        run_test("移动语义", test_move_semantics);
        run_test("基本写入操作", test_write_basic);
        run_test("写入字节操作", test_write_bytes);
        run_test("填充操作", test_fill);
        run_test("基本读取操作", test_read_basic);
        run_test("读取字节操作", test_read_byte);
        run_test("查看操作", test_peek);
        run_test("定位/获取/重置操作", test_seek_tell_rewind);
        run_test("容量查询", test_capacity_queries);
        run_test("元素访问", test_element_access);
        run_test("查找操作", test_find_operations);
        run_test("拷贝操作", test_copy_operations);
        run_test("跳过操作", test_skip_operations);
        run_test("边界情况", test_edge_cases);
        run_test("大数据操作", test_large_data);
        run_test("混合操作", test_mixed_operations);

        fmt::print("\n");
        fmt::print(fg(fmt::color::light_blue) | fmt::emphasis::bold,
                   "=== 所有测试完成 ===\n");

        if (passed == total) {
            fmt::print(fg(fmt::color::green) | fmt::emphasis::bold,
                       "✓ 所有测试通过! ({}/{})\n", passed, total);
        } else {
            fmt::print(fg(fmt::color::red) | fmt::emphasis::bold,
                       "✗ 测试失败! 通过: {}/{}\n", passed, total);
        }
    }

   private:
    // 测试工具函数
    static bool compare_vectors(const std::vector<unsigned char>& v1,
                                const std::vector<unsigned char>& v2) {
        if (v1.size() != v2.size()) return false;
        for (size_t i = 0; i < v1.size(); ++i) {
            if (v1[i] != v2[i]) return false;
        }
        return true;
    }

    static std::string vector_to_string(const std::vector<unsigned char>& vec) {
        std::ostringstream oss;
        for (size_t i = 0; i < vec.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << static_cast<int>(vec[i]);
        }
        return oss.str();
    }

    static void print_test(const std::string& test_name) {
        fmt::print("测试: {:<30} ... ", test_name);
    }

    static void print_passed() {
        fmt::print(fg(fmt::color::green), "✓ 通过\n");
    }

    static void print_failed(const std::string& msg = "") {
        fmt::print(fg(fmt::color::red), "✗ 失败");
        if (!msg.empty()) {
            fmt::print(" - {}", msg);
        }
        fmt::print("\n");
    }

    // 测试用例
    static void test_default_construction() {
        memory_stream stream;

        assert(stream.size() == 0);
        assert(stream.empty());
        assert(stream.tell() == 0);
        assert(stream.chunk_capacity() ==
               static_cast<size_t>(block_sizes::DefaultChunkSize));
        assert(stream.chunk_count() == 0);
        assert(!stream.can_read());
        assert(stream.readable_bytes() == 0);
        assert(stream.eof());
    }

    static void test_custom_construction() {
        // 使用 enum 构造
        memory_stream stream1(block_sizes::KB_2);
        assert(stream1.chunk_capacity() == 2048);

        // 使用 size_t 构造
        memory_stream stream2(4096);
        assert(stream2.chunk_capacity() == 4096);
    }

    static void test_move_semantics() {
        // 创建原始流
        memory_stream original(block_sizes::KB_1);
        std::vector<unsigned char> data = {1, 2, 3, 4, 5};
        original.write(data.data(), data.size());
        original.read_byte();  // 移动读取位置

        // 保存状态
        size_t original_size = original.size();
        size_t original_tell = original.tell();

        // 移动构造
        memory_stream moved(std::move(original));
        assert(moved.size() == original_size);
        assert(moved.tell() == original_tell);
        assert(moved.chunk_capacity() == 1024);

        // 移动后原始对象应为空
        assert(original.size() == 0);
        assert(original.empty());

        // 移动赋值
        memory_stream another(block_sizes::KB_2);
        another = std::move(moved);
        assert(another.size() == original_size);
    }

    static void test_write_basic() {
        memory_stream stream(block_sizes::Bytes_128);

        // 写入少量数据
        std::vector<unsigned char> data1 = {1, 2, 3, 4, 5};
        size_t written = stream.write(data1.data(), data1.size());
        assert(written == 5);
        assert(stream.size() == 5);
        assert(!stream.empty());

        // 写入更多数据
        std::vector<unsigned char> data2(100, 0xFF);
        written = stream.write(data2.data(), data2.size());
        assert(written == 100);
        assert(stream.size() == 105);
        assert(stream.chunk_count() > 0);

        // 写入零字节
        written = stream.write(nullptr, 0);
        assert(written == 0);
        written = stream.write(data2.data(), 0);
        assert(written == 0);
    }

    static void test_write_bytes() {
        memory_stream stream(block_sizes::Bytes_64);

        // 写入单个字节
        assert(stream.write_byte(0xAA));
        assert(stream.size() == 1);
        assert(stream.tell() == 0);

        // 写入多个字节
        for (int i = 0; i < 10; ++i) {
            assert(stream.write_byte(static_cast<unsigned char>(i)));
        }
        assert(stream.size() == 11);
    }

    static void test_fill() {
        memory_stream stream(block_sizes::Bytes_64);

        // 填充少量数据
        size_t filled = stream.fill(0xCC, 10);
        assert(filled == 10);
        assert(stream.size() == 10);

        // 检查填充的内容
        for (size_t i = 0; i < 10; ++i) {
            auto byte = stream.peek_byte(i);
            assert(byte && *byte == 0xCC);
        }

        // 填充大量数据（跨chunk）
        filled = stream.fill(0xDD, 100);
        assert(filled == 100);
        assert(stream.size() == 110);
        assert(stream.chunk_count() > 1);
    }

    static void test_read_basic() {
        memory_stream stream(block_sizes::Bytes_64);

        // 准备数据
        std::vector<unsigned char> original(50);
        for (size_t i = 0; i < original.size(); ++i) {
            original[i] = static_cast<unsigned char>(i);
        }
        stream.write(original.data(), original.size());

        // 读取部分数据
        unsigned char buffer1[20];
        size_t read = stream.read(buffer1, 20);
        assert(read == 20);
        assert(stream.tell() == 20);

        // 验证读取的数据
        for (size_t i = 0; i < 20; ++i) {
            assert(buffer1[i] == original[i]);
        }

        // 读取剩余数据
        unsigned char buffer2[30];
        read = stream.read(buffer2, 30);
        assert(read == 30);
        assert(stream.tell() == 50);
        assert(stream.eof());

        // 读取更多（应该返回0）
        read = stream.read(buffer2, 10);
        assert(read == 0);
        assert(stream.tell() == 50);
    }

    static void test_read_byte() {
        memory_stream stream(block_sizes::Bytes_64);

        // 准备数据
        for (int i = 0; i < 10; ++i) {
            stream.write_byte(static_cast<unsigned char>(i));
        }

        // 逐个字节读取
        for (int i = 0; i < 10; ++i) {
            auto byte = stream.read_byte();
            assert(byte && *byte == static_cast<unsigned char>(i));
            assert(stream.tell() == static_cast<size_t>(i + 1));
        }

        // 读取结束
        auto byte = stream.read_byte();
        assert(!byte);
        assert(stream.eof());
    }

    static void test_peek() {
        memory_stream stream(block_sizes::Bytes_128);

        // 准备数据
        std::vector<unsigned char> data(50);
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = static_cast<unsigned char>(i + 1);
        }
        stream.write(data.data(), data.size());

        // 从不同位置peek
        unsigned char buffer[10];

        // peek开头
        size_t peeked = stream.peek(0, buffer, 10);
        assert(peeked == 10);
        for (size_t i = 0; i < 10; ++i) {
            assert(buffer[i] == data[i]);
        }
        assert(stream.tell() == 0);  // peek不移动位置

        // peek中间
        peeked = stream.peek(20, buffer, 10);
        assert(peeked == 10);
        for (size_t i = 0; i < 10; ++i) {
            assert(buffer[i] == data[20 + i]);
        }

        // peek单个字节
        auto byte = stream.peek_byte(25);
        assert(byte && *byte == data[25]);

        // peek超出范围
        byte = stream.peek_byte(100);
        assert(!byte);

        peeked = stream.peek(45, buffer, 10);
        assert(peeked == 5);  // 只有5个字节可读
    }

    static void test_seek_tell_rewind() {
        memory_stream stream(block_sizes::Bytes_64);

        // 准备数据
        std::vector<unsigned char> data(30);
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = static_cast<unsigned char>(i);
        }
        stream.write(data.data(), data.size());

        // 初始位置
        assert(stream.tell() == 0);

        // seek到不同位置
        assert(stream.seek(10));
        assert(stream.tell() == 10);

        // 读取验证
        unsigned char buffer[5];
        size_t read = stream.read(buffer, 5);
        assert(read == 5);
        for (size_t i = 0; i < 5; ++i) {
            assert(buffer[i] == data[10 + i]);
        }
        assert(stream.tell() == 15);

        // seek到开头
        assert(stream.seek(0));
        assert(stream.tell() == 0);

        // seek到末尾
        assert(stream.seek(30));
        assert(stream.tell() == 30);
        assert(stream.eof());

        // seek超出范围
        assert(!stream.seek(35));
        assert(stream.tell() == 30);  // 位置不变

        // rewind
        stream.rewind();
        assert(stream.tell() == 0);
        assert(!stream.eof());
    }

    static void test_capacity_queries() {
        memory_stream stream(block_sizes::Bytes_256);

        // 初始状态
        assert(stream.empty());
        assert(stream.size() == 0);
        assert(stream.chunk_capacity() == 256);
        assert(stream.chunk_count() == 0);
        assert(!stream.can_read());
        assert(stream.readable_bytes() == 0);
        assert(stream.eof());

        // 写入数据
        std::vector<unsigned char> data(300, 0xAA);  // 超过一个chunk
        stream.write(data.data(), data.size());

        // 写入后状态
        assert(!stream.empty());
        assert(stream.size() == 300);
        assert(stream.chunk_count() >= 2);  // 至少2个chunk
        assert(stream.can_read());
        assert(stream.readable_bytes() == 300);
        assert(!stream.eof());

        // 读取部分数据后
        stream.read_byte();
        assert(stream.size() == 300);  // 大小不变
        assert(stream.tell() == 1);
        assert(stream.readable_bytes() == 299);
        assert(stream.can_read());
    }

    static void test_element_access() {
        memory_stream stream(block_sizes::Bytes_128);

        // 准备数据
        std::vector<unsigned char> data = {10, 20, 30, 40, 50};
        stream.write(data.data(), data.size());

        // at() 方法
        auto byte1 = stream.at(0);
        assert(byte1 && *byte1 == 10);

        auto byte2 = stream.at(2);
        assert(byte2 && *byte2 == 30);

        auto byte3 = stream.at(4);
        assert(byte3 && *byte3 == 50);

        auto byte4 = stream.at(10);  // 超出范围
        assert(!byte4);

        // front() 和 back()
        auto front = stream.front();
        assert(front && *front == 10);

        auto back = stream.back();
        assert(back && *back == 50);

        // 清空后测试
        stream.clear();
        assert(!stream.front());
        assert(!stream.back());
        assert(!stream.at(0));
    }

    static void test_find_operations() {
        memory_stream stream(block_sizes::Bytes_128);

        // 准备数据: 1, 2, 3, 4, 5, 3, 6, 7, 3, 8
        std::vector<unsigned char> data = {1, 2, 3, 4, 5, 3, 6, 7, 3, 8};
        stream.write(data.data(), data.size());

        // 从开头查找
        auto pos1 = stream.find(3);
        assert(pos1 && *pos1 == 2);  // 第一个3在位置2

        // 从指定位置查找
        auto pos2 = stream.find(3, 3);
        assert(pos2 && *pos2 == 5);  // 从位置3开始，找到位置5的3

        // 从当前位置查找
        stream.seek(6);
        auto pos3 = stream.find_from_current(3);
        assert(pos3 && *pos3 == 8);  // 从位置6开始，找到位置8的3

        // 查找不存在的字节
        auto pos4 = stream.find(100);
        assert(!pos4);

        // 查找超出范围
        auto pos5 = stream.find(1, 20);
        assert(!pos5);
    }

    static void test_copy_operations() {
        memory_stream stream(block_sizes::Bytes_256);

        // 准备数据
        std::vector<unsigned char> original(100);
        for (size_t i = 0; i < original.size(); ++i) {
            original[i] = static_cast<unsigned char>(i % 256);
        }
        stream.write(original.data(), original.size());

        // 拷贝整个流到vector
        auto vec1 = stream.copy_to_vector();
        assert(vec1.size() == 100);
        assert(compare_vectors(vec1, original));

        // 从当前位置拷贝
        stream.seek(20);
        auto vec2 = stream.copy_from_current(30);
        assert(vec2.size() == 30);
        for (size_t i = 0; i < 30; ++i) {
            assert(vec2[i] == original[20 + i]);
        }

        // 拷贝超出范围
        stream.seek(90);
        auto vec3 = stream.copy_from_current(20);
        assert(vec3.size() == 10);  // 只有10个字节可拷贝

        // 比较相等性
        memory_stream stream2(block_sizes::Bytes_256);
        stream2.write(original.data(), original.size());
        assert(stream.equals(stream2));

        // 不同数据的比较
        memory_stream stream3(block_sizes::Bytes_256);
        stream3.write(original.data(), 50);
        assert(!stream.equals(stream3));
    }

    static void test_skip_operations() {
        memory_stream stream(block_sizes::Bytes_128);

        // 准备数据
        std::vector<unsigned char> data(50);
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = static_cast<unsigned char>(i + 1);
        }
        stream.write(data.data(), data.size());

        // skip
        assert(stream.skip(10));
        assert(stream.tell() == 10);

        // 验证跳过后的读取
        unsigned char buffer[5];
        size_t read = stream.read(buffer, 5);
        assert(read == 5);
        for (size_t i = 0; i < 5; ++i) {
            assert(buffer[i] == data[10 + i]);
        }

        // skip_until
        stream.rewind();
        auto pos = stream.skip_until(25);
        assert(pos && *pos == 24);  // 值25在位置24
        assert(stream.tell() == 24);

        // skip_until 找不到
        stream.rewind();
        pos = stream.skip_until(100);
        assert(!pos);
        assert(stream.tell() == 0);  // 位置不变

        // skip 超出范围
        assert(!stream.skip(100));
    }

    static void test_edge_cases() {
        // 空流测试
        memory_stream empty_stream(block_sizes::Bytes_64);

        assert(empty_stream.empty());
        assert(empty_stream.read(nullptr, 10) == 0);
        assert(!empty_stream.read_byte());
        assert(!empty_stream.peek_byte(0));
        assert(empty_stream.peek(0, nullptr, 10) == 0);
        assert(!empty_stream.seek(1));
        assert(empty_stream.tell() == 0);
        assert(!empty_stream.front());
        assert(!empty_stream.back());
        assert(!empty_stream.find(0));
        assert(empty_stream.copy_to_vector().empty());
        assert(!empty_stream.skip(1));

        // 写入空数据
        memory_stream stream(block_sizes::Bytes_128);
        assert(stream.write(nullptr, 0) == 0);
        assert(stream.write(nullptr, 10) == 0);

        unsigned char data[5] = {1, 2, 3, 4, 5};
        assert(stream.write(data, 0) == 0);

        // peek 空指针
        assert(stream.peek(0, nullptr, 10) == 0);

        // 读取位置在末尾时的操作
        stream.write(data, 5);
        stream.seek(5);
        assert(stream.eof());
        assert(stream.read(data, 5) == 0);
        assert(!stream.read_byte());
    }

    static void test_large_data() {
        memory_stream stream(block_sizes::KB_1);

        // 写入大量数据
        const size_t large_size = 5000;  // 5KB，超过一个chunk
        std::vector<unsigned char> large_data(large_size);
        for (size_t i = 0; i < large_size; ++i) {
            large_data[i] = static_cast<unsigned char>(i % 256);
        }

        size_t written = stream.write(large_data.data(), large_size);
        assert(written == large_size);
        assert(stream.size() == large_size);
        assert(stream.chunk_count() >= 5);  // 至少5个chunk

        // 读取大量数据
        std::vector<unsigned char> read_buffer(large_size);
        size_t read = stream.read(read_buffer.data(), large_size);
        assert(read == large_size);

        // 验证数据一致性
        for (size_t i = 0; i < large_size; ++i) {
            assert(read_buffer[i] == large_data[i]);
        }

        // 测试跨chunk的peek
        stream.rewind();
        unsigned char buffer[1500];  // 跨多个chunk
        size_t peeked = stream.peek(500, buffer, 1500);
        assert(peeked == 1500);
        for (size_t i = 0; i < 1500; ++i) {
            assert(buffer[i] == large_data[500 + i]);
        }
    }

    static void test_mixed_operations() {
        memory_stream stream(block_sizes::Bytes_256);

        // 混合写入操作
        assert(stream.write_byte(1));

        unsigned char data1[] = {2, 3, 4};
        assert(stream.write(data1, 3) == 3);

        assert(stream.fill(5, 3) == 3);

        // 当前数据: 1, 2, 3, 4, 5, 5, 5
        assert(stream.size() == 7);

        // 混合读取操作
        unsigned char buffer[4];
        assert(stream.read(buffer, 2) == 2);
        assert(buffer[0] == 1 && buffer[1] == 2);
        assert(stream.tell() == 2);

        // peek
        assert(stream.peek(3, buffer, 3) == 3);
        assert(buffer[0] == 4 && buffer[1] == 5 && buffer[2] == 5);
        assert(stream.tell() == 2);  // peek不移动位置

        // 读取单个字节
        auto byte = stream.read_byte();
        assert(byte && *byte == 3);
        assert(stream.tell() == 3);

        // seek和读取
        assert(stream.seek(5));
        byte = stream.read_byte();
        assert(byte && *byte == 5);

        // 再次写入
        unsigned char data2[] = {6, 7, 8};
        assert(stream.write(data2, 3) == 3);

        // 总数据: 1, 2, 3, 4, 5, 5, 5, 6, 7, 8
        assert(stream.size() == 10);

        // 混合查找和跳过
        stream.rewind();
        auto pos = stream.skip_until(5);
        assert(pos && *pos == 4);  // 第一个5在位置4
        assert(stream.tell() == 4);

        // 拷贝当前数据
        auto vec = stream.copy_from_current(3);
        assert(vec.size() == 3);
        assert(vec[0] == 5 && vec[1] == 5 && vec[2] == 5);
    }
};