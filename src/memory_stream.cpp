#include "memory_stream/memory_stream.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

// chunk_node 构造函数实现
memory_stream::chunk_node::chunk_node(size_type capacity)
    : chunk(capacity), next(nullptr) {}

// --- iterator 实现 ---
bool memory_stream::iterator::operator==(const iterator& other) const {
    return node == other.node && pos_in_node == other.pos_in_node;
}

bool memory_stream::iterator::operator!=(const iterator& other) const {
    return !(*this == other);
}

memory_stream::iterator& memory_stream::iterator::operator++() {
    if (node) {
        ++pos_in_node;
        if (pos_in_node >= node->chunk.size()) {
            node = node->next.get();
            pos_in_node = 0;
        }
    }
    return *this;
}

memory_stream::iterator memory_stream::iterator::operator++(int) {
    iterator temp = *this;
    ++(*this);
    return temp;
}

memory_stream::value_type memory_stream::iterator::operator*() const {
    if (!node || pos_in_node >= node->chunk.size()) {
        throw std::out_of_range("Invalid iterator dereference");
    }
    return node->chunk[pos_in_node];
}

// --- const_iterator 实现 ---
bool memory_stream::const_iterator::operator==(
    const const_iterator& other) const {
    return node == other.node && pos_in_node == other.pos_in_node;
}

bool memory_stream::const_iterator::operator!=(
    const const_iterator& other) const {
    return !(*this == other);
}

memory_stream::const_iterator& memory_stream::const_iterator::operator++() {
    if (node) {
        ++pos_in_node;
        if (pos_in_node >= node->chunk.size()) {
            node = node->next.get();
            pos_in_node = 0;
        }
    }
    return *this;
}

memory_stream::const_iterator memory_stream::const_iterator::operator++(int) {
    const_iterator temp = *this;
    ++(*this);
    return temp;
}

memory_stream::value_type memory_stream::const_iterator::operator*() const {
    if (!node || pos_in_node >= node->chunk.size()) {
        throw std::out_of_range("Invalid iterator dereference");
    }
    return node->chunk[pos_in_node];
}

// --- memory_stream 构造函数 ---
memory_stream::memory_stream(size_type chunk_capacity)
    : chunk_capacity_(chunk_capacity) {}

memory_stream::memory_stream(chunk_size_type chunk_size)
    : chunk_capacity_(static_cast<size_type>(chunk_size)) {}

// 移动构造
memory_stream::memory_stream(memory_stream&& other) noexcept
    : head_(std::move(other.head_)),
      tail_(other.tail_),
      current_read_node_(other.current_read_node_),
      current_read_pos_(other.current_read_pos_),
      total_size_(other.total_size_),
      chunk_capacity_(other.chunk_capacity_) {
    other.tail_ = nullptr;
    other.current_read_node_ = nullptr;
    other.current_read_pos_ = 0;
    other.total_size_ = 0;
}

// 移动赋值
memory_stream& memory_stream::operator=(memory_stream&& other) noexcept {
    if (this != &other) {
        clear();

        head_ = std::move(other.head_);
        tail_ = other.tail_;
        current_read_node_ = other.current_read_node_;
        current_read_pos_ = other.current_read_pos_;
        total_size_ = other.total_size_;
        chunk_capacity_ = other.chunk_capacity_;

        other.tail_ = nullptr;
        other.current_read_node_ = nullptr;
        other.current_read_pos_ = 0;
        other.total_size_ = 0;
    }
    return *this;
}

// 析构函数
memory_stream::~memory_stream() = default;

// --- 主要操作 ---
memory_stream::size_type memory_stream::write(const value_type* data,
                                              size_type count) {
    if (!data || count == 0) {
        return 0;
    }

    size_type bytes_written = 0;
    const value_type* src = data;

    while (bytes_written < count) {
        if (!ensure_write_block()) {
            break;
        }

        size_type write_size =
            tail_->chunk.write(src + bytes_written, count - bytes_written);
        bytes_written += write_size;
        total_size_ += write_size;

        if (tail_->chunk.full() && bytes_written < count) {
            add_new_chunk();
        }
    }

    return bytes_written;
}

bool memory_stream::write_byte(value_type byte) {
    if (!ensure_write_block()) {
        return false;
    }

    if (tail_->chunk.write_byte(byte)) {
        ++total_size_;
        return true;
    }
    return false;
}

memory_stream::size_type memory_stream::fill(value_type byte, size_type count) {
    if (count == 0) {
        return 0;
    }

    size_type bytes_written = 0;

    while (bytes_written < count) {
        if (!ensure_write_block()) {
            break;
        }

        size_type write_size = tail_->chunk.fill(byte, count - bytes_written);
        bytes_written += write_size;
        total_size_ += write_size;

        if (tail_->chunk.full() && bytes_written < count) {
            add_new_chunk();
        }
    }

    return bytes_written;
}

