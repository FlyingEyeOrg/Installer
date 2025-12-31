#include <fmt/core.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "memory_stream/memory_stream.hpp"


// 测试辅助函数
template <typename T>
std::string bytes_to_hex(const T* data, size_t size) {
    static const char hex_chars[] = "0123456789ABCDEF";
    std::string result;
    result.reserve(size * 3);

    for (size_t i = 0; i < size; ++i) {
        if (i > 0) result.push_back(' ');
        result.push_back(hex_chars[(data[i] >> 4) & 0x0F]);
        result.push_back(hex_chars[data[i] & 0x0F]);
    }
    return result;
}

// 测试1: 基本构造和查询
bool test_basic_construction() {
    fmt::print("\n🧪 测试1: 基本构造和查询\n");

    try {
        // 测试默认构造函数
        memory_stream stream1(64);
        assert(stream1.empty());
        assert(stream1.size() == 0);
        assert(stream1.chunk_count() == 0);
        assert(stream1.chunk_capacity() == 64);
        fmt::print("  ✓ 默认构造测试通过\n");

        // 测试枚举构造
        memory_stream stream2(block_sizes::KB_1);
        assert(stream2.chunk_capacity() == 1024);
        fmt::print("  ✓ 枚举构造测试通过\n");

        // 测试移动构造
        memory_stream stream3(32);
        stream3.write(
            reinterpret_cast<const memory_stream::value_type*>("test"), 4);
        memory_stream stream4(std::move(stream3));
        assert(stream3.empty());
        assert(stream4.size() == 4);
        fmt::print("  ✓ 移动构造测试通过\n");

        // 测试移动赋值
        memory_stream stream5(16);
        stream5 = std::move(stream4);
        assert(stream4.empty());
        assert(stream5.size() == 4);
        fmt::print("  ✓ 移动赋值测试通过\n");

        fmt::print("✅ 测试1 全部通过\n");
        return true;
    } catch (const std::exception& e) {
        fmt::print("❌ 测试1 失败: {}\n", e.what());
        return false;
    }
}

// 测试2: 小数据写入和读取
bool test_small_data_io() {
    fmt::print("\n🧪 测试2: 小数据写入和读取\n");

    try {
        memory_stream stream(16);
        fmt::print("  创建流，块大小: {} 字节\n", stream.chunk_capacity());

        // 写入数据
        const char* data = "Hello, MemoryStream!";
        size_t data_len = 20;  // 不包括字符串结尾的\0
        size_t written = stream.write(
            reinterpret_cast<const memory_stream::value_type*>(data), data_len);

        fmt::print("  写入数据: '{}' ({} 字节)\n", std::string(data, data_len),
                   written);
        assert(written == data_len);
        assert(stream.size() == data_len);
        assert(!stream.empty());
        assert(stream.chunk_count() == 1);

        // peek 测试
        char peek_buffer[6] = {0};
        size_t peeked = stream.peek(
            reinterpret_cast<memory_stream::value_type*>(peek_buffer), 5);
        assert(peeked == 5);
        assert(std::string(peek_buffer, 5) == "Hello");
        fmt::print("  peek 前5字节: '{}'\n", std::string(peek_buffer, 5));

        // 读取测试
        char buffer[21] = {0};
        size_t read = stream.read(
            reinterpret_cast<memory_stream::value_type*>(buffer), 20);
        assert(read == 20);
        assert(std::string(buffer, 20) == std::string(data, 20));
        fmt::print("  读取数据: '{}' ({} 字节)\n", std::string(buffer, 20),
                   read);

        // 验证读取后数据仍在
        assert(stream.size() == 20);

        fmt::print("✅ 测试2 全部通过\n");
        return true;
    } catch (const std::exception& e) {
        fmt::print("❌ 测试2 失败: {}\n", e.what());
        return false;
    }
}

