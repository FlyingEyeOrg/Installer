#include "memory_stream/memory_stream.hpp"

#undef min
#undef max

// chunk_node 构造函数
memory_stream::chunk_node::chunk_node(size_type capacity, size_type start_pos)
    : chunk(capacity), global_start(start_pos) {}

// --- 构造函数和析构函数 ---

// 默认构造函数，使用默认块大小
memory_stream::memory_stream()
    : memory_stream(static_cast<size_type>(block_sizes::DefaultChunkSize)) {}

// 指定固定块大小的构造函数
memory_stream::memory_stream(size_type chunk_size)
    : head_(nullptr),
      tail_(nullptr),
      current_read_node_(nullptr),
      total_size_(0),
      read_pos_(0),
      default_chunk_size_(chunk_size) {
    if (chunk_size == 0) {
        throw std::invalid_argument("Chunk size cannot be zero");
    }

    // 默认策略：固定块大小
    chunk_size_strategy_ = [this](size_type, size_type) {
        return default_chunk_size_;
    };
}

// 使用块大小策略的构造函数
memory_stream::memory_stream(chunk_size_strategy strategy)
    : head_(nullptr),
      tail_(nullptr),
      current_read_node_(nullptr),
      total_size_(0),
      read_pos_(0),
      default_chunk_size_(0),
      chunk_size_strategy_(std::move(strategy)) {
    if (!chunk_size_strategy_) {
        throw std::invalid_argument("Chunk size strategy cannot be null");
    }
}

// 使用 BlockSizes 枚举的构造函数
memory_stream::memory_stream(block_sizes block_size)
    : memory_stream(static_cast<size_type>(block_size)) {}

// 移动构造
memory_stream::memory_stream(memory_stream&& other) noexcept
    : head_(std::move(other.head_)),
      tail_(other.tail_),
      current_read_node_(other.current_read_node_),
      total_size_(other.total_size_),
      read_pos_(other.read_pos_),
      chunk_size_strategy_(std::move(other.chunk_size_strategy_)),
      default_chunk_size_(other.default_chunk_size_) {
    other.tail_ = nullptr;
    other.current_read_node_ = nullptr;
    other.total_size_ = 0;
    other.read_pos_ = 0;
    other.default_chunk_size_ = 0;
}

// 移动赋值
memory_stream& memory_stream::operator=(memory_stream&& other) noexcept {
    if (this != &other) {
        clear();
        head_ = std::move(other.head_);
        tail_ = other.tail_;
        current_read_node_ = other.current_read_node_;
        total_size_ = other.total_size_;
        read_pos_ = other.read_pos_;
        chunk_size_strategy_ = std::move(other.chunk_size_strategy_);
        default_chunk_size_ = other.default_chunk_size_;

        other.tail_ = nullptr;
        other.current_read_node_ = nullptr;
        other.total_size_ = 0;
        other.read_pos_ = 0;
        other.default_chunk_size_ = 0;
    }
    return *this;
}

// 析构函数
memory_stream::~memory_stream() = default;

// --- 块大小策略 ---

// 设置固定块大小策略
void memory_stream::set_fixed_chunk_size(size_type chunk_size) {
    if (chunk_size == 0) {
        throw std::invalid_argument("Chunk size cannot be zero");
    }
    default_chunk_size_ = chunk_size;
    chunk_size_strategy_ = [this](size_type, size_type) {
        return default_chunk_size_;
    };
}

// 设置指数增长的块大小策略
void memory_stream::set_exponential_chunk_size(size_type initial_size,
                                               double growth_factor) {
    if (initial_size == 0 || growth_factor <= 1.0) {
        throw std::invalid_argument(
            "Invalid parameters for exponential growth");
    }

    chunk_size_strategy_ = [initial_size, growth_factor](size_type current_size,
                                                         size_type) {
        if (current_size == 0) {
            return initial_size;
        }
        return static_cast<size_type>(current_size * growth_factor);
    };
}

