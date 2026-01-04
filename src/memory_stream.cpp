#include "memory_stream/memory_stream.hpp"

#include <algorithm>
#include <cstring>

// --- chunk_node 实现 ---
memory_stream::chunk_node::chunk_node(size_type chunk_size)
    : chunk(chunk_size) {}

// --- memory_stream 实现 ---

// 构造函数
memory_stream::memory_stream(chunk_size_type chunk_size)
    : chunk_capacity_(static_cast<size_type>(chunk_size)) {}

memory_stream::memory_stream(size_type chunk_capacity)
    : chunk_capacity_(chunk_capacity) {}

// 移动构造
memory_stream::memory_stream(memory_stream&& other) noexcept
    : head_(std::move(other.head_)),
      tail_(std::move(other.tail_)),
      total_size_(other.total_size_),
      read_pos_(other.read_pos_),
      chunk_capacity_(other.chunk_capacity_),
      chunk_count_(other.chunk_count_),
      read_chunk_(std::move(other.read_chunk_)),
      read_offset_in_chunk_(other.read_offset_in_chunk_) {
    other.total_size_ = 0;
    other.read_pos_ = 0;
    other.chunk_capacity_ = 0;
    other.chunk_count_ = 0;
    other.read_offset_in_chunk_ = 0;
}

// 移动赋值
memory_stream& memory_stream::operator=(memory_stream&& other) noexcept {
    if (this != &other) {
        head_ = std::move(other.head_);
        tail_ = std::move(other.tail_);
        total_size_ = other.total_size_;
        read_pos_ = other.read_pos_;
        chunk_capacity_ = other.chunk_capacity_;
        chunk_count_ = other.chunk_count_;
        read_chunk_ = std::move(other.read_chunk_);
        read_offset_in_chunk_ = other.read_offset_in_chunk_;

        other.total_size_ = 0;
        other.read_pos_ = 0;
        other.chunk_capacity_ = 0;
        other.chunk_count_ = 0;
        other.read_offset_in_chunk_ = 0;
    }
    return *this;
}

// --- 私有辅助方法 ---

// 创建新的chunk节点
void memory_stream::create_new_chunk() {
    auto new_node = std::make_shared<chunk_node>(chunk_capacity_);

    if (tail_ == nullptr) {
        // 第一个节点
        head_ = new_node;
        tail_ = new_node;
        new_node->set_global_start(0);
    } else {
        // 添加到链表末尾
        new_node->set_global_start(total_size_);
        tail_->next = new_node;
        tail_ = new_node;
    }

    chunk_count_++;
}

// 查找包含指定位置的chunk及其偏移量
std::pair<std::shared_ptr<memory_stream::chunk_node>, memory_stream::size_type>
memory_stream::find_chunk_with_offset(size_type pos) const {
    if (pos >= total_size_ || head_ == nullptr) {
        return {nullptr, 0};
    }

    // 如果读取缓存有效，先检查缓存
    if (read_chunk_ && pos >= read_chunk_->global_start) {
        size_type chunk_end =
            read_chunk_->global_start + read_chunk_->chunk.size();
        if (pos < chunk_end) {
            return {read_chunk_, pos - read_chunk_->global_start};
        }

        // 检查下一个chunk
        if (read_chunk_->next && pos >= read_chunk_->next->global_start) {
            auto next = read_chunk_->next;
            size_type next_end = next->global_start + next->chunk.size();
            if (pos < next_end) {
                return {next, pos - next->global_start};
            }
        }
    }

    // 从头开始遍历
    std::shared_ptr<chunk_node> current = head_;
    while (current != nullptr) {
        size_type chunk_size = current->chunk.size();
        if (pos < current->global_start + chunk_size) {
            return {current, pos - current->global_start};
        }
        current = current->next;
    }

    return {nullptr, 0};
}

// 更新读取位置缓存
void memory_stream::update_read_cache() {
    if (read_pos_ >= total_size_) {
        read_chunk_ = nullptr;
        read_offset_in_chunk_ = 0;
        return;
    }

    auto [chunk, offset] = find_chunk_with_offset(read_pos_);
    read_chunk_ = chunk;
    read_offset_in_chunk_ = offset;
}

// 从指定chunk和偏移量开始读取
memory_stream::size_type memory_stream::read_from_chunk(
    std::shared_ptr<chunk_node> start_chunk, size_type offset_in_chunk,
    value_type* buffer, size_type count) const {
    if (start_chunk == nullptr || buffer == nullptr || count == 0) {
        return 0;
    }

    size_type read_total = 0;
    value_type* current_buffer = buffer;
    size_type remaining = count;
    std::shared_ptr<chunk_node> current = start_chunk;
    size_type offset = offset_in_chunk;

    while (remaining > 0 && current != nullptr) {
        size_type chunk_size = current->chunk.size();
        if (offset >= chunk_size) {
            // 应该不会发生，但安全起见
            break;
        }

        size_type chunk_available = chunk_size - offset;
        size_type to_read_this_chunk =
            (remaining < chunk_available) ? remaining : chunk_available;

        if (to_read_this_chunk > 0) {
            size_type actually_read = current->chunk.peek_at(
                offset, current_buffer, to_read_this_chunk);

            if (actually_read > 0) {
                read_total += actually_read;
                remaining -= actually_read;
                current_buffer += actually_read;
                offset = 0;  // 后续chunk从0开始读取
                current = current->next;
            } else {
                break;
            }
        } else {
            break;
        }
    }

    return read_total;
}