// 测试3: 跨块大数据写入
bool test_large_data_cross_chunks() {
    fmt::print("\n🧪 测试3: 跨块大数据写入\n");

    try {
        const size_t chunk_size = 32;
        const size_t total_data = 100;
        memory_stream stream(chunk_size);

        fmt::print("  块大小: {} 字节, 总数据: {} 字节\n", chunk_size,
                   total_data);

        // 准备测试数据
        std::vector<memory_stream::value_type> data(total_data);
        for (size_t i = 0; i < total_data; ++i) {
            data[i] = static_cast<memory_stream::value_type>(i % 256);
        }

        // 写入数据
        size_t written = stream.write(data.data(), total_data);
        assert(written == total_data);
        assert(stream.size() == total_data);

        // 验证块数
        size_t expected_chunks = (total_data + chunk_size - 1) / chunk_size;
        size_t actual_chunks = stream.chunk_count();
        fmt::print("  预期块数: {}, 实际块数: {}\n", expected_chunks,
                   actual_chunks);
        assert(actual_chunks == expected_chunks);

        // 验证块使用情况
        auto stats = stream.get_chunk_stats();
        fmt::print("  块使用统计:\n");
        for (const auto& stat : stats) {
            fmt::print("    块 {:2d}: {:3} / {:3} 字节 ({:5.1f}%)\n",
                       stat.chunk_index, stat.data_size, stat.capacity,
                       stat.usage_percent);

            // 前几个块应该都满了
            if (stat.chunk_index < actual_chunks - 1) {
                assert(stat.data_size == chunk_size);
                assert(stat.usage_percent == 100.0);
            }
        }

        // 验证数据
        std::vector<memory_stream::value_type> read_buffer(total_data);
        size_t read = stream.read(read_buffer.data(), total_data);
        assert(read == total_data);
        assert(std::equal(data.begin(), data.end(), read_buffer.begin()));

        fmt::print("  ✅ 数据验证通过 ({} 字节)\n", read);
        fmt::print("✅ 测试3 全部通过\n");
        return true;
    } catch (const std::exception& e) {
        fmt::print("❌ 测试3 失败: {}\n", e.what());
        return false;
    }
}

// 测试4: 逐字节操作
bool test_byte_by_byte_operations() {
    fmt::print("\n🧪 测试4: 逐字节操作\n");

    try {
        memory_stream stream(8);
        fmt::print("  块大小: {} 字节\n", stream.chunk_capacity());

        // 逐字节写入
        fmt::print("  写入: ");
        for (int i = 0; i < 10; ++i) {
            char ch = 'A' + i;
            bool success = stream.write_byte(ch);
            assert(success);
            fmt::print("'{}' ", ch);
        }
        fmt::print("\n");

        assert(stream.size() == 10);
        assert(stream.chunk_count() == 2);

        // 测试 front 和 back
        auto front_byte = stream.front();
        auto back_byte = stream.back();
        assert(front_byte.has_value() && front_byte.value() == 'A');
        assert(back_byte.has_value() && back_byte.value() == 'J');
        fmt::print("  首字节: '{}', 尾字节: '{}'\n",
                   static_cast<char>(front_byte.value()),
                   static_cast<char>(back_byte.value()));

        // 逐字节读取
        fmt::print("  读取: ");
        for (int i = 0; i < 10; ++i) {
            auto byte = stream.read_byte();
            assert(byte.has_value());
            assert(byte.value() == 'A' + i);
            fmt::print("'{}' ", static_cast<char>(byte.value()));
        }
        fmt::print("\n");

        // 测试读取完的情况
        assert(stream.read_byte() == std::nullopt);

        fmt::print("✅ 测试4 全部通过\n");
        return true;
    } catch (const std::exception& e) {
        fmt::print("❌ 测试4 失败: {}\n", e.what());
        return false;
    }
}

