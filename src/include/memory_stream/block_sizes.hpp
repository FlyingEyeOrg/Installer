#pragma once
#include <memory>

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