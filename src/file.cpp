#include "file.hpp"

#include <fstream>

std::vector<std::byte> file::read_all_bytes(const fs::path& file_path) {
    // 打开文件，以二进制模式读取
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + file_path.string());
    }

    // 获取文件大小
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // 创建足够大的缓冲区
    std::vector<std::byte> buffer(file_size);

    if (file_size > 0) {
        // 读取文件内容
        if (!file.read(reinterpret_cast<char*>(buffer.data()), file_size)) {
            throw std::runtime_error("Failed to read file: " +
                                     file_path.string());
        }
    }

    return buffer;
}

void file::write_all_bytes(const fs::path& out_file,
                           const std::span<const std::byte> data) {
    // 确保输出目录存在
    fs::create_directories(out_file.parent_path());

    // 打开文件，以二进制模式写入
    std::ofstream file(out_file, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to crate file: " + out_file.string());
    }

    // 写入数据
    if (!data.empty()) {
        if (!file.write(reinterpret_cast<const char*>(data.data()),
                        static_cast<std::streamsize>(data.size()))) {
            throw std::runtime_error("Failed to write file: " +
                                     out_file.string());
        }
    }

    // 确保数据写入磁盘
    file.flush();
}