// 测试5: 批量填充操作
bool test_fill_operation() {
    fmt::print("\n🧪 测试5: 批量填充操作\n");

    try {
        memory_stream stream(16);
        fmt::print("  块大小: {} 字节\n", stream.chunk_capacity());

        // 批量填充
        const memory_stream::value_type fill_byte = 0xAA;
        const size_t fill_count = 50;
        size_t written = stream.fill(fill_byte, fill_count);

        fmt::print("  填充 {} 字节 0x{:02X}\n", written, fill_byte);
        assert(written == fill_count);
        assert(stream.size() == fill_count);

        // 验证填充结果
        std::vector<memory_stream::value_type> buffer(50);
        size_t peeked = stream.peek(buffer.data(), 50);
        assert(peeked == 50);

        bool all_same =
            std::all_of(buffer.begin(), buffer.end(),
                        [fill_byte](auto b) { return b == fill_byte; });
        assert(all_same);

        // 十六进制显示前20字节
        fmt::print("  前20字节: {}\n", bytes_to_hex(buffer.data(), 20));

        // 测试部分读取
        char small_buffer[10] = {0};
        size_t read = stream.read(
            reinterpret_cast<memory_stream::value_type*>(small_buffer), 10);
        assert(read == 10);
        assert(std::all_of(small_buffer, small_buffer + 10,
                           [fill_byte](auto b) { return b == fill_byte; }));

        fmt::print("  ✓ 批量填充验证通过\n");
        fmt::print("✅ 测试5 全部通过\n");
        return true;
    } catch (const std::exception& e) {
        fmt::print("❌ 测试5 失败: {}\n", e.what());
        return false;
    }
}

// 测试6: 迭代器测试
bool test_iterators() {
    fmt::print("\n🧪 测试6: 迭代器测试\n");

    try {
        memory_stream stream(8);

        // 写入数据
        std::string test_data = "ABCDEFGHIJKLMNOPQRST";
        stream.write(reinterpret_cast<const memory_stream::value_type*>(
                         test_data.c_str()),
                     test_data.size());

        fmt::print("  写入数据: '{}' ({} 字节)\n", test_data, test_data.size());

        // 使用迭代器遍历
        std::string from_iterator;
        for (auto it = stream.begin(); it != stream.end(); ++it) {
            from_iterator.push_back(static_cast<char>(*it));
        }

        fmt::print("  迭代器遍历结果: '{}'\n", from_iterator);
        assert(from_iterator == test_data);

        // 使用范围for遍历
        std::string from_range_for;
        for (auto byte : stream) {
            from_range_for.push_back(static_cast<char>(byte));
        }

        fmt::print("  范围for遍历结果: '{}'\n", from_range_for);
        assert(from_range_for == test_data);

        // 测试const迭代器
        const memory_stream& const_stream = stream;
        std::string from_const_iterator;
        for (auto it = const_stream.cbegin(); it != const_stream.cend(); ++it) {
            from_const_iterator.push_back(static_cast<char>(*it));
        }

        fmt::print("  const迭代器遍历结果: '{}'\n", from_const_iterator);
        assert(from_const_iterator == test_data);

        fmt::print("✅ 测试6 全部通过\n");
        return true;
    } catch (const std::exception& e) {
        fmt::print("❌ 测试6 失败: {}\n", e.what());
        return false;
    }
}

// 测试7: 消费操作测试
bool test_consume_operation() {
    fmt::print("\n🧪 测试7: 消费操作测试\n");

    try {
        memory_stream stream(16);

        // 写入数据
        std::string data = "HelloWorld1234567890";
        stream.write(
            reinterpret_cast<const memory_stream::value_type*>(data.c_str()),
            data.size());

        fmt::print("  初始数据: '{}' ({} 字节)\n", data, data.size());
        fmt::print("  初始块数: {}\n", stream.chunk_count());

        // 消费部分数据
        char buffer1[5] = {0};
        size_t consumed1 = stream.consume(
            reinterpret_cast<memory_stream::value_type*>(buffer1), 5);

        fmt::print("  消费5字节: '{}'\n", std::string(buffer1, 5));
        assert(consumed1 == 5);
        assert(std::string(buffer1, 5) == "Hello");
        assert(stream.size() == 15);

        // 继续消费
        char buffer2[6] = {0};
        size_t consumed2 = stream.consume(
            reinterpret_cast<memory_stream::value_type*>(buffer2), 6);

        fmt::print("  消费6字节: '{}'\n", std::string(buffer2, 6));
        assert(consumed2 == 6);
        assert(std::string(buffer2, 6) == "World1");
        assert(stream.size() == 9);

        // 消费剩余数据
        char buffer3[10] = {0};
        size_t consumed3 = stream.consume(
            reinterpret_cast<memory_stream::value_type*>(buffer3), 10);

        fmt::print("  消费剩余 {} 字节: '{}'\n", consumed3,
                   std::string(buffer3, consumed3));
        assert(consumed3 == 9);
        assert(std::string(buffer3, 9) == "234567890");
        assert(stream.empty());

        fmt::print("  ✓ 消费操作验证通过\n");
        fmt::print("✅ 测试7 全部通过\n");
        return true;
    } catch (const std::exception& e) {
        fmt::print("❌ 测试7 失败: {}\n", e.what());
        return false;
    }
}