// --- 写入操作 ---

// 写入数据
memory_stream::size_type memory_stream::write(const value_type* data,
                                              size_type count) {
    if (count == 0 || data == nullptr) {
        return 0;
    }

    size_type written = 0;
    const value_type* current_data = data;
    size_type remaining = count;

    while (remaining > 0) {
        if (tail_ == nullptr || tail_->chunk.full()) {
            create_new_chunk();
        }

        size_type can_write = tail_->chunk.available_space();
        if (can_write == 0) {
            create_new_chunk();
            can_write = tail_->chunk.available_space();
        }

        size_type to_write = (can_write < remaining) ? can_write : remaining;
        size_type actually_written = tail_->chunk.write(current_data, to_write);

        if (actually_written > 0) {
            written += actually_written;
            remaining -= actually_written;
            current_data += actually_written;
            total_size_ += actually_written;
        } else {
            break;
        }
    }

    return written;
}

// 写入单个字节
bool memory_stream::write_byte(value_type byte) {
    if (tail_ == nullptr || tail_->chunk.full()) {
        create_new_chunk();
    }

    if (tail_->chunk.write_byte(byte)) {
        total_size_++;
        return true;
    }

    return false;
}

// 批量写入相同字节
memory_stream::size_type memory_stream::fill(value_type byte, size_type count) {
    if (count == 0) {
        return 0;
    }

    size_type written = 0;
    size_type remaining = count;

    while (remaining > 0) {
        if (tail_ == nullptr || tail_->chunk.full()) {
            create_new_chunk();
        }

        size_type can_write = tail_->chunk.available_space();
        if (can_write == 0) {
            create_new_chunk();
            can_write = tail_->chunk.available_space();
        }

        size_type to_write = (can_write < remaining) ? can_write : remaining;
        size_type actually_written = tail_->chunk.fill(byte, to_write);

        if (actually_written > 0) {
            written += actually_written;
            remaining -= actually_written;
            total_size_ += actually_written;
        } else {
            break;
        }
    }

    return written;
}

// --- 读取操作 ---

// 从当前位置读取数据
memory_stream::size_type memory_stream::read(value_type* buffer,
                                             size_type count) {
    if (buffer == nullptr || count == 0 || read_pos_ >= total_size_) {
        return 0;
    }

    // 获取当前位置的chunk和偏移
    auto [chunk, offset] = find_chunk_with_offset(read_pos_);
    if (chunk == nullptr) {
        return 0;
    }

    // 读取数据
    size_type bytes_read = read_from_chunk(chunk, offset, buffer, count);

    // 移动读取位置
    if (bytes_read > 0) {
        read_pos_ += bytes_read;
        update_read_cache();
    }

    return bytes_read;
}

// 读取单个字节
std::optional<memory_stream::value_type> memory_stream::read_byte() {
    if (read_pos_ >= total_size_) {
        return std::nullopt;
    }

    auto [chunk, offset] = find_chunk_with_offset(read_pos_);
    if (chunk == nullptr) {
        return std::nullopt;
    }

    auto byte = chunk->chunk.at(offset);
    if (byte) {
        read_pos_++;
        update_read_cache();
    }

    return byte;
}

// 从指定位置peek数据
memory_stream::size_type memory_stream::peek(size_type pos, value_type* buffer,
                                             size_type count) const {
    if (pos >= total_size_ || buffer == nullptr || count == 0) {
        return 0;
    }

    size_type available = total_size_ - pos;
    size_type to_read = (count < available) ? count : available;

    if (to_read == 0) {
        return 0;
    }

    auto [chunk, offset] = find_chunk_with_offset(pos);
    if (chunk == nullptr) {
        return 0;
    }

    return read_from_chunk(chunk, offset, buffer, to_read);
}

// peek单个字节
std::optional<memory_stream::value_type> memory_stream::peek_byte(
    size_type pos) const {
    if (pos >= total_size_) {
        return std::nullopt;
    }

    auto [chunk, offset] = find_chunk_with_offset(pos);
    if (!chunk) {
        return std::nullopt;
    }

    return chunk->chunk.at(offset);
}

// 设置读取位置
bool memory_stream::seek(size_type new_pos) {
    if (new_pos > total_size_) {
        return false;
    }

    if (new_pos != read_pos_) {
        read_pos_ = new_pos;
        update_read_cache();
    }

    return true;
}

// 获取当前读取位置
memory_stream::size_type memory_stream::tell() const noexcept {
    return read_pos_;
}

// 将读取位置重置到开头
void memory_stream::rewind() noexcept {
    read_pos_ = 0;
    if (head_ != nullptr) {
        read_chunk_ = head_;
        read_offset_in_chunk_ = 0;
    }
}

