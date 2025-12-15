#pragma once

#include <filesystem>
#include <optional>
#include <span>

namespace fs = std::filesystem;

class zstd_compression {
   private:
    zstd_compression() {}

   public:
    static std::optional<std::vector<uint8_t>> compress(
        const std::span<const std::byte> data);
    static std::optional<std::vector<uint8_t>> decompress(
        const std::span<const std::byte> data);
};
