#include "tar_archive.hpp"

#include <fmt/core.h>

#include <cstring>
#include <fstream>
#include <iostream>

#include "microtar.hpp"
#include "string_convertor.hpp"

tar_archive::tar_archive(const fs::path& path, tar_archive::mode mode) {
    const char* mode_str = "";
    switch (mode) {
        case tar_archive::mode::read:
            mode_str = "r";
            break;
        case tar_archive::mode::write:
            mode_str = "w";
            break;
        case tar_archive::mode::append:
            mode_str = "a";
            break;
    }

    const auto gbk_path = string_convertor::utf8_to_gbk(path.string());

    int res = mtar_open(&tar_, gbk_path.c_str(), mode_str);

    if (res != MTAR_ESUCCESS) {
        throw std::runtime_error(
            fmt::format("Failed to open tar archive: {}", mtar_strerror(res)));
    }
    is_open_ = true;
}

tar_archive::~tar_archive() {
    if (is_open_) {
        mtar_close(&tar_);
    }
}

tar_archive::tar_archive(tar_archive&& other) noexcept
    : tar_(other.tar_), is_open_(other.is_open_) {
    other.is_open_ = false;
}

tar_archive& tar_archive::operator=(tar_archive&& other) noexcept {
    if (this != &other) {
        if (is_open_) {
            mtar_close(&tar_);
        }
        tar_ = other.tar_;
        is_open_ = other.is_open_;
        other.is_open_ = false;
    }
    return *this;
}

void tar_archive::add_file(const fs::path& file_path,
                           const fs::path& archive_path) {
    if (!fs::exists(file_path)) {
        throw std::runtime_error(
            fmt::format("File not found: {}", file_path.string()));
    }

    if (!fs::is_regular_file(file_path)) {
        throw std::runtime_error(
            fmt::format("Not a regular file: {}", file_path.string()));
    }

    auto final_archive_path =
        archive_path.empty() ? file_path.filename() : archive_path;
    auto file_size = fs::file_size(file_path);

    fmt::print("Adding file: {} as {}\n", file_path.string(),
               final_archive_path.string());

    // 打开文件
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error(
            fmt::format("Failed to open file: {}", file_path.string()));
    }

    auto const final_archive_path_string = final_archive_path.string();

    // 将文件名从 utf8 转换为 gbk 字符串
    const auto gbk_string =
        string_convertor::utf8_to_gbk(final_archive_path.string());

    // 写入文件头
    int res = mtar_write_file_header(&tar_, gbk_string.c_str(), file_size);

    if (res != MTAR_ESUCCESS) {
        throw std::runtime_error(
            fmt::format("Failed to write file header: {}", mtar_strerror(res)));
    }

    // 写入文件内容
    std::vector<char> buffer(file_size);
    file.read(buffer.data(), file_size);

    res = mtar_write_data(&tar_, buffer.data(), file_size);
    if (res != MTAR_ESUCCESS) {
        throw std::runtime_error(
            fmt::format("Failed to write file data: {}", mtar_strerror(res)));
    }
}

void tar_archive::add_directory(const fs::path& dir_path,
                                const fs::path& archive_base) {
    // 判断路径是否存在
    if (!fs::exists(dir_path)) {
        throw std::runtime_error(
            fmt::format("Directory not found: {}", dir_path.string()));
    }

    // 判断目录是否存在
    if (!fs::is_directory(dir_path)) {
        throw std::runtime_error(
            fmt::format("Not a directory: {}", dir_path.string()));
    }

    fmt::print("Adding directory: {}\n", dir_path.string());

    // 遍历打包目录，注意：所有获取的路径，均是 utf8 编码
    // 要在 windows 中使用，需要转换成 gbk 编码
    for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
            fs::path relative_path = fs::relative(entry.path(), dir_path);
            if (!archive_base.empty()) {
                relative_path = archive_base / relative_path;
            }
            add_file(entry.path(), relative_path);
        } else if (entry.is_directory()) {
            // 对于目录，我们需要创建一个目录条目
            fs::path relative_path = fs::relative(entry.path(), dir_path);
            if (!archive_base.empty()) {
                relative_path = archive_base / relative_path;
            }

            // 确保目录路径以 '/' 结尾
            std::string dir_entry_path = relative_path.string();
            if (dir_entry_path.back() != '/') {
                dir_entry_path += '/';
            }

            fmt::print("Adding directory entry: {}\n", dir_entry_path);

            // 将目录从 utf8 转换成 gbk 存储
            const auto gbk_string =
                string_convertor::utf8_to_gbk(dir_entry_path.c_str());

            int res = mtar_write_dir_header(&tar_, gbk_string.c_str());
            if (res != MTAR_ESUCCESS) {
                throw std::runtime_error(
                    fmt::format("Failed to write directory header: {}",
                                mtar_strerror(res)));
            }
        }
    }
}

void tar_archive::extract_all(const fs::path& output_dir) {
    if (!fs::exists(output_dir)) {
        if (!fs::create_directories(output_dir)) {
            fmt::print("The output directory {} create failed\n",
                       output_dir.string());
            return;
        }
    }

    mtar_header_t header;
    while (mtar_read_header(&tar_, &header) != MTAR_ENULLRECORD) {
        // 输出目录名从 gbk 转换成 utf8 编码
        const auto utf8_name_string =
            string_convertor::gbk_to_utf8(header.name);
        fs::path file_path = output_dir.string() + "/" + utf8_name_string;

        fmt::print("path: {}\n", file_path.string());

        // 检查是否是目录
        const auto is_directory =
            header.type == MTAR_TDIR ||
            (utf8_name_string[utf8_name_string.size() - 1] == '/');

        // 尝试创建目录，如果目录不存在，那么不会创建它
        if (is_directory) {
            if (fs::create_directories(file_path)) {
                fmt::print("Creating directory: {}\n", file_path.string());
            }
            mtar_next(&tar_);
            continue;
        }

        // 创建父目录
        fs::create_directories(file_path.parent_path());

        fmt::print("Extracting file: {} (size: {} bytes)\n", file_path.string(),
                   header.size);

        // 打开输出文件
        std::ofstream file(file_path, std::ios::binary);
        if (!file) {
            throw std::runtime_error(
                fmt::format("Failed to create file: {}", file_path.string()));
        }

        // 读取文件内容
        std::vector<char> buffer(header.size);
        int res = mtar_read_data(&tar_, buffer.data(), header.size);
        if (res != MTAR_ESUCCESS) {
            throw std::runtime_error(fmt::format(
                "Failed to read file data : {} ", mtar_strerror(res)));
        }

        // 写入文件内容
        file.write(buffer.data(), header.size);
        mtar_next(&tar_);
    }
}

void tar_archive::finalize() {
    int res = mtar_finalize(&tar_);
    if (res != MTAR_ESUCCESS) {
        throw std::runtime_error(fmt::format(
            "Failed to finalize tar archive: {}", mtar_strerror(res)));
    }
}

void tar_archive::create(const fs::path& output_path,
                         const std::vector<fs::path>& files) {
    tar_archive archive(output_path, mode::write);
    for (const auto& file : files) {
        archive.add_file(file);
    }
    archive.finalize();
}

void tar_archive::create_from_directory(const fs::path& output_path,
                                        const fs::path& input_dir) {
    tar_archive archive(output_path, mode::write);
    archive.add_directory(input_dir, input_dir.filename());
    archive.finalize();
}

void tar_archive::extract(const fs::path& input_path,
                          const fs::path& output_dir) {
    tar_archive archive(input_path, mode::read);
    archive.extract_all(output_dir);
}