// 测试8: seek和位置操作
bool test_seek_and_position() {
    fmt::print("\n🧪 测试8: seek和位置操作\n");

    try {
        memory_stream stream(8);

        // 写入字母表
        std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        stream.write(reinterpret_cast<const memory_stream::value_type*>(
                         alphabet.c_str()),
                     26);

        fmt::print("  写入字母表 ({} 字节)\n", alphabet.size());

        // 测试 seek
        assert(stream.seek_read_position(5));

        char buffer[5] = {0};
        size_t read1 = stream.read(
            reinterpret_cast<memory_stream::value_type*>(buffer), 5);
        assert(read1 == 5);
        assert(std::string(buffer, 5) == "FGHIJ");
        fmt::print("  seek到位置5，读取: '{}'\n", std::string(buffer, 5));

        // 重置读取位置
        stream.reset_read_position();

        char buffer2[5] = {0};
        size_t read2 = stream.read(
            reinterpret_cast<memory_stream::value_type*>(buffer2), 5);
        assert(read2 == 5);
        assert(std::string(buffer2, 5) == "ABCDE");
        fmt::print("  重置位置，读取: '{}'\n", std::string(buffer2, 5));

        // 测试 seek 到末尾
        assert(stream.seek_read_position(20));

        char buffer3[6] = {0};
        size_t read3 = stream.read(
            reinterpret_cast<memory_stream::value_type*>(buffer3), 6);
        assert(read3 == 6);
        assert(std::string(buffer3, 6) == "UVWXYZ");
        fmt::print("  seek到位置20，读取: '{}'\n", std::string(buffer3, 6));

        // 测试无效 seek
        assert(!stream.seek_read_position(100));
        fmt::print("  ✓ 无效seek测试通过\n");

        fmt::print("✅ 测试8 全部通过\n");
        return true;
    } catch (const std::exception& e) {
        fmt::print("❌ 测试8 失败: {}\n", e.what());
        return false;
    }
}

// 测试9: compact 操作测试
bool test_compact_operation() {
    fmt::print("\n🧪 测试9: compact 操作测试\n");

    try {
        memory_stream stream(8);

        // 写入并消费，制造碎片
        std::string data = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        stream.write(
            reinterpret_cast<const memory_stream::value_type*>(data.c_str()),
            26);

        // 消费部分数据
        stream.consume(nullptr, 6);  // 消费 ABCDEF

        size_t before_compact = stream.chunk_count();
        auto stats_before = stream.get_chunk_stats();

        fmt::print("  compact前:\n");
        fmt::print("    块数: {}\n", before_compact);
        fmt::print("    总大小: {} 字节\n", stream.size());
        fmt::print("    使用统计:\n");
        for (const auto& stat : stats_before) {
            fmt::print("      块 {}: {}/{} 字节\n", stat.chunk_index,
                       stat.data_size, stat.capacity);
        }

        // 执行 compact
        stream.compact();

        size_t after_compact = stream.chunk_count();
        auto stats_after = stream.get_chunk_stats();

        fmt::print("  compact后:\n");
        fmt::print("    块数: {}\n", after_compact);
        fmt::print("    总大小: {} 字节\n", stream.size());
        fmt::print("    使用统计:\n");
        for (const auto& stat : stats_after) {
            fmt::print("      块 {}: {}/{} 字节\n", stat.chunk_index,
                       stat.data_size, stat.capacity);
        }

        // 验证数据
        std::vector<memory_stream::value_type> result(stream.size());
        size_t read = stream.read(result.data(), result.size());
        assert(read == 20);
        assert(std::string(result.begin(), result.end()) ==
               "GHIJKLMNOPQRSTUVWXYZ");

        fmt::print("  ✓ 数据验证通过: '{}'\n",
                   std::string(result.begin(), result.end()));

        // 验证块数应该减少
        assert(after_compact <= before_compact);

        fmt::print("✅ 测试9 全部通过\n");
        return true;
    } catch (const std::exception& e) {
        fmt::print("❌ 测试9 失败: {}\n", e.what());
        return false;
    }
}

