#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>

#include "memory_chunk.hpp"


enum class block_sizes : std::size_t {
    // 小内存块（适合小数据、消息头等）
    Bytes_16 = 16,    // 16字节
    Bytes_32 = 32,    // 32字节
    Bytes_64 = 64,    // 64字节
    Bytes_128 = 128,  // 128字节
    Bytes_256 = 256,  // 256字节

    // 中等内存块（适合常见数据包、缓存等）
    KB_1 = 1 * 1024,    // 1KB
    KB_2 = 2 * 1024,    // 2KB
    KB_4 = 4 * 1024,    // 4KB（常见内存页大小）
    KB_8 = 8 * 1024,    // 8KB
    KB_16 = 16 * 1024,  // 16KB

    // 大内存块（适合文件I/O、大数据等）
    KB_32 = 32 * 1024,    // 32KB
    KB_64 = 64 * 1024,    // 64KB
    KB_128 = 128 * 1024,  // 128KB
    KB_256 = 256 * 1024,  // 256KB
    KB_512 = 512 * 1024,  // 512KB

    // 特大内存块（适合大文件处理等）
    MB_1 = 1 * 1024 * 1024,    // 1MB
    MB_2 = 2 * 1024 * 1024,    // 2MB
    MB_4 = 4 * 1024 * 1024,    // 4MB
    MB_8 = 8 * 1024 * 1024,    // 8MB
    MB_16 = 16 * 1024 * 1024,  // 16MB

    // 特殊用途大小
    DefaultChunkSize = KB_1,    // 默认块大小（1KB）
    NetworkPacketSize = KB_2,   // 典型网络数据包大小
    PageSize = KB_4,            // 典型内存页大小
    CacheLineSize = Bytes_64,   // 典型CPU缓存行大小
    MaxSafeStackSize = KB_128,  // 栈上安全分配的推荐最大值
};

/**
 * @brief 内存块流 - 基于链表的内存流管理
 *
 * 将多个固定大小的 memory_chunk 用链表连接起来，提供流式读写接口
 * 支持动态增长，适用于不确定大小的数据流
 */
class memory_stream {
   public:
    using value_type = unsigned char;
    using size_type = std::size_t;
    using pointer = value_type*;
    using const_pointer = const value_type*;

   private:
    /**
     * @brief 内存块节点
     */
    struct chunk_node {
        memory_chunk chunk;                // 实际内存块
        std::unique_ptr<chunk_node> next;  // 下一个节点
        size_type global_start;            // 在流中的起始位置

        chunk_node(size_type capacity, size_type start_pos);
    };

    // 块大小策略函数类型
    using chunk_size_strategy =
        std::function<size_type(size_type current_size, size_type data_size)>;

    std::unique_ptr<chunk_node> head_;         // 链表头
    chunk_node* tail_;                         // 链表尾（便于快速追加）
    chunk_node* current_read_node_;            // 当前读取节点
    size_type total_size_;                     // 总数据大小
    size_type read_pos_;                       // 读取位置（全局位置）
    chunk_size_strategy chunk_size_strategy_;  // 块大小策略
    size_type default_chunk_size_;             // 默认块大小

   public:
    // --- 构造函数和析构函数 ---

    /**
     * @brief 默认构造函数，使用默认块大小
     */
    memory_stream();

    /**
     * @brief 指定固定块大小的构造函数
     * @param chunk_size 每个内存块的固定大小
     */
    explicit memory_stream(size_type chunk_size);

    /**
     * @brief 使用块大小策略的构造函数
     * @param strategy 块大小策略函数
     */
    explicit memory_stream(chunk_size_strategy strategy);

    /**
     * @brief 使用 BlockSizes 枚举的构造函数
     * @param block_size 预定义的块大小
     */
    explicit memory_stream(block_sizes block_size);

    // 禁止拷贝
    memory_stream(const memory_stream&) = delete;
    memory_stream& operator=(const memory_stream&) = delete;

    // 允许移动
    memory_stream(memory_stream&& other) noexcept;
    memory_stream& operator=(memory_stream&& other) noexcept;

    ~memory_stream();

    // --- 块大小策略 ---

    /**
     * @brief 设置固定块大小策略
     * @param chunk_size 固定块大小
     */
    void set_fixed_chunk_size(size_type chunk_size);

