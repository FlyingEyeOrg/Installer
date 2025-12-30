#include <fmt/color.h>
#include <fmt/core.h>

#include <cassert>
#include <random>
#include <vector>

#include "memory_chunk.hpp"

class memory_chunk_tests {
   public:
    static void run_all_tests() {
        fmt::print(fg(fmt::color::light_blue) | fmt::emphasis::bold,
                   "=== 开始 MemoryChunk 测试 ===\n\n");

        int passed = 0;
        int total = 0;

        passed += test_constructor();
        total++;
        passed += test_move_operations();
        total++;
        passed += test_write_operations();
        total++;
        passed += test_read_operations();
        total++;
        passed += test_update_operations();
        total++;
        passed += test_delete_operations();
        total++;
        passed += test_access_operations();
        total++;
        passed += test_capacity_operations();
        total++;
        passed += test_utility_operations();
        total++;
        passed += test_edge_cases();
        total++;

        fmt::print("\n");
        if (passed == total) {
            fmt::print(fg(fmt::color::green) | fmt::emphasis::bold,
                       "=== 所有 {} 个测试全部通过 ===\n", total);
        } else {
            fmt::print(fg(fmt::color::red) | fmt::emphasis::bold,
                       "=== 测试失败: {}/{} 通过 ===\n", passed, total);
        }
    }

   private:
    static bool test_constructor() {
        fmt::print(fg(fmt::color::cyan), "测试构造函数...\n");
        bool all_passed = true;

        // 测试默认构造
        {
            memory_chunk mc;
            fmt::print("  {}默认构造: ", get_check_mark(true));
            fmt::print("size={}, capacity={}, empty={}\n", mc.size(),
                       mc.capacity(), mc.empty() ? "true" : "false");
            assert(mc.empty());
            assert(mc.capacity() == 0);
        }

        // 测试带容量的构造
        {
            memory_chunk mc(10);
            fmt::print("  {}带容量构造: ", get_check_mark(true));
            fmt::print("size={}, capacity={}\n", mc.size(), mc.capacity());
            assert(mc.capacity() == 10);
            assert(mc.empty());
            assert(mc.available_space() == 10);
        }

        fmt::print(fg(fmt::color::green), "构造函数测试完成 ✓\n\n");
        return all_passed;
    }

    static bool test_move_operations() {
        fmt::print(fg(fmt::color::cyan), "测试移动操作...\n");
        bool all_passed = true;

        // 测试移动构造
        {
            memory_chunk mc1(10);
            unsigned char data[] = {1, 2, 3, 4, 5};
            mc1.write(data, 5);

            memory_chunk mc2(std::move(mc1));
            fmt::print("  {}移动构造: ", get_check_mark(true));
            fmt::print("原对象 size={}, 新对象 size={}\n", mc1.size(),
                       mc2.size());
            assert(mc1.size() == 0);
            assert(mc1.capacity() == 0);
            assert(mc1.data() == nullptr);
            assert(mc2.size() == 5);
            assert(mc2.capacity() == 10);
        }

        // 测试移动赋值
        {
            memory_chunk mc1(10);
            memory_chunk mc2(5);
            unsigned char data[] = {6, 7, 8};
            mc1.write(data, 3);

            mc2 = std::move(mc1);
            fmt::print("  {}移动赋值: ", get_check_mark(true));
            fmt::print("目标对象 size={}\n", mc2.size());
            assert(mc1.size() == 0);
            assert(mc1.capacity() == 0);
            assert(mc2.size() == 3);
        }

        fmt::print(fg(fmt::color::green), "移动操作测试完成 ✓\n\n");
        return all_passed;
    }