// 测试10: trim 操作测试
bool test_trim_operation() {
    fmt::print("\n🧪 测试10: trim 操作测试\n");

    try {
        memory_stream stream(8);

        // 写入数据
        std::string data = "12345678901234567890";
        stream.write(
            reinterpret_cast<const memory_stream::value_type*>(data.c_str()),
            20);

        fmt::print("  初始: {} 字节, {} 个块\n", stream.size(),
                   stream.chunk_count());

        // 消费所有数据
        stream.consume(nullptr, 20);

        fmt::print("  消费后: {} 字节, {} 个块\n", stream.size(),
                   stream.chunk_count());
        assert(stream.empty());
        assert(stream.chunk_count() > 0);  // 应该还有空块

        // 执行 trim
        stream.trim();

        fmt::print("  trim后: {} 字节, {} 个块\n", stream.size(),
                   stream.chunk_count());
        assert(stream.empty());
        assert(stream.chunk_count() == 0);  // trim 后应该没有块了

        // 测试 trim 后可以重新写入
        bool write_success = stream.write_byte('X');
        assert(write_success);
        assert(stream.size() == 1);
        assert(stream.chunk_count() == 1);

        fmt::print("  ✓ trim后重新写入测试通过\n");
        fmt::print("✅ 测试10 全部通过\n");
        return true;
    } catch (const std::exception& e) {
        fmt::print("❌ 测试10 失败: {}\n", e.what());
        return false;
    }
}

// 测试11: 边界条件测试
bool test_edge_cases() {
    fmt::print("\n🧪 测试11: 边界条件测试\n");

    try {
        // 测试1: 空流操作
        {
            memory_stream stream(16);

            assert(stream.read_byte() == std::nullopt);
            assert(stream.read(nullptr, 10) == 0);
            assert(stream.peek(nullptr, 10) == 0);

            auto front = stream.front();
            auto back = stream.back();
            assert(!front.has_value());
            assert(!back.has_value());

            fmt::print("  ✓ 空流操作测试通过\n");
        }

        // 测试2: 零长度操作
        {
            memory_stream stream(16);

            assert(stream.write(nullptr, 0) == 0);
            assert(
                stream.write(
                    reinterpret_cast<const memory_stream::value_type*>("test"),
                    0) == 0);
            assert(stream.fill(0xFF, 0) == 0);

            fmt::print("  ✓ 零长度操作测试通过\n");
        }

        // 测试3: 写入 nullptr
        {
            memory_stream stream(16);

            size_t written = stream.write(nullptr, 10);
            assert(written == 0);
            assert(stream.size() == 0);

            fmt::print("  ✓ nullptr写入测试通过\n");
        }

        // 测试4: 缓冲区满的情况
        {
            memory_stream stream(8);

            // 写入刚好填满的数据
            std::string data = "12345678";
            size_t written =
                stream.write(reinterpret_cast<const memory_stream::value_type*>(
                                 data.c_str()),
                             8);
            assert(written == 8);
            assert(stream.full() ==
                   false);  // memory_stream 没有 full() 方法，memory_chunk 才有
            assert(stream.size() == 8);

            // 尝试写入更多数据
            written = stream.write(
                reinterpret_cast<const memory_stream::value_type*>("9"), 1);
            assert(written == 1);
            assert(stream.size() == 9);
            assert(stream.chunk_count() == 2);

            fmt::print("  ✓ 缓冲区满测试通过\n");
        }

        // 测试5: 大容量流
        {
            memory_stream stream(static_cast<size_t>(block_sizes::MB_1));
            assert(stream.chunk_capacity() == 1024 * 1024);

            // 写入少量数据
            std::string data = "Small data in large chunk";
            stream.write(reinterpret_cast<const memory_stream::value_type*>(
                             data.c_str()),
                         data.size());

            fmt::print("  ✓ 大容量流测试通过 (块大小: {} MB)\n",
                       stream.chunk_capacity() / (1024 * 1024));
        }

        fmt::print("✅ 测试11 全部通过\n");
        return true;
    } catch (const std::exception& e) {
        fmt::print("❌ 测试11 失败: {}\n", e.what());
        return false;
    }
}

