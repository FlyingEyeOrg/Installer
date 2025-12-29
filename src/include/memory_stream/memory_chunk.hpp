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

    explicit memory_chunk(size_type capacity) {
        capacity_ = capacity;
        // 如果capacity=0，不需要分配内存
        if (capacity != 0) {
            buffer_ = new value_type[capacity];
        }
        clear();
    }

    // 禁止拷贝
    memory_chunk(const memory_chunk&) = delete;
    memory_chunk& operator=(const memory_chunk&) = delete;

    // 移动构造
    memory_chunk(memory_chunk&& other) noexcept
        : buffer_(other.buffer_),
          capacity_(other.capacity_),
          size_(other.size_) {
        other.buffer_ = nullptr;
        other.capacity_ = 0;
        other.size_ = 0;
    }

    // 移动赋值
    memory_chunk& operator=(memory_chunk&& other) noexcept {
        if (this != &other) {
            delete[] buffer_;
            buffer_ = other.buffer_;
            capacity_ = other.capacity_;
            size_ = other.size_;

            other.buffer_ = nullptr;
            other.capacity_ = 0;
            other.size_ = 0;
        }
        return *this;
    }

    // 析构函数
    ~memory_chunk() { delete[] buffer_; }

    // --- CRUD 操作 ---

    // Create: 写入数据，返回实际写入的字节数
    size_type write(const_pointer data, size_type count) {
        if (count == 0 || data == nullptr) {
            return 0;
        }

        // 计算实际可写入的大小
        size_type write_size = std::min(count, capacity_ - size_);
        if (write_size == 0) {
            return 0;
        }

        // 写入数据
        std::memcpy(buffer_ + size_, data, write_size);
        size_ += write_size;

        return write_size;
    }

    // Create: 在指定位置插入数据
    bool insert(size_type pos, const_pointer data, size_type count) {
        if (data == nullptr || pos > size_) {
            return false;
        }

        if (count == 0) {
            return true;
        }

        // 检查是否有足够空间
        if (size_ + count > capacity_) {
            return false;
        }

        // 将位置pos之后的数据向后移动
        if (pos < size_) {
            std::memmove(buffer_ + pos + count, buffer_ + pos, size_ - pos);
        }

        // 插入新数据
        std::memcpy(buffer_ + pos, data, count);
        size_ += count;

        return true;
    }

    // Create: 写入单个字节
    bool write_byte(value_type byte) {
        if (full()) {
            return false;
        }

        buffer_[size_] = byte;
        ++size_;
        return true;
    }

    // Create: 在末尾追加单个字节
    bool push_back(value_type byte) { return write_byte(byte); }

    // Create: 批量写入多个相同字节
    size_type fill(value_type byte, size_type count) {
        if (count == 0) {
            return 0;
        }

        // 计算实际可写入的大小
        size_type write_size = std::min(count, capacity_ - size_);
        if (write_size == 0) {
            return 0;
        }

        // 填充数据
        std::memset(buffer_ + size_, byte, write_size);
        size_ += write_size;

        return write_size;
    }

    // Read: 读取数据但不移除
    size_type peek(pointer buffer, size_type count) const {
        if (count == 0 || buffer == nullptr) {
            return 0;
        }

        count = std::min(count, size_);
        if (count == 0) {
            return 0;
        }

        std::memcpy(buffer, buffer_, count);
        return count;
    }

    // Read: 从指定位置读取数据但不移除
    size_type peek_at(size_type pos, pointer buffer, size_type count) const {
        if (buffer == nullptr || pos >= size_) {
            return 0;
        }

        if (count == 0) {
            return 0;
        }

        // 计算实际可读取的大小
        size_type read_size = std::min(count, size_ - pos);
        if (read_size == 0) {
            return 0;
        }

        std::memcpy(buffer, buffer_ + pos, read_size);
        return read_size;
    }

    // Read: 读取数据并移除
    size_type read(pointer buffer, size_type count) {
        if (buffer == nullptr) {
            return 0;
        }

        size_type bytes_read = peek(buffer, count);
        if (bytes_read > 0) {
            // 移除已读取的数据
            if (bytes_read < size_) {
                // 将剩余数据向前移动
                std::memmove(buffer_, buffer_ + bytes_read, size_ - bytes_read);
            }
            size_ -= bytes_read;
        }
        return bytes_read;
    }

    // Read: 读取单个字节
    std::optional<value_type> read_byte() {
        if (empty()) {
            return std::nullopt;
        }

        value_type byte = buffer_[0];
        // 移除第一个字节
        if (size_ > 1) {
            std::memmove(buffer_, buffer_ + 1, size_ - 1);
        }
        --size_;

        return byte;
    }

    // Read: 读取并移除第一个字节
    std::optional<value_type> pop_front() { return read_byte(); }

    // Read: 读取并移除最后一个字节
    std::optional<value_type> pop_back() {
        if (empty()) {
            return std::nullopt;
        }

        value_type byte = buffer_[size_ - 1];
        --size_;
        return byte;
    }

    // Update: 更新指定位置的数据
    bool update(size_type pos, const_pointer data, size_type count) {
        if (data == nullptr || pos + count > size_) {
            return false;  // 超出有效数据范围
        }

        std::memcpy(buffer_ + pos, data, count);
        return true;
    }

    // Update: 更新单个字节
    bool update_byte(size_type pos, value_type byte) {
        if (pos >= size_) {
            return false;
        }

        buffer_[pos] = byte;
        return true;
    }

    // Delete: 从开头删除数据
    bool consume_front(size_type count) {
        if (count > size_) {
            return false;
        }

        if (count == 0) {
            return true;
        }

        if (count < size_) {
            // 将剩余数据向前移动
            std::memmove(buffer_, buffer_ + count, size_ - count);
        }
        size_ -= count;

        return true;
    }

    // Delete: 从末尾删除数据
    bool consume_back(size_type count) {
        if (count > size_) {
            return false;
        }

        size_ -= count;
        return true;
    }

    // Delete: 删除指定位置的数据
    bool erase(size_type pos, size_type count) {
        if (pos >= size_) {
            return false;
        }

        // 计算实际要删除的数量
        count = std::min(count, size_ - pos);
        if (count == 0) {
            return true;
        }

        // 将后面的数据向前移动
        if (pos + count < size_) {
            std::memmove(buffer_ + pos, buffer_ + pos + count,
                         size_ - (pos + count));
        }
        size_ -= count;

        return true;
    }

    // Delete: 删除指定位置的单个字节
    bool erase_at(size_type pos) { return erase(pos, 1); }

    // Delete: 清空所有数据
    void clear() { size_ = 0; }

    // --- 查询操作 ---

    // 获取指定位置的数据（不检查边界）
    value_type operator[](size_type pos) const {
        if (pos >= size_) {
            throw std::out_of_range("memory_chunk index out of range");
        }
        return buffer_[pos];
    }

    // 获取指定位置的引用（不检查边界）
    reference operator[](size_type pos) {
        if (pos >= size_) {
            throw std::out_of_range("memory_chunk index out of range");
        }
        return buffer_[pos];
    }

    // 安全获取指定位置的数据
    std::optional<value_type> at(size_type pos) const {
        if (pos >= size_) {
            return std::nullopt;
        }
        return buffer_[pos];
    }

    // 获取第一个字节
    std::optional<value_type> front() const {
        if (empty()) {
            return std::nullopt;
        }
        return buffer_[0];
    }

    // 获取最后一个字节
    std::optional<value_type> back() const {
        if (empty()) {
            return std::nullopt;
        }
        return buffer_[size_ - 1];
    }

    // --- 容量查询 ---
    size_type size() const noexcept { return size_; }
    size_type capacity() const noexcept { return capacity_; }
    size_type available_space() const noexcept { return capacity_ - size_; }
    bool empty() const noexcept { return size_ == 0; }
    bool full() const noexcept { return size_ == capacity_; }

    // --- 直接访问内部缓冲区（谨慎使用）---
    const_pointer data() const noexcept { return buffer_; }
    pointer data() noexcept { return buffer_; }

    const_pointer begin() const noexcept { return buffer_; }
    pointer begin() noexcept { return buffer_; }

    const_pointer end() const noexcept { return buffer_ + size_; }
    pointer end() noexcept { return buffer_ + size_; }

    // 获取可连续写入的起始指针和长度
    std::pair<pointer, size_type> contiguous_write() const noexcept {
        if (full()) {
            return {nullptr, 0};
        }
        return {buffer_ + size_, capacity_ - size_};
    }

    // --- 辅助功能 ---

    // 重置容量（会丢失所有数据）
    bool resize(size_type new_capacity) {
        if (new_capacity < size_) {
            return false;  // 新容量不能小于当前数据大小
        }

        pointer new_buffer = new value_type[new_capacity];

        // 复制现有数据到新缓冲区
        if (size_ > 0) {
            std::memcpy(new_buffer, buffer_, size_);
        }

        delete[] buffer_;
        buffer_ = new_buffer;
        capacity_ = new_capacity;

        return true;
    }

    // 交换两个 memory_chunk
    void swap(memory_chunk& other) noexcept {
        // 交换所有成员变量
        pointer temp_buffer = buffer_;
        buffer_ = other.buffer_;
        other.buffer_ = temp_buffer;

        size_type temp = capacity_;
        capacity_ = other.capacity_;
        other.capacity_ = temp;

        temp = size_;
        size_ = other.size_;
        other.size_ = temp;
    }

    // 从另一个 memory_chunk 复制数据
    size_type copy_from(const memory_chunk& other) {
        if (this == &other || other.empty()) {
            return 0;
        }

        // 获取要复制的数据大小
        size_type to_copy = std::min(other.size(), available_space());
        if (to_copy == 0) {
            return 0;
        }

        std::memcpy(buffer_ + size_, other.buffer_, to_copy);
        size_ += to_copy;

        return to_copy;
    }

    // 从指定位置开始复制数据
    size_type copy_from(const memory_chunk& other, size_type src_pos,
                        size_type count) {
        if (this == &other || src_pos >= other.size()) {
            return 0;
        }

        // 计算实际可复制的数量
        size_type available = other.size() - src_pos;
        size_type to_copy =
            std::min(std::min(count, available), available_space());
        if (to_copy == 0) {
            return 0;
        }

        std::memcpy(buffer_ + size_, other.buffer_ + src_pos, to_copy);
        size_ += to_copy;

        return to_copy;
    }

    // 从指定位置开始查找字节
    std::optional<size_type> find(value_type byte,
                                  size_type start_pos = 0) const {
        if (start_pos >= size_) {
            return std::nullopt;
        }

        for (size_type i = start_pos; i < size_; ++i) {
            if (buffer_[i] == byte) {
                return i;
            }
        }

        return std::nullopt;
    }

    // 比较两个 memory_chunk 的内容是否相同
    bool equals(const memory_chunk& other) const {
        if (this == &other) {
            return true;
        }

        if (size_ != other.size_) {
            return false;
        }

        if (size_ == 0) {
            return true;
        }

        return std::memcmp(buffer_, other.buffer_, size_) == 0;
    }

    // 比较与原始数据的相等性
    bool equals(const_pointer data, size_type count) const {
        if (data == nullptr) {
            return size_ == 0;
        }

        if (size_ != count) {
            return false;
        }

        if (size_ == 0) {
            return true;
        }

        return std::memcmp(buffer_, data, size_) == 0;
    }

    // 获取剩余连续空间
    size_type contiguous_space() const noexcept { return capacity_ - size_; }

    // 将数据移动到开头，释放末尾空间
    void compact() {
        // 数据已经在开头，无需移动
        // 这个方法主要是为了接口一致性
    }
};

// 全局 swap 函数
inline void swap(memory_chunk& a, memory_chunk& b) noexcept { a.swap(b); }