    /**
     * @brief 设置指数增长的块大小策略
     * @param initial_size 初始块大小
     * @param growth_factor 增长因子
     */
    void set_exponential_chunk_size(size_type initial_size,
                                    double growth_factor = 2.0);

    /**
     * @brief 设置基于数据大小的块大小策略
     * @param min_size 最小块大小
     * @param max_size 最大块大小
     */
    void set_adaptive_chunk_size(size_type min_size, size_type max_size);

    /**
     * @brief 自定义块大小策略
     * @param strategy 策略函数
     */
    void set_chunk_size_strategy(chunk_size_strategy strategy);

    // --- 写入操作 ---

    /**
     * @brief 写入数据到流
     * @param data 数据指针
     * @param count 数据大小
     * @return 实际写入的字节数
     */
    size_type write(const_pointer data, size_type count);

    /**
     * @brief 写入单个字节
     * @param byte 要写入的字节
     * @return 是否成功
     */
    bool write_byte(value_type byte);

    /**
     * @brief 在末尾追加数据
     * @param data 数据指针
     * @param count 数据大小
     * @return 实际写入的字节数
     */
    size_type append(const_pointer data, size_type count);

    /**
     * @brief 填充多个相同字节
     * @param byte 要填充的字节
     * @param count 填充数量
     * @return 实际填充的字节数
     */
    size_type fill(value_type byte, size_type count);

    // --- 读取操作 ---

    /**
     * @brief 读取数据但不移除
     * @param buffer 输出缓冲区
     * @param count 要读取的大小
     * @return 实际读取的字节数
     */
    size_type peek(pointer buffer, size_type count) const;

    /**
     * @brief 读取数据并移除
     * @param buffer 输出缓冲区
     * @param count 要读取的大小
     * @return 实际读取的字节数
     */
    size_type read(pointer buffer, size_type count);

    /**
     * @brief 读取单个字节
     * @return 读取的字节，如果流为空则返回 std::nullopt
     */
    std::optional<value_type> read_byte();

    /**
     * @brief 跳过指定数量的字节
     * @param count 要跳过的字节数
     * @return 实际跳过的字节数
     */
    size_type skip(size_type count);

    /**
     * @brief 消耗（移除）已读取的数据
     * @param count 要消耗的字节数
     * @return 实际消耗的字节数
     */
    size_type consume(size_type count);

    // --- 查询操作 ---

    /**
     * @brief 获取总数据大小
     */
    size_type size() const noexcept;

    /**
     * @brief 获取块数量
     */
    size_type chunk_count() const noexcept;

    /**
     * @brief 获取已用块的总容量
     */
    size_type total_capacity() const noexcept;

    /**
     * @brief 检查流是否为空
     */
    bool empty() const noexcept;

    /**
     * @brief 获取当前读取位置
     */
    size_type tell() const noexcept;

    /**
     * @brief 获取可用读取的数据大小
     */
    size_type available() const noexcept;

    /**
     * @brief 获取块大小统计信息
     */
    void get_chunk_stats(size_type& min_size, size_type& max_size,
                         size_type& avg_size) const;

    // --- 流控制 ---

    /**
     * @brief 清空流
     */
    void clear();

    /**
     * @brief 重置读取位置到开头
     */
    void rewind();

    /**
     * @brief 设置读取位置
     * @param pos 新的读取位置
     * @return 是否成功
     */
    bool seek(size_type pos);

    // --- 遍历访问 ---

    /**
     * @brief 遍历所有内存块
     * @param visitor 访问器函数
     */
    void for_each_chunk(std::function<void(const memory_chunk&)> visitor) const;

   private:
    // --- 私有辅助方法 ---

    /**
     * @brief 分配新的内存块
     */
    void allocate_new_chunk();

    /**
     * @brief 查找包含指定全局位置的节点
     * @param global_pos 全局位置
     * @return 包含该位置的节点，如果没找到返回nullptr
     */
    chunk_node* find_node_at_position(size_type global_pos) const;

    /**
     * @brief 更新所有节点的全局起始位置
     * @param offset 偏移量
     */
    void update_global_starts(size_type offset);

    /**
     * @brief 删除空的头节点
     */
    void remove_empty_head();

    /**
     * @brief 更新指定节点之后的所有节点的全局起始位置
     * @param start_node 起始节点
     */
    void update_global_starts_after(chunk_node* start_node);
};