// 设置基于数据大小的块大小策略
void memory_stream::set_adaptive_chunk_size(size_type min_size,
                                            size_type max_size) {
    if (min_size == 0 || max_size < min_size) {
        throw std::invalid_argument("Invalid parameters for adaptive size");
    }

    chunk_size_strategy_ = [min_size, max_size](size_type,
                                                size_type data_size) {
        // 根据数据大小选择块大小
        if (data_size <= min_size) {
            return min_size;
        } else if (data_size >= max_size) {
            return max_size;
        } else {
            // 向上取整到最近的2的幂
            size_t size = 1;
            while (size < data_size) {
                size <<= 1;
            }
            return std::min(size, max_size);
        }
    };
}

// 自定义块大小策略
void memory_stream::set_chunk_size_strategy(chunk_size_strategy strategy) {
    if (!strategy) {
        throw std::invalid_argument("Chunk size strategy cannot be null");
    }
    chunk_size_strategy_ = std::move(strategy);
}

// --- 写入操作 ---

// 写入数据到流
memory_stream::size_type memory_stream::write(const_pointer data,
                                              size_type count) {
    if (count == 0 || data == nullptr) {
        return 0;
    }

    size_type written = 0;
    const_pointer current_data = data;

    while (written < count) {
        // 确保有可写的块
        if (!tail_ || tail_->chunk.full()) {
            allocate_new_chunk();
        }

        // 写入当前块
        size_type to_write = count - written;
        size_type chunk_written = tail_->chunk.write(current_data, to_write);

        if (chunk_written == 0) {
            // 当前块已满，分配新块
            allocate_new_chunk();
            continue;
        }

        written += chunk_written;
        current_data += chunk_written;
        total_size_ += chunk_written;
    }

    return written;
}

// 写入单个字节
bool memory_stream::write_byte(value_type byte) {
    if (!tail_ || tail_->chunk.full()) {
        allocate_new_chunk();
    }

    if (tail_->chunk.write_byte(byte)) {
        ++total_size_;
        return true;
    }
    return false;
}

// 在末尾追加数据
memory_stream::size_type memory_stream::append(const_pointer data,
                                               size_type count) {
    return write(data, count);
}

// 填充多个相同字节
memory_stream::size_type memory_stream::fill(value_type byte, size_type count) {
    if (count == 0) {
        return 0;
    }

    size_type filled = 0;

    while (filled < count) {
        if (!tail_ || tail_->chunk.full()) {
            allocate_new_chunk();
        }

        size_type to_fill = count - filled;
        size_type chunk_filled = tail_->chunk.fill(byte, to_fill);

        if (chunk_filled == 0) {
            allocate_new_chunk();
            continue;
        }

        filled += chunk_filled;
        total_size_ += chunk_filled;
    }

    return filled;
}

// --- 读取操作 ---

// 读取数据但不移除
memory_stream::size_type memory_stream::peek(pointer buffer,
                                             size_type count) const {
    if (count == 0 || buffer == nullptr || empty()) {
        return 0;
    }

    count = std::min(count, total_size_ - read_pos_);
    if (count == 0) {
        return 0;
    }

    // 找到读取位置所在的节点
    chunk_node* node = find_node_at_position(read_pos_);
    if (!node) {
        return 0;
    }

    size_type read = 0;
    size_type current_pos = read_pos_;
    pointer current_buffer = buffer;

    while (read < count && node) {
        // 计算在节点内的相对位置
        size_type node_relative_pos = current_pos - node->global_start;
        size_type node_available = node->chunk.size() - node_relative_pos;
        size_type to_read = std::min(count - read, node_available);

        if (to_read > 0) {
            size_type node_read =
                node->chunk.peek_at(node_relative_pos, current_buffer, to_read);
            if (node_read == 0) {
                break;
            }

            read += node_read;
            current_buffer += node_read;
            current_pos += node_read;
        }

        node = node->next.get();
    }

    return read;
}