    static bool test_write_operations() {
        fmt::print(fg(fmt::color::cyan), "测试写入操作...\n");
        bool all_passed = true;

        memory_chunk mc(20);

        // 测试 write
        {
            unsigned char data[] = {1, 2, 3, 4, 5};
            size_t written = mc.write(data, 5);
            fmt::print("  {}write: ", get_check_mark(true));
            fmt::print("写入 {} 字节\n", written);
            assert(written == 5);
            assert(mc.size() == 5);

            // 验证数据
            unsigned char buffer[5];
            mc.peek(buffer, 5);
            for (int i = 0; i < 5; i++) {
                assert(buffer[i] == i + 1);
            }
        }

        // 测试 write 溢出
        {
            unsigned char data[20];
            for (int i = 0; i < 20; i++) {
                data[i] = static_cast<unsigned char>(i);
            }
            size_t written = mc.write(data, 20);
            fmt::print("  {}write 溢出: ", get_check_mark(true));
            fmt::print("写入 {} 字节\n", written);
            assert(written == 15);  // 只剩15字节空间
            assert(mc.full());
        }

        mc.clear();

        // 测试 insert
        {
            mc.write(reinterpret_cast<const unsigned char*>("ABCDE"), 5);
            unsigned char data[] = {88, 99};
            bool result = mc.insert(2, data, 2);
            fmt::print("  {}insert: ", get_check_mark(result));
            fmt::print("在位置2插入2字节 {}\n", result ? "成功" : "失败");
            assert(result);
            assert(mc.size() == 7);

            unsigned char buffer[7];
            mc.peek(buffer, 7);
            assert(buffer[0] == 'A');
            assert(buffer[1] == 'B');
            assert(buffer[2] == 88);
            assert(buffer[3] == 99);
        }

        // 测试 write_byte
        {
            mc.clear();
            bool result1 = mc.write_byte(100);
            bool result2 = mc.write_byte(200);
            bool success = result1 && result2;
            fmt::print("  {}write_byte: ", get_check_mark(success));
            fmt::print("写入字节 {} 和 {}\n", 100, 200);
            assert(result1);
            assert(result2);
            assert(mc.size() == 2);
            assert(mc[0] == 100);
            assert(mc[1] == 200);
        }

        // 测试 push_back
        {
            mc.clear();
            bool result = mc.push_back(42);
            fmt::print("  {}push_back: ", get_check_mark(result));
            fmt::print("追加字节 {}\n", 42);
            assert(result);
            assert(mc.size() == 1);
            assert(mc[0] == 42);
        }

        // 测试 fill
        {
            mc.clear();
            size_t filled = mc.fill(255, 10);
            fmt::print("  {}fill: ", get_check_mark(filled == 10));
            fmt::print("填充 {} 个 255\n", filled);
            assert(filled == 10);
            assert(mc.size() == 10);

            for (size_t i = 0; i < mc.size(); i++) {
                assert(mc[i] == 255);
            }
        }

        fmt::print(fg(fmt::color::green), "写入操作测试完成 ✓\n\n");
        return all_passed;
    }

