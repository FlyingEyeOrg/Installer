#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "block_sizes.hpp"
#include "memory_chunk.hpp"

class memory_stream {
   public:
    using value_type = memory_chunk::value_type;
    using size_type = std::size_t;
    using chunk_size_type = block_sizes;

   private:
    // 链表节点结构
    struct chunk_node {
        memory_chunk chunk;                // 内存块
        std::unique_ptr<chunk_node> next;  // 下一个节点

        // 构造函数
        explicit chunk_node(size_type capacity);
    };

    // 迭代器
    struct iterator {
        chunk_node* node = nullptr;
        size_type pos_in_node = 0;

        bool operator==(const iterator& other) const;
        bool operator!=(const iterator& other) const;
        iterator& operator++();
        iterator operator++(int);
        value_type operator*() const;
    };

    struct const_iterator {
        const chunk_node* node = nullptr;
        size_type pos_in_node = 0;

        bool operator==(const const_iterator& other) const;
        bool operator!=(const const_iterator& other) const;
        const_iterator& operator++();
        const_iterator operator++(int);
        value_type operator*() const;
    };

   private:
    std::unique_ptr<chunk_node> head_;         // 链表头
    chunk_node* tail_ = nullptr;               // 链表尾（用于快速追加）
    chunk_node* current_read_node_ = nullptr;  // 当前读取节点
    size_type current_read_pos_ = 0;           // 当前读取位置
    size_type total_size_ = 0;                 // 总数据大小
    size_type chunk_capacity_;                 // 每个块的固定容量

   public:
    // 构造函数
    explicit memory_stream(size_type chunk_capacity = static_cast<size_type>(
                               block_sizes::DefaultChunkSize));

    explicit memory_stream(chunk_size_type chunk_size);

    // 禁止拷贝
    memory_stream(const memory_stream&) = delete;
    memory_stream& operator=(const memory_stream&) = delete;

    // 移动构造
    memory_stream(memory_stream&& other) noexcept;

    // 移动赋值
    memory_stream& operator=(memory_stream&& other) noexcept;

    // 析构函数
    ~memory_stream();

    // --- 主要操作 ---

    // 写入数据到流末尾
    size_type write(const value_type* data, size_type count);

    // 写入单个字节
    bool write_byte(value_type byte);

    // 批量写入相同字节
    size_type fill(value_type byte, size_type count);

    // 从流中读取数据
    size_type read(value_type* buffer, size_type count);

    // 读取数据但不移除
    size_type peek(value_type* buffer, size_type count) const;

    // 从当前位置读取单个字节
    std::optional<value_type> read_byte();

    // 读取并移除数据
    size_type consume(value_type* buffer, size_type count);

    // 重置读取位置
    void reset_read_position();

    // 跳转到指定位置
    bool seek_read_position(size_type pos);

    // --- 查询操作 ---

    // 获取总大小
    size_type size() const noexcept;

    // 获取块数
    size_type chunk_count() const noexcept;

    // 检查是否为空
    bool empty() const noexcept;

    // 获取块容量
    size_type chunk_capacity() const noexcept;

    // --- 清理操作 ---

    // 清空流
    void clear();

    // 清除已读取的数据
    void trim();

    // --- 迭代器支持 ---

    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;

    // --- 高级操作 ---

    // 获取所有数据到一个连续缓冲区
    std::vector<value_type> to_vector() const;

    // 合并所有块（压缩）
    void compact();

    // 获取块使用统计
    struct chunk_stats {
        size_type chunk_index;
        size_type data_size;
        size_type capacity;
        double usage_percent;
    };

    std::vector<chunk_stats> get_chunk_stats() const;

   private:
    // 确保有可写的块
    bool ensure_write_block();

    // 添加新块
    void add_new_chunk();

    // 移除第一个块
    void remove_front_chunk();
};