// 读取数据并移除
memory_stream::size_type memory_stream::read(pointer buffer, size_type count) {
    size_type bytes_read = peek(buffer, count);
    if (bytes_read > 0) {
        consume(bytes_read);
    }
    return bytes_read;
}

// 读取单个字节
std::optional<memory_stream::value_type> memory_stream::read_byte() {
    if (empty() || read_pos_ >= total_size_) {
        return std::nullopt;
    }

    // 找到当前读取位置所在的节点
    chunk_node* node = find_node_at_position(read_pos_);
    if (!node) {
        return std::nullopt;
    }

    // 计算在节点内的相对位置
    size_type node_relative_pos = read_pos_ - node->global_start;

    // 从节点读取字节
    auto byte = node->chunk.at(node_relative_pos);
    if (byte) {
        // 移除这个字节
        node->chunk.erase_at(node_relative_pos);
        ++read_pos_;
        --total_size_;

        // 如果这个节点空了，并且不是当前读取节点，可能需要调整
        if (node->chunk.empty() && node != current_read_node_) {
            // 可以在这里实现节点回收逻辑
        }
    }

    return byte;
}

// 跳过指定数量的字节
memory_stream::size_type memory_stream::skip(size_type count) {
    count = std::min(count, total_size_ - read_pos_);
    if (count > 0) {
        read_pos_ += count;

        // 更新当前读取节点
        if (current_read_node_) {
            while (current_read_node_ &&
                   read_pos_ >= current_read_node_->global_start +
                                    current_read_node_->chunk.size()) {
                current_read_node_ = current_read_node_->next.get();
            }
        }
    }
    return count;
}

// 消耗（移除）已读取的数据
memory_stream::size_type memory_stream::consume(size_type count) {
    count = std::min(count, total_size_ - read_pos_);
    if (count == 0) {
        return 0;
    }

    size_type consumed = 0;
    size_type to_consume = count;

    while (to_consume > 0 && head_) {
        chunk_node* node = head_.get();

        // 计算在这个节点中要消耗多少
        size_type node_available = node->chunk.size();
        size_type node_consume = std::min(to_consume, node_available);

        if (node_consume > 0) {
            // 从节点开头删除数据
            if (!node->chunk.consume_front(node_consume)) {
                break;
            }

            consumed += node_consume;
            to_consume -= node_consume;
            read_pos_ += node_consume;
            total_size_ -= node_consume;

            // 更新所有后续节点的全局起始位置
            update_global_starts(node_consume);

            // 如果节点空了，删除它
            if (node->chunk.empty()) {
                remove_empty_head();
            }
        }
    }

    return consumed;
}

// --- 查询操作 ---

// 获取总数据大小
memory_stream::size_type memory_stream::size() const noexcept {
    return total_size_;
}

// 获取块数量
memory_stream::size_type memory_stream::chunk_count() const noexcept {
    size_type count = 0;
    chunk_node* node = head_.get();
    while (node) {
        ++count;
        node = node->next.get();
    }
    return count;
}

// 获取已用块的总容量
memory_stream::size_type memory_stream::total_capacity() const noexcept {
    size_type capacity = 0;
    chunk_node* node = head_.get();
    while (node) {
        capacity += node->chunk.capacity();
        node = node->next.get();
    }
    return capacity;
}

// 检查流是否为空
bool memory_stream::empty() const noexcept { return total_size_ == 0; }

// 获取当前读取位置
memory_stream::size_type memory_stream::tell() const noexcept {
    return read_pos_;
}

// 获取可用读取的数据大小
memory_stream::size_type memory_stream::available() const noexcept {
    return total_size_ - read_pos_;
}

// 获取块大小统计信息
void memory_stream::get_chunk_stats(size_type& min_size, size_type& max_size,
                                    size_type& avg_size) const {
    min_size = std::numeric_limits<size_type>::max();
    max_size = 0;
    avg_size = 0;

    size_type count = 0;
    chunk_node* node = head_.get();

    while (node) {
        size_type chunk_size = node->chunk.size();
        min_size = std::min(min_size, chunk_size);
        max_size = std::max(max_size, chunk_size);
        avg_size += chunk_size;
        ++count;
        node = node->next.get();
    }

    if (count > 0) {
        avg_size /= count;
    } else {
        min_size = 0;
        avg_size = 0;
    }
}