memory_stream::size_type memory_stream::read(value_type* buffer,
                                             size_type count) {
    if (!buffer || count == 0 || empty()) {
        return 0;
    }

    size_type bytes_read = 0;
    value_type* dst = buffer;

    if (!current_read_node_) {
        current_read_node_ = head_.get();
        current_read_pos_ = 0;
    }

    while (bytes_read < count && current_read_node_) {
        size_type available_in_node =
            current_read_node_->chunk.size() - current_read_pos_;
        if (available_in_node == 0) {
            current_read_node_ = current_read_node_->next.get();
            current_read_pos_ = 0;
            continue;
        }

        size_type to_read = std::min(count - bytes_read, available_in_node);
        size_type actually_read = current_read_node_->chunk.peek_at(
            current_read_pos_, dst + bytes_read, to_read);

        bytes_read += actually_read;
        current_read_pos_ += actually_read;

        if (current_read_pos_ >= current_read_node_->chunk.size()) {
            current_read_node_ = current_read_node_->next.get();
            current_read_pos_ = 0;
        }
    }

    return bytes_read;
}

memory_stream::size_type memory_stream::peek(value_type* buffer,
                                             size_type count) const {
    if (!buffer || count == 0 || empty()) {
        return 0;
    }

    chunk_node* node = head_.get();
    size_type pos = 0;
    size_type bytes_peeked = 0;

    while (bytes_peeked < count && node) {
        size_type available_in_node = node->chunk.size() - pos;
        if (available_in_node == 0) {
            node = node->next.get();
            pos = 0;
            continue;
        }

        size_type to_peek = std::min(count - bytes_peeked, available_in_node);
        size_type actually_peeked =
            node->chunk.peek_at(pos, buffer + bytes_peeked, to_peek);

        bytes_peeked += actually_peeked;
        pos += actually_peeked;

        if (pos >= node->chunk.size()) {
            node = node->next.get();
            pos = 0;
        }
    }

    return bytes_peeked;
}

std::optional<memory_stream::value_type> memory_stream::read_byte() {
    if (empty()) {
        return std::nullopt;
    }

    if (!current_read_node_) {
        current_read_node_ = head_.get();
        current_read_pos_ = 0;
    }

    while (current_read_node_) {
        if (current_read_pos_ < current_read_node_->chunk.size()) {
            auto byte = current_read_node_->chunk.at(current_read_pos_);
            if (byte.has_value()) {
                ++current_read_pos_;
                if (current_read_pos_ >= current_read_node_->chunk.size()) {
                    current_read_node_ = current_read_node_->next.get();
                    current_read_pos_ = 0;
                }
                return byte;
            }
        }
        current_read_node_ = current_read_node_->next.get();
        current_read_pos_ = 0;
    }

    return std::nullopt;
}

memory_stream::size_type memory_stream::consume(value_type* buffer,
                                                size_type count) {
    size_type bytes_consumed = 0;
    value_type* dst = buffer;

    while (bytes_consumed < count && !empty()) {
        if (!head_) {
            break;
        }

        size_type to_consume =
            std::min(count - bytes_consumed, head_->chunk.size());

        if (to_consume > 0) {
            if (dst) {
                head_->chunk.peek(dst + bytes_consumed, to_consume);
            }

            if (head_->chunk.consume_front(to_consume)) {
                total_size_ -= to_consume;
                bytes_consumed += to_consume;

                if (head_->chunk.empty()) {
                    remove_front_chunk();
                }
            }
        }
    }

    return bytes_consumed;
}

void memory_stream::reset_read_position() {
    current_read_node_ = head_.get();
    current_read_pos_ = 0;
}

bool memory_stream::seek_read_position(size_type pos) {
    if (pos > total_size_) {
        return false;
    }

    chunk_node* node = head_.get();
    size_type accumulated = 0;

    while (node && accumulated + node->chunk.size() <= pos) {
        accumulated += node->chunk.size();
        node = node->next.get();
    }

    if (node) {
        current_read_node_ = node;
        current_read_pos_ = pos - accumulated;
        return true;
    }

    return false;
}

// --- 查询操作 ---
memory_stream::size_type memory_stream::size() const noexcept {
    return total_size_;
}

memory_stream::size_type memory_stream::chunk_count() const noexcept {
    size_type count = 0;
    const chunk_node* node = head_.get();
    while (node) {
        ++count;
        node = node->next.get();
    }
    return count;
}