    static bool test_read_operations() {
        fmt::print(fg(fmt::color::cyan), "测试读取操作...\n");
        bool all_passed = true;

        memory_chunk mc(20);
        unsigned char data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        mc.write(data, 10);

        // 测试 peek
        {
            unsigned char buffer[5];
            size_t read = mc.peek(buffer, 5);
            fmt::print("  {}peek: ", get_check_mark(read == 5));
            fmt::print("读取 {} 字节\n", read);
            assert(read == 5);
            assert(mc.size() == 10);  // 数据未移除
            for (int i = 0; i < 5; i++) {
                assert(buffer[i] == i + 1);
            }
        }

        // 测试 peek_at
        {
            unsigned char buffer[3];
            size_t read = mc.peek_at(3, buffer, 3);
            fmt::print("  {}peek_at: ", get_check_mark(read == 3));
            fmt::print("从位置3读取 {} 字节\n", read);
            assert(read == 3);
            assert(buffer[0] == 4);
            assert(buffer[1] == 5);
            assert(buffer[2] == 6);
        }

        // 测试 read
        {
            unsigned char buffer[4];
            size_t read = mc.read(buffer, 4);
            fmt::print("  {}read: ", get_check_mark(read == 4));
            fmt::print("读取并移除 {} 字节\n", read);
            assert(read == 4);
            assert(mc.size() == 6);  // 数据被移除
            assert(mc[0] == 5);      // 现在第一个是5
        }

        // 测试 read_byte
        {
            auto byte = mc.read_byte();
            bool success = byte.has_value();
            fmt::print("  {}read_byte: ", get_check_mark(success));
            if (success) {
                fmt::print("读取到字节 {}\n", static_cast<int>(*byte));
                assert(*byte == 5);
            } else {
                fmt::print("读取失败\n");
            }
            assert(mc.size() == 5);
        }

        // 测试 pop_front
        {
            auto byte = mc.pop_front();
            bool success = byte.has_value();
            fmt::print("  {}pop_front: ", get_check_mark(success));
            if (success) {
                fmt::print("读取到字节 {}\n", static_cast<int>(*byte));
                assert(*byte == 6);
            } else {
                fmt::print("读取失败\n");
            }
            assert(mc.size() == 4);
        }

        // 测试 pop_back
        {
            auto byte = mc.pop_back();
            bool success = byte.has_value();
            fmt::print("  {}pop_back: ", get_check_mark(success));
            if (success) {
                fmt::print("读取到字节 {}\n", static_cast<int>(*byte));
                assert(*byte == 10);
            } else {
                fmt::print("读取失败\n");
            }
            assert(mc.size() == 3);
        }

        fmt::print(fg(fmt::color::green), "读取操作测试完成 ✓\n\n");
        return all_passed;
    }

    static bool test_update_operations() {
        fmt::print(fg(fmt::color::cyan), "测试更新操作...\n");
        bool all_passed = true;

        memory_chunk mc(20);
        unsigned char data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        mc.write(data, 10);

        // 测试 update
        {
            unsigned char new_data[] = {100, 101, 102};
            bool result = mc.update(3, new_data, 3);
            fmt::print("  {}update: ", get_check_mark(result));
            fmt::print("更新位置3-5的数据 {}\n", result ? "成功" : "失败");
            assert(result);
            assert(mc[3] == 100);
            assert(mc[4] == 101);
            assert(mc[5] == 102);
        }

        // 测试 update_byte
        {
            bool result = mc.update_byte(7, 200);
            fmt::print("  {}update_byte: ", get_check_mark(result));
            fmt::print("更新位置7的字节为 {}\n", 200);
            assert(result);
            assert(mc[7] == 200);
        }

        fmt::print(fg(fmt::color::green), "更新操作测试完成 ✓\n\n");
        return all_passed;
    }

