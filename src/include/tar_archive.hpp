#pragma once

#include <filesystem>
#include <vector>

#include "microtar.hpp"

namespace fs = std::filesystem;

class tar_archive {
   public:
    enum class mode { read, write, append };

    tar_archive(const fs::path& path, mode mode);
    ~tar_archive();

    // 禁止拷贝
    tar_archive(const tar_archive&) = delete;
    tar_archive& operator=(const tar_archive&) = delete;

    // 移动构造
    tar_archive(tar_archive&& other) noexcept;

    // 移动赋值
    tar_archive& operator=(tar_archive&& other) noexcept;

    // 添加单个文件到 tar 包
    void add_file(const fs::path& file_path, const fs::path& archive_path = {});

    // 添加整个目录到 tar 包
    void add_directory(const fs::path& dir_path,
                       const fs::path& archive_base = {});

    // 解压整个 tar 包到指定目录
    void extract_all(const fs::path& output_dir);

    // 完成写入操作
    void finalize();

    // 打包多个文件到 tar 包
    static void create(const fs::path& output_path,
                       const std::vector<fs::path>& files);

    // 打包整个目录到 tar 包
    static void create_from_directory(const fs::path& output_path,
                                      const fs::path& input_dir);

    // 解压 tar 包到指定目录
    static void extract(const fs::path& input_path, const fs::path& output_dir);

   private:
    mtar_t tar_;
    bool is_open_ = true;
};
