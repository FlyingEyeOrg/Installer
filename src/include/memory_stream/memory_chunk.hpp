#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>
#include <optional>
#include <stdexcept>

class memory_chunk {
   public:
    using value_type = unsigned char;
    using size_type = std::size_t;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;

   private:
    pointer buffer_ = nullptr;
    size_type capacity_ = 0;  // 固定容量
    size_type size_ = 0;      // 当前有效数据大小

   public:
    // 构造函数
    memory_chunk() noexcept = default;

    explicit memory_chunk(size_type capacity);

    // 禁止拷贝
    memory_chunk(const memory_chunk&) = delete;
    memory_chunk& operator=(const memory_chunk&) = delete;

    // 移动构造
    memory_chunk(memory_chunk&& other) noexcept;

    // 移动赋值
    memory_chunk& operator=(memory_chunk&& other) noexcept;

    // 析构函数
    ~memory_chunk();

    // --- CRUD 操作 ---

    // Create: 写入数据，返回实际写入的字节数
    size_type write(const_pointer data, size_type count);

    // Create: 在指定位置插入数据
    bool insert(size_type pos, const_pointer data, size_type count);

    // Create: 写入单个字节
    bool write_byte(value_type byte);

    // Create: 在末尾追加单个字节
    bool push_back(value_type byte);

    // Create: 批量写入多个相同字节
    size_type fill(value_type byte, size_type count);

    // Read: 读取数据但不移除
    size_type peek(pointer buffer, size_type count) const;

    // Read: 从指定位置读取数据但不移除
    size_type peek_at(size_type pos, pointer buffer, size_type count) const;

    // Read: 读取数据并移除
    size_type read(pointer buffer, size_type count);

    // Read: 读取单个字节
    std::optional<value_type> read_byte();

    // Read: 读取并移除第一个字节
    std::optional<value_type> pop_front();

    // Read: 读取并移除最后一个字节
    std::optional<value_type> pop_back();

    // Update: 更新指定位置的数据
    bool update(size_type pos, const_pointer data, size_type count);

    // Update: 更新单个字节
    bool update_byte(size_type pos, value_type byte);

    // Delete: 从开头删除数据
    bool consume_front(size_type count);

    // Delete: 从末尾删除数据
    bool consume_back(size_type count);

    // Delete: 删除指定位置的数据
    bool erase(size_type pos, size_type count);

    // Delete: 删除指定位置的单个字节
    bool erase_at(size_type pos);

    // Delete: 清空所有数据
    void clear();

    // --- 查询操作 ---

    // 获取指定位置的数据（不检查边界）
    value_type operator[](size_type pos) const;

    // 获取指定位置的引用（不检查边界）
    reference operator[](size_type pos);

    // 安全获取指定位置的数据
    std::optional<value_type> at(size_type pos) const;

    // 获取第一个字节
    std::optional<value_type> front() const;

    // 获取最后一个字节
    std::optional<value_type> back() const;

    // --- 容量查询 ---
    size_type size() const noexcept;
    size_type capacity() const noexcept;
    size_type available_space() const noexcept;
    bool empty() const noexcept;
    bool full() const noexcept;

    // --- 直接访问内部缓冲区（谨慎使用）---
    const_pointer data() const noexcept;
    pointer data() noexcept;

    const_pointer begin() const noexcept;
    pointer begin() noexcept;

    const_pointer end() const noexcept;
    pointer end() noexcept;

    // 获取可连续写入的起始指针和长度
    std::pair<pointer, size_type> contiguous_write() const noexcept;

    // --- 辅助功能 ---

    // 重置容量（会丢失所有数据）
    bool resize(size_type new_capacity);

    // 交换两个 memory_chunk
    void swap(memory_chunk& other) noexcept;

    // 从另一个 memory_chunk 复制数据
    size_type copy_from(const memory_chunk& other);

    // 从指定位置开始复制数据
    size_type copy_from(const memory_chunk& other, size_type src_pos,
                        size_type count);

    // 从指定位置开始查找字节
    std::optional<size_type> find(value_type byte,
                                  size_type start_pos = 0) const;

    // 比较两个 memory_chunk 的内容是否相同
    bool equals(const memory_chunk& other) const;

    // 比较与原始数据的相等性
    bool equals(const_pointer data, size_type count) const;

    // 获取剩余连续空间
    size_type contiguous_space() const noexcept;

    // 将数据移动到开头，释放末尾空间
    void compact();
};

// 全局 swap 函数
inline void swap(memory_chunk& a, memory_chunk& b) noexcept;