    static bool test_delete_operations() {
        fmt::print(fg(fmt::color::cyan), "测试删除操作...\n");
        bool all_passed = true;

        memory_chunk mc(20);
        unsigned char data[20];
        for (int i = 0; i < 20; i++) {
            data[i] = static_cast<unsigned char>(i);
        }
        mc.write(data, 20);

        // 测试 consume_front
        {
            bool result = mc.consume_front(5);
            fmt::print("  {}consume_front: ", get_check_mark(result));
            fmt::print("删除前5字节 {}\n", result ? "成功" : "失败");
            assert(result);
            assert(mc.size() == 15);
            assert(mc[0] == 5);  // 删除后第一个是原来的第5个
        }

        // 测试 consume_back
        {
            bool result = mc.consume_back(3);
            fmt::print("  {}consume_back: ", get_check_mark(result));
            fmt::print("删除后3字节 {}\n", result ? "成功" : "失败");
            assert(result);
            assert(mc.size() == 12);
            assert(mc[mc.size() - 1] == 16);  // 删除后最后一个是原来的第16个
        }

        // 测试 erase
        {
            bool result = mc.erase(3, 4);
            fmt::print("  {}erase: ", get_check_mark(result));
            fmt::print("从位置3删除4字节 {}\n", result ? "成功" : "失败");
            assert(result);
            assert(mc.size() == 8);

            // 验证删除后的数据
            // 原始数据（删除前）: 5,6,7,8,9,10,11,12,13,14,15,16
            // 删除位置3开始的4个: 8,9,10,11
            // 删除后: 5,6,7,12,13,14,15,16
            assert(mc[0] == 5);
            assert(mc[1] == 6);
            assert(mc[2] == 7);
            assert(mc[3] == 12);  // 正确值应该是12
            assert(mc[4] == 13);
            assert(mc[5] == 14);
            assert(mc[6] == 15);
            assert(mc[7] == 16);
        }

        // 测试 erase_at
        {
            // 当前数据: 5,6,7,12,13,14,15,16
            bool result = mc.erase_at(2);  // 删除索引2（值7）
            fmt::print("  {}erase_at: ", get_check_mark(result));
            fmt::print("删除位置2的字节 {}\n", result ? "成功" : "失败");
            assert(result);
            assert(mc.size() == 7);
            assert(mc[0] == 5);
            assert(mc[1] == 6);
            assert(mc[2] == 12);  // 原来索引3的值
        }

        // 测试 clear
        {
            mc.clear();
            fmt::print("  {}clear: ", get_check_mark(true));
            fmt::print("清空所有数据\n");
            assert(mc.empty());
        }

        fmt::print(fg(fmt::color::green), "删除操作测试完成 ✓\n\n");
        return all_passed;
    }

    static bool test_access_operations() {
        fmt::print(fg(fmt::color::cyan), "测试访问操作...\n");
        bool all_passed = true;

        memory_chunk mc(10);
        unsigned char data[] = {10, 20, 30, 40, 50};
        mc.write(data, 5);

        // 测试 operator[]
        {
            fmt::print("  {}operator[]: ", get_check_mark(true));
            try {
                assert(mc[0] == 10);
                assert(mc[1] == 20);
                assert(mc[4] == 50);
                fmt::print("正常访问\n");

                // 测试越界访问应该抛出异常
                try {
                    unsigned char val = mc[10];
                    fmt::print("  {}operator[] 越界测试: ",
                               get_check_mark(false));
                    fmt::print("应该抛出异常但未抛出\n");
                    all_passed = false;
                } catch (const std::out_of_range&) {
                    fmt::print("  {}operator[] 越界测试: ",
                               get_check_mark(true));
                    fmt::print("正确抛出异常\n");
                }
            } catch (const std::exception& e) {
                fmt::print("  {}operator[]: ", get_check_mark(false));
                fmt::print("异常: {}\n", e.what());
                all_passed = false;
            }
        }

        // 测试 at
        {
            auto val1 = mc.at(0);
            auto val2 = mc.at(2);
            auto val3 = mc.at(10);

            bool success = val1 && *val1 == 10 && val2 && *val2 == 30 && !val3;
            fmt::print("  {}at: ", get_check_mark(success));
            fmt::print("安全访问测试\n");
            assert(success);
        }

        // 测试 front
        {
            auto val = mc.front();
            bool success = val.has_value();
            fmt::print("  {}front: ", get_check_mark(success));
            if (success) {
                fmt::print("获取到第一个字节 {}\n", static_cast<int>(*val));
                assert(*val == 10);
            } else {
                fmt::print("获取失败\n");
            }
        }

        // 测试 back
        {
            auto val = mc.back();
            bool success = val.has_value();
            fmt::print("  {}back: ", get_check_mark(success));
            if (success) {
                fmt::print("获取到最后一个字节 {}\n", static_cast<int>(*val));
                assert(*val == 50);
            } else {
                fmt::print("获取失败\n");
            }
        }

        fmt::print(fg(fmt::color::green), "访问操作测试完成 ✓\n\n");
        return all_passed;
    }