// --- 流控制 ---

// 清空流
void memory_stream::clear() {
    head_.reset();
    tail_ = nullptr;
    current_read_node_ = nullptr;
    total_size_ = 0;
    read_pos_ = 0;
}

// 重置读取位置到开头
void memory_stream::rewind() {
    read_pos_ = 0;
    current_read_node_ = head_.get();
}

// 设置读取位置
bool memory_stream::seek(size_type pos) {
    if (pos > total_size_) {
        return false;
    }

    read_pos_ = pos;
    current_read_node_ = find_node_at_position(pos);
    return true;
}

// --- 遍历访问 ---

// 遍历所有内存块
void memory_stream::for_each_chunk(
    std::function<void(const memory_chunk&)> visitor) const {
    chunk_node* node = head_.get();
    while (node) {
        visitor(node->chunk);
        node = node->next.get();
    }
}

// --- 私有辅助方法 ---

// 分配新的内存块
void memory_stream::allocate_new_chunk() {
    size_type new_chunk_size = chunk_size_strategy_(total_capacity(), 0);
    if (new_chunk_size == 0) {
        new_chunk_size = static_cast<size_type>(block_sizes::DefaultChunkSize);
    }

    size_type global_start =
        tail_ ? tail_->global_start + tail_->chunk.capacity() : 0;

    auto new_node = std::make_unique<chunk_node>(new_chunk_size, global_start);

    if (!head_) {
        head_ = std::move(new_node);
        tail_ = head_.get();
        current_read_node_ = head_.get();
    } else {
        tail_->next = std::move(new_node);
        tail_ = tail_->next.get();
    }
}

// 查找包含指定全局位置的节点
memory_stream::chunk_node* memory_stream::find_node_at_position(
    size_type global_pos) const {
    if (global_pos >= total_size_) {
        return nullptr;
    }

    // 从当前读取节点开始查找
    chunk_node* node = current_read_node_;
    if (node && global_pos >= node->global_start &&
        global_pos < node->global_start + node->chunk.size()) {
        return node;
    }

    // 回退到头部开始查找
    node = head_.get();
    while (node) {
        if (global_pos >= node->global_start &&
            global_pos < node->global_start + node->chunk.size()) {
            return node;
        }
        node = node->next.get();
    }

    return nullptr;
}

// 更新所有节点的全局起始位置
void memory_stream::update_global_starts(size_type offset) {
    chunk_node* node = head_.get();
    while (node) {
        if (node->global_start >= offset) {
            node->global_start -= offset;
        } else {
            node->global_start = 0;
        }
        node = node->next.get();
    }
}

// 删除空的头节点
void memory_stream::remove_empty_head() {
    if (!head_ || !head_->chunk.empty()) {
        return;
    }

    // 保存下一个节点
    std::unique_ptr<chunk_node> next = std::move(head_->next);

    // 如果当前读取节点是头节点，移动到下一个
    if (current_read_node_ == head_.get()) {
        current_read_node_ = next.get();
    }

    // 如果尾节点是头节点，更新尾节点
    if (tail_ == head_.get()) {
        tail_ = next.get();
    }

    // 删除头节点
    head_ = std::move(next);

    // 更新后续节点的全局起始位置
    if (head_) {
        head_->global_start = 0;
        update_global_starts_after(head_.get());
    }
}

// 更新指定节点之后的所有节点的全局起始位置
void memory_stream::update_global_starts_after(chunk_node* start_node) {
    if (!start_node) return;

    chunk_node* node = start_node;
    size_type current_start = node->global_start + node->chunk.capacity();

    node = node->next.get();
    while (node) {
        node->global_start = current_start;
        current_start += node->chunk.capacity();
        node = node->next.get();
    }
}