#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <vector>

namespace fs = std::filesystem;

class zstd_compression {
   private:
    zstd_compression() {}

   public:
    static std::optional<std::vector<std::byte>> compress(
        const std::span<const std::byte> data);
    static std::optional<std::vector<std::byte>> decompress(
        const std::span<const std::byte> data);
};