    static bool test_capacity_operations() {
        fmt::print(fg(fmt::color::cyan), "测试容量操作...\n");
        bool all_passed = true;

        memory_chunk mc(15);
        unsigned char data[] = {1, 2, 3, 4, 5};
        mc.write(data, 5);

        fmt::print("  {}size: {}\n", get_check_mark(true), mc.size());
        fmt::print("  {}capacity: {}\n", get_check_mark(true), mc.capacity());
        fmt::print("  {}available_space: {}\n", get_check_mark(true),
                   mc.available_space());
        fmt::print("  {}empty: {}\n", get_check_mark(true),
                   mc.empty() ? "true" : "false");
        fmt::print("  {}full: {}\n", get_check_mark(true),
                   mc.full() ? "true" : "false");

        assert(mc.size() == 5);
        assert(mc.capacity() == 15);
        assert(mc.available_space() == 10);
        assert(!mc.empty());
        assert(!mc.full());

        // 测试 contiguous_space
        fmt::print("  {}contiguous_space: {}\n", get_check_mark(true),
                   mc.contiguous_space());
        assert(mc.contiguous_space() == 10);

        fmt::print(fg(fmt::color::green), "容量操作测试完成 ✓\n\n");
        return all_passed;
    }

    static bool test_utility_operations() {
        fmt::print(fg(fmt::color::cyan), "测试工具操作...\n");
        bool all_passed = true;

        // 测试 data 和迭代器
        {
            memory_chunk mc(10);
            unsigned char data[] = {1, 2, 3, 4, 5};
            mc.write(data, 5);

            fmt::print("  {}data/begin/end: ", get_check_mark(true));
            assert(mc.data() == mc.begin());
            assert(mc.begin() + 5 == mc.end());

            // 测试遍历
            int sum = 0;
            for (auto it = mc.begin(); it != mc.end(); ++it) {
                sum += *it;
            }
            fmt::print("迭代器正常，元素和={}\n", sum);
            assert(sum == 15);
        }

        // 测试 resize
        {
            memory_chunk mc(5);
            mc.write(reinterpret_cast<const unsigned char*>("ABC"), 3);

            bool result = mc.resize(10);
            fmt::print("  {}resize 扩大: ", get_check_mark(result));
            fmt::print("从5扩大到10 {}\n", result ? "成功" : "失败");
            assert(result);
            assert(mc.capacity() == 10);
            assert(mc.size() == 3);

            // 验证数据完整
            unsigned char buffer[3];
            mc.peek(buffer, 3);
            assert(buffer[0] == 'A' && buffer[1] == 'B' && buffer[2] == 'C');

            // 测试缩小容量失败
            result = mc.resize(2);
            fmt::print("  {}resize 缩小失败: ", get_check_mark(!result));
            fmt::print("尝试缩小到2 {}\n", result ? "错误成功" : "正确失败");
            assert(!result);
        }

        // 测试 swap
        {
            memory_chunk mc1(10);
            memory_chunk mc2(5);
            mc1.write(reinterpret_cast<const unsigned char*>("Hello"), 5);
            mc2.write(reinterpret_cast<const unsigned char*>("World"), 5);

            mc1.swap(mc2);
            fmt::print("  {}swap: ", get_check_mark(true));

            unsigned char buffer1[6], buffer2[6];
            mc1.peek(buffer1, 5);
            mc2.peek(buffer2, 5);
            buffer1[5] = buffer2[5] = 0;

            fmt::print("交换后: mc1='{}', mc2='{}'\n",
                       reinterpret_cast<char*>(buffer1),
                       reinterpret_cast<char*>(buffer2));

            assert(mc1.size() == 5);
            assert(mc1.capacity() == 5);
            assert(mc2.size() == 5);
            assert(mc2.capacity() == 10);
        }

        // 测试 copy_from
        {
            memory_chunk source(10);
            memory_chunk dest(8);
            source.write(reinterpret_cast<const unsigned char*>("SourceData"),
                         10);

            size_t copied = dest.copy_from(source);
            fmt::print("  {}copy_from: ", get_check_mark(copied == 8));
            fmt::print("复制 {} 字节\n", copied);
            assert(copied == 8);
            assert(dest.size() == 8);

            copied = dest.copy_from(source, 3, 4);
            fmt::print("  {}copy_from 指定位置: ", get_check_mark(copied == 0));
            fmt::print("复制 {} 字节\n", copied);
            assert(copied == 0);  // dest 已满
        }

        // 测试 find
        {
            memory_chunk mc(20);
            mc.write(reinterpret_cast<const unsigned char*>("Hello World!"),
                     12);

            auto pos = mc.find('W');
            fmt::print("  {}find: ",
                       get_check_mark(pos.has_value() && *pos == 6));
            if (pos) {
                fmt::print("在位置 {} 找到 'W'\n", *pos);
                assert(*pos == 6);
            } else {
                fmt::print("未找到 'W'\n");
            }

            pos = mc.find('X', 0);
            fmt::print("  {}find 不存在的字符: ",
                       get_check_mark(!pos.has_value()));
            if (!pos) {
                fmt::print("正确: 未找到 'X'\n");
            } else {
                fmt::print("错误: 找到了 'X'\n");
            }
        }

        // 测试 equals
        {
            memory_chunk mc1(10);
            memory_chunk mc2(10);
            mc1.write(reinterpret_cast<const unsigned char*>("Test"), 4);
            mc2.write(reinterpret_cast<const unsigned char*>("Test"), 4);

            bool equal = mc1.equals(mc2);
            fmt::print("  {}equals 相同: ", get_check_mark(equal));
            fmt::print("{}\n", equal ? "true" : "false");
            assert(equal);

            memory_chunk mc3(10);
            mc3.write(reinterpret_cast<const unsigned char*>("Diff"), 4);
            equal = mc1.equals(mc3);
            fmt::print("  {}equals 不同: ", get_check_mark(!equal));
            fmt::print("{}\n", equal ? "true" : "false");
            assert(!equal);

            equal =
                mc1.equals(reinterpret_cast<const unsigned char*>("Test"), 4);
            fmt::print("  {}equals 原始数据: ", get_check_mark(equal));
            fmt::print("{}\n", equal ? "true" : "false");
            assert(equal);
        }

        // 测试 contiguous_write
        {
            memory_chunk mc(10);
            mc.write(reinterpret_cast<const unsigned char*>("Hello"), 5);

            auto [ptr, size] = mc.contiguous_write();
            fmt::print("  {}contiguous_write: ", get_check_mark(size == 5));
            fmt::print("剩余连续空间 {} 字节\n", size);
            assert(size == 5);
            assert(ptr == mc.data() + 5);

            // 写入数据
            std::memcpy(ptr, "World", 5);
            mc.write(reinterpret_cast<const unsigned char*>(""),
                     5);  // 更新size

            unsigned char buffer[10];
            mc.peek(buffer, 10);
            buffer[10] = 0;
            fmt::print("    写入后数据: '{}'\n",
                       reinterpret_cast<char*>(buffer));
        }

        fmt::print(fg(fmt::color::green), "工具操作测试完成 ✓\n\n");
        return all_passed;
    }