// 清除所有数据
void memory_stream::clear() noexcept {
    head_ = nullptr;
    tail_ = nullptr;
    read_chunk_ = nullptr;
    total_size_ = 0;
    read_pos_ = 0;
    chunk_count_ = 0;
    read_offset_in_chunk_ = 0;
}

// --- 查询操作 ---

// 获取指定位置的字节
std::optional<memory_stream::value_type> memory_stream::at(
    size_type pos) const {
    return peek_byte(pos);
}

// 获取第一个字节
std::optional<memory_stream::value_type> memory_stream::front() const {
    if (head_ == nullptr || head_->chunk.empty()) {
        return std::nullopt;
    }
    return head_->chunk.front();
}

// 获取最后一个字节
std::optional<memory_stream::value_type> memory_stream::back() const {
    if (tail_ == nullptr || tail_->chunk.empty()) {
        return std::nullopt;
    }
    return tail_->chunk.back();
}

// 是否可读取更多数据
bool memory_stream::can_read() const noexcept {
    return read_pos_ < total_size_;
}

// 剩余可读取字节数
memory_stream::size_type memory_stream::readable_bytes() const noexcept {
    return (read_pos_ < total_size_) ? (total_size_ - read_pos_) : 0;
}

// 是否到达末尾
bool memory_stream::eof() const noexcept { return read_pos_ >= total_size_; }

// --- 容量查询 ---
memory_stream::size_type memory_stream::size() const noexcept {
    return total_size_;
}

bool memory_stream::empty() const noexcept { return total_size_ == 0; }

memory_stream::size_type memory_stream::chunk_capacity() const noexcept {
    return chunk_capacity_;
}

memory_stream::size_type memory_stream::chunk_count() const noexcept {
    return chunk_count_;
}

// --- 高级功能 ---

// 拷贝数据到vector
std::vector<memory_stream::value_type> memory_stream::copy_to_vector() const {
    std::vector<value_type> result;
    result.reserve(total_size_);

    value_type temp[1024];
    std::shared_ptr<chunk_node> current = head_;

    while (current != nullptr) {
        size_type chunk_size = current->chunk.size();
        size_type offset = 0;

        while (offset < chunk_size) {
            size_type to_read =
                (chunk_size - offset < 1024) ? (chunk_size - offset) : 1024;

            size_type read = current->chunk.peek_at(offset, temp, to_read);

            if (read > 0) {
                result.insert(result.end(), temp, temp + read);
                offset += read;
            } else {
                break;
            }
        }

        current = current->next;
    }

    return result;
}

// 从当前位置拷贝指定数量的数据到vector
std::vector<memory_stream::value_type> memory_stream::copy_from_current(
    size_type count) const {
    std::vector<value_type> result;

    if (read_pos_ >= total_size_) {
        return result;
    }

    size_type available = total_size_ - read_pos_;
    size_type to_copy = (count < available) ? count : available;

    if (to_copy == 0) {
        return result;
    }

    result.resize(to_copy);
    size_type copied = peek(read_pos_, result.data(), to_copy);
    result.resize(copied);

    return result;
}

// 查找字节
std::optional<memory_stream::size_type> memory_stream::find(
    value_type byte, size_type start_pos) const {
    if (start_pos >= total_size_) {
        return std::nullopt;
    }

    auto [current, offset] = find_chunk_with_offset(start_pos);
    if (!current) {
        return std::nullopt;
    }

    while (current != nullptr) {
        std::optional<size_type> pos_in_chunk =
            current->chunk.find(byte, offset);

        if (pos_in_chunk) {
            return current->global_start + *pos_in_chunk;
        }

        offset = 0;
        current = current->next;
    }

    return std::nullopt;
}

// 查找字节（从当前位置开始）
std::optional<memory_stream::size_type> memory_stream::find_from_current(
    value_type byte) const {
    return find(byte, read_pos_);
}

// 比较两个stream的内容
bool memory_stream::equals(const memory_stream& other) const {
    if (this == &other) {
        return true;
    }

    if (total_size_ != other.total_size_) {
        return false;
    }

    if (total_size_ == 0) {
        return true;
    }

    std::shared_ptr<chunk_node> curr1 = head_;
    std::shared_ptr<chunk_node> curr2 = other.head_;

    while (curr1 != nullptr && curr2 != nullptr) {
        if (!curr1->chunk.equals(curr2->chunk)) {
            return false;
        }
        curr1 = curr1->next;
        curr2 = curr2->next;
    }

    return curr1 == nullptr && curr2 == nullptr;
}

// 跳过指定字节数
bool memory_stream::skip(size_type count) {
    if (read_pos_ + count > total_size_) {
        return false;
    }

    read_pos_ += count;
    update_read_cache();
    return true;
}

// 跳过字节直到找到特定字节
std::optional<memory_stream::size_type> memory_stream::skip_until(
    value_type byte) {
    auto pos = find_from_current(byte);
    if (pos) {
        read_pos_ = *pos;
        update_read_cache();
    }
    return pos;
}