// 测试12: 性能测试
bool test_performance() {
    fmt::print("\n🧪 测试12: 性能测试\n");

    try {
        const size_t chunk_size = 4096;         // 4KB
        const size_t total_size = 1024 * 1024;  // 1MB

        memory_stream stream(chunk_size);

        fmt::print("  配置: 块大小={}字节, 总数据={}字节\n", chunk_size,
                   total_size);

        // 准备测试数据
        std::vector<memory_stream::value_type> data(total_size);
        for (size_t i = 0; i < total_size; ++i) {
            data[i] = static_cast<memory_stream::value_type>(i % 256);
        }

        // 写入性能测试
        auto start_write = std::chrono::high_resolution_clock::now();
        size_t written = stream.write(data.data(), total_size);
        auto end_write = std::chrono::high_resolution_clock::now();
        auto write_duration =
            std::chrono::duration<double>(end_write - start_write);

        assert(written == total_size);
        fmt::print("  写入: {} 字节, 用时: {:.3f}秒, 速度: {:.2f} MB/s\n",
                   written, write_duration.count(),
                   (total_size / (1024.0 * 1024.0)) / write_duration.count());

        // 读取性能测试
        std::vector<memory_stream::value_type> read_buffer(total_size);

        auto start_read = std::chrono::high_resolution_clock::now();
        size_t total_read = 0;
        const size_t read_chunk_size = 8192;  // 8KB

        while (total_read < total_size) {
            size_t to_read = std::min(read_chunk_size, total_size - total_read);
            size_t read = stream.read(read_buffer.data() + total_read, to_read);
            total_read += read;
        }

        auto end_read = std::chrono::high_resolution_clock::now();
        auto read_duration =
            std::chrono::duration<double>(end_read - start_read);

        assert(total_read == total_size);
        fmt::print("  读取: {} 字节, 用时: {:.3f}秒, 速度: {:.2f} MB/s\n",
                   total_read, read_duration.count(),
                   (total_size / (1024.0 * 1024.0)) / read_duration.count());

        // 验证数据
        assert(std::equal(data.begin(), data.end(), read_buffer.begin()));
        fmt::print("  ✓ 数据完整性验证通过\n");

        // 显示统计信息
        auto stats = stream.get_chunk_stats();
        size_t total_chunks = stats.size();
        double avg_usage = 0;

        for (const auto& stat : stats) {
            avg_usage += stat.usage_percent;
        }
        avg_usage /= total_chunks;

        fmt::print("  块统计: {} 个块, 平均使用率: {:.1f}%\n", total_chunks,
                   avg_usage);

        fmt::print("✅ 测试12 全部通过\n");
        return true;
    } catch (const std::exception& e) {
        fmt::print("❌ 测试12 失败: {}\n", e.what());
        return false;
    }
}