    static bool test_edge_cases() {
        fmt::print(fg(fmt::color::cyan), "测试边界条件...\n");
        bool all_passed = true;

        // 测试空 chunk
        {
            memory_chunk mc(0);
            fmt::print("  {}空chunk测试: ", get_check_mark(true));
            fmt::print("空容量\n");
            assert(mc.empty());
            assert(mc.full());
            assert(mc.size() == 0);
            assert(mc.capacity() == 0);
            assert(mc.data() == nullptr);

            // 写入应该失败
            unsigned char data = 1;
            size_t written = mc.write(&data, 1);
            assert(written == 0);
        }

        // 测试边界写入
        {
            memory_chunk mc(3);
            unsigned char data[] = {1, 2, 3, 4};  // 超过容量

            size_t written = mc.write(data, 4);
            fmt::print("  {}边界写入: ", get_check_mark(written == 3));
            fmt::print("写入 {}/4 字节\n", written);
            assert(written == 3);
            assert(mc.full());
        }

        // 测试空指针处理
        {
            memory_chunk mc(10);

            size_t result = mc.write(nullptr, 5);
            fmt::print("  {}空指针写入: ", get_check_mark(result == 0));
            fmt::print("{} 字节写入\n", result);
            assert(result == 0);

            result = mc.peek(nullptr, 5);
            fmt::print("  {}空指针读取: ", get_check_mark(result == 0));
            fmt::print("{} 字节读取\n", result);
            assert(result == 0);

            bool success = mc.insert(0, nullptr, 5);
            fmt::print("  {}空指针插入: ", get_check_mark(!success));
            fmt::print("{}\n", success ? "成功" : "失败");
            assert(!success);
        }

        // 测试零长度操作
        {
            memory_chunk mc(10);
            unsigned char data[] = {1, 2, 3};

            mc.write(data, 3);
            size_t result = mc.write(data, 0);
            fmt::print("  {}零长度写入: ", get_check_mark(result == 0));
            fmt::print("{} 字节写入\n", result);
            assert(result == 0);
            assert(mc.size() == 3);

            result = mc.peek(data, 0);
            fmt::print("  {}零长度读取: ", get_check_mark(result == 0));
            fmt::print("{} 字节读取\n", result);
            assert(result == 0);

            bool success = mc.consume_front(0);
            fmt::print("  {}零长度删除: ", get_check_mark(success));
            fmt::print("{}\n", success ? "成功" : "失败");
            assert(success);
            assert(mc.size() == 3);
        }

        // 测试随机操作
        {
            fmt::print("  {}随机操作测试: ", get_check_mark(true));
            std::mt19937 rng(42);
            std::uniform_int_distribution<> dist(0, 255);

            memory_chunk mc(100);
            std::vector<unsigned char> reference;

            for (int i = 0; i < 1000; i++) {
                int op = rng() % 10;

                switch (op) {
                    case 0: {  // write
                        int count = rng() % 20;
                        std::vector<unsigned char> data(count);
                        for (auto& byte : data)
                            byte = static_cast<unsigned char>(dist(rng));

                        size_t written = mc.write(data.data(), count);
                        for (size_t j = 0; j < written; j++) {
                            reference.push_back(data[j]);
                        }
                        break;
                    }
                    case 1: {  // read
                        int count = rng() % 20;
                        std::vector<unsigned char> buffer(count);

                        size_t read = mc.read(buffer.data(), count);
                        for (size_t j = 0; j < read; j++) {
                            assert(buffer[j] == reference[j]);
                        }
                        reference.erase(reference.begin(),
                                        reference.begin() + read);
                        break;
                    }
                    case 2: {  // pop_front
                        auto byte = mc.pop_front();
                        if (byte) {
                            assert(*byte == reference.front());
                            reference.erase(reference.begin());
                        }
                        break;
                    }
                    case 3: {  // update
                        if (!reference.empty()) {
                            int pos = rng() % reference.size();
                            unsigned char new_byte =
                                static_cast<unsigned char>(dist(rng));
                            mc.update_byte(pos, new_byte);
                            reference[pos] = new_byte;
                        }
                        break;
                    }
                }

                // 验证一致性
                assert(mc.size() == reference.size());
                for (size_t j = 0; j < mc.size(); j++) {
                    assert(mc[j] == reference[j]);
                }
            }
            fmt::print("1000次随机操作通过\n");
        }

        fmt::print(fg(fmt::color::green), "边界条件测试完成 ✓\n\n");
        return all_passed;
    }

    static std::string get_check_mark(bool passed) {
        if (passed) {
            return fmt::format(fg(fmt::color::green) | fmt::emphasis::bold,
                               "✓");
        } else {
            return fmt::format(fg(fmt::color::red) | fmt::emphasis::bold, "✗");
        }
    }
};