bool memory_stream::empty() const noexcept { return total_size_ == 0; }

memory_stream::size_type memory_stream::chunk_capacity() const noexcept {
    return chunk_capacity_;
}

// --- 清理操作 ---
void memory_stream::clear() {
    head_.reset();
    tail_ = nullptr;
    current_read_node_ = nullptr;
    current_read_pos_ = 0;
    total_size_ = 0;
}

void memory_stream::trim() {
    while (head_ && head_->chunk.empty()) {
        remove_front_chunk();
    }

    if (head_) {
        current_read_node_ = head_.get();
        current_read_pos_ = 0;
    } else {
        current_read_node_ = nullptr;
        current_read_pos_ = 0;
    }
}

// --- 迭代器支持 ---
memory_stream::iterator memory_stream::begin() {
    if (!head_ || head_->chunk.empty()) {
        return {nullptr, 0};
    }
    return {head_.get(), 0};
}

memory_stream::const_iterator memory_stream::begin() const {
    if (!head_ || head_->chunk.empty()) {
        return {nullptr, 0};
    }
    return {head_.get(), 0};
}

memory_stream::iterator memory_stream::end() {
    if (!tail_) {
        return {nullptr, 0};
    }
    return {nullptr, 0};
}

memory_stream::const_iterator memory_stream::end() const {
    if (!tail_) {
        return {nullptr, 0};
    }
    return {nullptr, 0};
}

memory_stream::const_iterator memory_stream::cbegin() const { return begin(); }

memory_stream::const_iterator memory_stream::cend() const { return end(); }

// --- 高级操作 ---
std::vector<memory_stream::value_type> memory_stream::to_vector() const {
    std::vector<value_type> result;
    result.reserve(total_size_);

    const chunk_node* node = head_.get();
    while (node) {
        const auto* data = node->chunk.data();
        size_type node_size = node->chunk.size();
        result.insert(result.end(), data, data + node_size);
        node = node->next.get();
    }

    return result;
}

void memory_stream::compact() {
    if (chunk_count() <= 1) {
        return;
    }

    auto new_chunk = std::make_unique<chunk_node>(chunk_capacity_);
    chunk_node* new_tail = new_chunk.get();

    chunk_node* node = head_.get();
    while (node) {
        size_type to_copy = node->chunk.size();
        if (to_copy > 0) {
            if (new_tail->chunk.full()) {
                auto next_chunk = std::make_unique<chunk_node>(chunk_capacity_);
                new_tail->next = std::move(next_chunk);
                new_tail = new_tail->next.get();
            }

            size_type available = new_tail->chunk.available_space();
            size_type copy_now = std::min(to_copy, available);

            new_tail->chunk.write(node->chunk.data(), copy_now);

            if (copy_now < to_copy) {
                new_tail->chunk.write(node->chunk.data() + copy_now,
                                      to_copy - copy_now);
            }
        }
        node = node->next.get();
    }

    head_ = std::move(new_chunk);
    tail_ = new_tail;
    current_read_node_ = head_.get();
    current_read_pos_ = 0;
}

std::vector<memory_stream::chunk_stats> memory_stream::get_chunk_stats() const {
    std::vector<chunk_stats> stats;
    const chunk_node* node = head_.get();
    size_type index = 0;

    while (node) {
        chunk_stats s;
        s.chunk_index = index;
        s.data_size = node->chunk.size();
        s.capacity = node->chunk.capacity();
        s.usage_percent =
            (s.capacity > 0)
                ? (static_cast<double>(s.data_size) / s.capacity * 100.0)
                : 0.0;
        stats.push_back(s);

        node = node->next.get();
        ++index;
    }

    return stats;
}

// --- 私有辅助方法 ---
bool memory_stream::ensure_write_block() {
    if (!head_) {
        head_ = std::make_unique<chunk_node>(chunk_capacity_);
        tail_ = head_.get();
        if (!current_read_node_) {
            current_read_node_ = head_.get();
        }
        return true;
    }

    if (tail_->chunk.full()) {
        add_new_chunk();
    }

    return tail_ != nullptr;
}

void memory_stream::add_new_chunk() {
    auto new_chunk = std::make_unique<chunk_node>(chunk_capacity_);
    tail_->next = std::move(new_chunk);
    tail_ = tail_->next.get();
}

void memory_stream::remove_front_chunk() {
    if (!head_) {
        return;
    }

    if (current_read_node_ == head_.get()) {
        current_read_node_ = head_->next.get();
        current_read_pos_ = 0;
    }

    head_ = std::move(head_->next);

    if (!head_) {
        tail_ = nullptr;
        current_read_node_ = nullptr;
        current_read_pos_ = 0;
    }
}