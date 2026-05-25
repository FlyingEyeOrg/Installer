#pragma once

#include <filesystem>
#include <span>
#include <vector>

namespace fs = std::filesystem;

class file {
   public:
    static std::vector<std::byte> read_all_bytes(const fs::path& file_path);
    static void write_all_bytes(const fs::path& out_file,
                                const std::span<const std::byte> data);
};