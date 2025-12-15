#include "zstd_compression.hpp"

#include <zstd.h>

#include <span>

std::optional<std::vector<std::byte>> zstd_compression::decompress(
    const std::span<const std::byte> data) {
    const auto buffer_size = ZSTD_getFrameContentSize(data.data(), data.size());

    // 检查错误情况
    if (buffer_size == ZSTD_CONTENTSIZE_ERROR ||
        buffer_size == ZSTD_CONTENTSIZE_UNKNOWN) {
        return std::nullopt;  // 不是有效的 ZSTD 数据
    }

    std::vector<std::byte> decompressed_buffer(buffer_size);

    const auto decompressed_size = ZSTD_decompress(
        decompressed_buffer.data(), buffer_size, data.data(), data.size());

    if (ZSTD_isError(decompressed_size)) {
        // 或返回空向量
        return std::nullopt;
    }

    // 调整大小到实际解压缩后的大小
    decompressed_buffer.resize(decompressed_size);

    return decompressed_buffer;
}

std::optional<std::vector<std::byte>> zstd_compression::compress(
    const std::span<const std::byte> data) {
    if (data.empty()) {
        // 或返回空向量
        return std::nullopt;
    }

    const size_t buffer_size = ZSTD_compressBound(data.size());

    // 安全检查：确保 buffer_size 合理
    if (buffer_size == 0) {
        return std::nullopt;
    }

    std::vector<std::byte> compressed_buffer(buffer_size);

    const size_t compressed_size =
        ZSTD_compress(compressed_buffer.data(), buffer_size, data.data(),
                      data.size(), ZSTD_maxCLevel());

    if (ZSTD_isError(compressed_size)) {
        // 或返回空向量
        return std::nullopt;
    }

    // 调整大小到实际压缩后的大小
    compressed_buffer.resize(compressed_size);

    return compressed_buffer;
}