// 测试13: 综合场景测试
bool test_integration_scenario() {
    fmt::print("\n🧪 测试13: 综合场景测试\n");

    try {
        // 模拟一个简单的网络数据包处理场景
        memory_stream network_stream(
            static_cast<size_t>(block_sizes::NetworkPacketSize));

        fmt::print("  场景: 网络数据包处理 (块大小: {} KB)\n",
                   network_stream.chunk_capacity() / 1024);

        // 模拟接收多个数据包
        std::vector<std::string> packets = {
            "PACKET1:Header:Data1:Data2:Data3",
            "PACKET2:Header:Data4:Data5:Data6:Data7",
            "PACKET3:Header:Data8:Data9",
            "PACKET4:Header:Data10:Data11:Data12:Data13:Data14"};

        fmt::print("  接收数据包:\n");
        for (const auto& packet : packets) {
            size_t written = network_stream.write(
                reinterpret_cast<const memory_stream::value_type*>(
                    packet.c_str()),
                packet.size());
            fmt::print("    ✓ 数据包: {} 字节\n", written);
        }

        fmt::print("  当前状态: {} 字节, {} 个块\n", network_stream.size(),
                   network_stream.chunk_count());

        // 模拟协议解析
        const char delimiter = ':';
        std::vector<std::string> parsed_packets;

        while (!network_stream.empty()) {
            std::string current_packet;

            while (true) {
                auto byte_opt = network_stream.read_byte();
                if (!byte_opt) {
                    break;  // 没有数据了
                }

                char ch = static_cast<char>(byte_opt.value());
                if (ch == delimiter) {
                    // 分隔符，一个字段结束
                    if (!current_packet.empty()) {
                        parsed_packets.push_back(current_packet);
                        current_packet.clear();
                    }
                } else {
                    current_packet.push_back(ch);
                }

                // 简单限制，防止无限循环
                if (current_packet.size() > 100) {
                    break;
                }
            }

            if (!current_packet.empty()) {
                parsed_packets.push_back(current_packet);
            }
        }

        fmt::print("  解析结果 ({} 个字段):\n", parsed_packets.size());
        for (size_t i = 0; i < std::min<size_t>(10, parsed_packets.size());
             ++i) {
            fmt::print("    [{:2d}] {}\n", i, parsed_packets[i]);
        }
        if (parsed_packets.size() > 10) {
            fmt::print("    ... 还有 {} 个字段\n", parsed_packets.size() - 10);
        }

        // 验证解析结果
        size_t expected_fields = 0;
        for (const auto& packet : packets) {
            expected_fields +=
                std::count(packet.begin(), packet.end(), ':') + 1;
        }

        assert(parsed_packets.size() == expected_fields);
        fmt::print("  ✓ 字段数量验证通过: {} 个字段\n", parsed_packets.size());

        // 显示最终统计
        fmt::print("  最终状态: {} 字节, {} 个块\n", network_stream.size(),
                   network_stream.chunk_count());

        fmt::print("✅ 测试13 全部通过\n");
        return true;
    } catch (const std::exception& e) {
        fmt::print("❌ 测试13 失败: {}\n", e.what());
        return false;
    }
}

// 主测试函数
void run_all_tests() {
    fmt::print("{:=^60}\n", " memory_stream 测试套件 ");
    fmt::print("开始运行所有测试...\n");

    bool all_passed = true;
    int test_count = 0;
    int passed_count = 0;

    // 定义测试函数列表
    using TestFunc = bool (*)();
    std::vector<std::pair<std::string, TestFunc>> tests = {
        {"基本构造和查询", test_basic_construction},
        {"小数据写入和读取", test_small_data_io},
        {"跨块大数据写入", test_large_data_cross_chunks},
        {"逐字节操作", test_byte_by_byte_operations},
        {"批量填充操作", test_fill_operation},
        {"迭代器测试", test_iterators},
        {"消费操作测试", test_consume_operation},
        {"seek和位置操作", test_seek_and_position},
        {"compact操作测试", test_compact_operation},
        {"trim操作测试", test_trim_operation},
        {"边界条件测试", test_edge_cases},
        {"性能测试", test_performance},
        {"综合场景测试", test_integration_scenario},
    };

    // 运行所有测试
    for (const auto& [test_name, test_func] : tests) {
        ++test_count;
        fmt::print("\n[{}/{}] 运行测试: {}\n", test_count, tests.size(),
                   test_name);

        try {
            if (test_func()) {
                ++passed_count;
                fmt::print("🎉 测试通过: {}\n", test_name);
            } else {
                all_passed = false;
                fmt::print("💥 测试失败: {}\n", test_name);
            }
        } catch (const std::exception& e) {
            all_passed = false;
            fmt::print("💥 测试异常: {} - {}\n", test_name, e.what());
        }
    }

    // 显示测试结果摘要
    fmt::print("\n{:=^60}\n", " 测试结果摘要 ");
    fmt::print("总计: {} 个测试\n", test_count);
    fmt::print("通过: {} 个\n", passed_count);
    fmt::print("失败: {} 个\n", test_count - passed_count);

    if (all_passed) {
        fmt::print("\n🎉🎉🎉 所有测试通过! 🎉🎉🎉\n");
    } else {
        fmt::print("\n❌❌❌ 有测试失败! ❌❌❌\n");
    }
    fmt::print("{:=^60}\n", "");
}
