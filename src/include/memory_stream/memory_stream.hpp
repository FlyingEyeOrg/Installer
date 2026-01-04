#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "block_sizes.hpp"
#include "memory_chunk.hpp"

class memory_stream {
   public:
    using value_type = unsigned char;
    using size_type = std::size_t;
    using chunk_type = memory_chunk;
    using chunk_size_type = block_sizes;

    // 构造函数
    explicit memory_stream(
        chunk_size_type chunk_size = block_sizes::DefaultChunkSize);
    explicit memory_stream(size_type chunk_capacity);

    // 禁止拷贝
    memory_stream(const memory_stream&) = delete;
    memory_stream& operator=(const memory_stream&) = delete;

    // 移动构造
    memory_stream(memory_stream&& other) noexcept;

    // 移动赋值
    memory_stream& operator=(memory_stream&& other) noexcept;

    // --- 核心操作 ---

    // 写入数据（末尾追加）
    size_type write(const value_type* data, size_type count);

    // 写入单个字节
    bool write_byte(value_type byte);

    // 批量写入相同字节
    size_type fill(value_type byte, size_type count);

    // --- 读取操作（不改变数据，只移动读取指针）---

    // 从当前位置读取数据
    size_type read(value_type* buffer, size_type count);

    // 读取单个字节
    std::optional<value_type> read_byte();

    // 从指定位置读取数据（不改变读取指针）
    size_type peek(size_type pos, value_type* buffer, size_type count) const;

    // peek单个字节（不改变读取指针）
    std::optional<value_type> peek_byte(size_type pos) const;

    // 设置读取位置
    bool seek(size_type new_pos);

    // 获取当前读取位置
    size_type tell() const noexcept;

    // 将读取位置重置到开头
    void rewind() noexcept;

    // 清除所有数据
    void clear() noexcept;

    // --- 查询操作 ---

    // 获取指定位置的字节
    std::optional<value_type> at(size_type pos) const;

    // 获取第一个字节
    std::optional<value_type> front() const;

    // 获取最后一个字节
    std::optional<value_type> back() const;

    // 是否可读取更多数据
    bool can_read() const noexcept;

    // 剩余可读取字节数
    size_type readable_bytes() const noexcept;

    // 是否到达末尾
    bool eof() const noexcept;

    // --- 容量查询 ---

    size_type size() const noexcept;
    bool empty() const noexcept;
    size_type chunk_capacity() const noexcept;
    size_type chunk_count() const noexcept;

    // --- 高级功能 ---

    // 拷贝数据到vector
    std::vector<value_type> copy_to_vector() const;

    // 从当前位置拷贝指定数量的数据到vector
    std::vector<value_type> copy_from_current(size_type count) const;

    // 查找字节
    std::optional<size_type> find(value_type byte,
                                  size_type start_pos = 0) const;

    // 查找字节（从当前位置开始）
    std::optional<size_type> find_from_current(value_type byte) const;

    // 比较两个stream的内容
    bool equals(const memory_stream& other) const;

    // 跳过指定字节数
    bool skip(size_type count);

    // 跳过字节直到找到特定字节
    std::optional<size_type> skip_until(value_type byte);

   private:
    // 链表节点结构
    struct chunk_node {
        chunk_type chunk;
        std::shared_ptr<chunk_node> next = nullptr;

        explicit chunk_node(size_type chunk_size);

        // chunk在整个流中的起始位置
        size_type global_start = 0;

        // 设置全局起始位置
        void set_global_start(size_type start) { global_start = start; }
    };

    std::shared_ptr<chunk_node> head_ = nullptr;
    std::shared_ptr<chunk_node> tail_ = nullptr;
    size_type total_size_ = 0;      // 总数据大小
    size_type read_pos_ = 0;        // 当前读取位置
    size_type chunk_capacity_ = 0;  // 每个chunk的容量
    size_type chunk_count_ = 0;     // chunk数量

    // 当前读取位置所在的chunk（缓存以提高性能）
    std::shared_ptr<chunk_node> read_chunk_ = nullptr;
    size_type read_offset_in_chunk_ = 0;  // 在当前chunk中的偏移量

    // 创建新的chunk节点
    void create_new_chunk();

    // 查找包含指定位置的chunk及其偏移量
    std::pair<std::shared_ptr<chunk_node>, size_type> find_chunk_with_offset(
        size_type pos) const;

    // 更新读取位置缓存
    void update_read_cache();

    // 从指定chunk和偏移量开始读取
    size_type read_from_chunk(std::shared_ptr<chunk_node> start_chunk,
                              size_type offset_in_chunk, value_type* buffer,
                              size_type count) const;
};