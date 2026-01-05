#include "tar/tar.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace tar {
namespace chrono = std::chrono;

// 工具函数定义保持不变
std::uintmax_t parse_octal(const char* data, std::size_t size) {
    std::uintmax_t result = 0;
    bool started = false;

    for (std::size_t i = 0; i < size; ++i) {
        char c = data[i];
        if (c == 0) break;
        if (c == ' ') {
            if (started) break;
            continue;
        }
        if (c < '0' || c > '7') break;

        started = true;
        result = result * 8 + (c - '0');
    }
    return result;
}

void write_octal(std::uintmax_t value, char* buffer, std::size_t size) {
    std::memset(buffer, ' ', size);
    buffer[size - 1] = 0;

    if (value == 0) {
        buffer[size - 2] = '0';
        return;
    }

    char temp[32];
    char* p = temp + sizeof(temp) - 1;
    *p = 0;

    do {
        *--p = '0' + (value % 8);
        value /= 8;
    } while (value > 0);

    std::size_t len = std::strlen(p);
    if (len > size - 1) len = size - 1;

    std::memcpy(buffer + size - len - 1, p, len);
}

unsigned int calculate_checksum(const header& hdr) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(&hdr);
    unsigned int sum = 0;

    for (std::size_t i = 0; i < sizeof(header); ++i) {
        if (i >= offsetof(header, chksum) && i < offsetof(header, chksum) + 8) {
            sum += ' ';
        } else {
            sum += p[i];
        }
    }
    return sum;
}

bool is_zero_block(const header& hdr) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(&hdr);
    for (std::size_t i = 0; i < sizeof(header); ++i) {
        if (p[i] != 0) return false;
    }
    return true;
}

// writer 类方法定义保持不变
writer::writer() {
    // 初始化字符串流
    out_.exceptions(std::ios::badbit | std::ios::failbit);
}

writer::~writer() {
    if (!out_.str().empty()) {  // 如果有数据，自动完成打包
        try {
            finish();
        } catch (...) {
            // 忽略析构函数中的异常
        }
    }
}

void writer::write_file_data(const fs::path& path, std::uintmax_t size) {
    if (size == 0) return;

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Cannot open file: " + path.string());
    }

    const std::size_t buffer_size = 8192;
    char buffer[buffer_size];
    std::uintmax_t remaining = size;

    while (remaining > 0) {
        std::size_t to_read = static_cast<std::size_t>(
            std::min<std::uintmax_t>(buffer_size, remaining));

        in.read(buffer, to_read);
        std::streamsize bytes_read = in.gcount();

        if (bytes_read <= 0) {
            throw std::runtime_error("Failed to read file: " + path.string());
        }

        out_.write(buffer, bytes_read);
        if (!out_) {
            throw std::runtime_error("Failed to write to archive");
        }

        remaining -= bytes_read;
    }

    // 填充到512字节边界
    std::size_t padding = (512 - (size % 512)) % 512;
    if (padding > 0) {
        std::memset(buffer, 0, padding);
        out_.write(buffer, padding);
    }
}

void writer::write_directory_entry(const std::string& dir_name) {
    // 准备头部
    header hdr = {};

    // 确保目录名以/结尾
    std::string rel_path = dir_name;
    if (!rel_path.empty() && rel_path.back() != '/') {
        rel_path.push_back('/');
    }

    if (rel_path.empty()) {
        rel_path = "./";
    }

    // 处理长路径
    if (rel_path.size() > 100) {
        std::size_t last_slash = rel_path.find_last_of('/');
        if (last_slash != std::string::npos && last_slash <= 155) {
            std::string prefix = rel_path.substr(0, last_slash);
            std::string name = rel_path.substr(last_slash + 1);

            if (name.size() <= 100 && prefix.size() <= 155) {
                std::strncpy(hdr.prefix, prefix.c_str(), 155);
                std::strncpy(hdr.name, name.c_str(), 100);
            } else {
                // 如果还是太长，只保存文件名部分
                std::strncpy(hdr.name, name.c_str(), 100);
            }
        } else {
            // 路径太长且没有斜杠，只保存最后100个字符
            std::size_t start =
                rel_path.size() > 100 ? rel_path.size() - 100 : 0;
            std::strncpy(hdr.name, rel_path.c_str() + start, 100);
        }
    } else {
        std::strncpy(hdr.name, rel_path.c_str(), 100);
    }

    // 目录权限
    write_octal(0755, hdr.mode, sizeof(hdr.mode));

    // 用户和组ID
    write_octal(0, hdr.uid, sizeof(hdr.uid));
    write_octal(0, hdr.gid, sizeof(hdr.gid));

    // 目录大小为0
    write_octal(0, hdr.size, sizeof(hdr.size));

    // 修改时间（使用当前时间）
    auto time = std::time(nullptr);
    write_octal(static_cast<std::uintmax_t>(time), hdr.mtime,
                sizeof(hdr.mtime));

    // 文件类型
    hdr.typeflag = '5';

    // 魔术字和版本
    std::memcpy(hdr.magic, "ustar", 5);
    hdr.magic[5] = 0;
    hdr.version[0] = '0';
    hdr.version[1] = '0';

    // 用户名和组名
    std::strncpy(hdr.uname, "user", 32);
    std::strncpy(hdr.gname, "group", 32);

    // 计算校验和
    unsigned int checksum = calculate_checksum(hdr);
    write_octal(checksum, hdr.chksum, sizeof(hdr.chksum));

    // 写入头部
    out_.write(reinterpret_cast<const char*>(&hdr), sizeof(header));
    if (!out_) {
        throw std::runtime_error("Failed to write header");
    }
}

void writer::write_file_entry(const std::string& file_path,
                              const fs::path& real_path) {
    // 准备头部
    header hdr = {};

    // 处理路径分隔符
    std::string rel_path = file_path;
    std::replace(rel_path.begin(), rel_path.end(), '\\', '/');

    // 处理长路径
    if (rel_path.size() > 100) {
        std::size_t last_slash = rel_path.find_last_of('/');
        if (last_slash != std::string::npos && last_slash <= 155) {
            std::string prefix = rel_path.substr(0, last_slash);
            std::string name = rel_path.substr(last_slash + 1);

            if (name.size() <= 100 && prefix.size() <= 155) {
                std::strncpy(hdr.prefix, prefix.c_str(), 155);
                std::strncpy(hdr.name, name.c_str(), 100);
            } else {
                // 如果还是太长，只保存文件名部分
                std::strncpy(hdr.name, name.c_str(), 100);
            }
        } else {
            // 路径太长且没有斜杠，只保存最后100个字符
            std::size_t start =
                rel_path.size() > 100 ? rel_path.size() - 100 : 0;
            std::strncpy(hdr.name, rel_path.c_str() + start, 100);
        }
    } else {
        std::strncpy(hdr.name, rel_path.c_str(), 100);
    }

    // 文件权限
    write_octal(0644, hdr.mode, sizeof(hdr.mode));

    // 用户和组ID
    write_octal(0, hdr.uid, sizeof(hdr.uid));
    write_octal(0, hdr.gid, sizeof(hdr.gid));

    // 文件大小
    std::uintmax_t size = fs::file_size(real_path);
    write_octal(size, hdr.size, sizeof(hdr.size));

    // 修改时间
    auto mtime = fs::last_write_time(real_path);
    auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(
        mtime - fs::file_time_type::clock::now() + chrono::system_clock::now());
    auto time = chrono::system_clock::to_time_t(sctp);
    write_octal(static_cast<std::uintmax_t>(time), hdr.mtime,
                sizeof(hdr.mtime));

    // 文件类型
    hdr.typeflag = '0';

    // 魔术字和版本
    std::memcpy(hdr.magic, "ustar", 5);
    hdr.magic[5] = 0;
    hdr.version[0] = '0';
    hdr.version[1] = '0';

    // 用户名和组名
    std::strncpy(hdr.uname, "user", 32);
    std::strncpy(hdr.gname, "group", 32);

    // 计算校验和
    unsigned int checksum = calculate_checksum(hdr);
    write_octal(checksum, hdr.chksum, sizeof(hdr.chksum));

    // 写入头部
    out_.write(reinterpret_cast<const char*>(&hdr), sizeof(header));
    if (!out_) {
        throw std::runtime_error("Failed to write header");
    }

    // 写入文件数据
    write_file_data(real_path, size);
}

void writer::add_directory_recursive(const fs::path& dir_path,
                                     const std::string& base_name) {
    // 遍历目录内容
    for (const auto& entry : fs::recursive_directory_iterator(
             dir_path, fs::directory_options::skip_permission_denied)) {
        try {
            // 计算在tar中的路径
            fs::path relative = fs::relative(entry.path(), dir_path);
            std::string tar_path = base_name + "/" + relative.string();
            std::replace(tar_path.begin(), tar_path.end(), '\\', '/');

            if (fs::is_directory(entry.status())) {
                // 确保目录路径以/结尾
                if (!tar_path.empty() && tar_path.back() != '/') {
                    tar_path.push_back('/');
                }
                write_directory_entry(tar_path);
            } else if (fs::is_regular_file(entry.status())) {
                write_file_entry(tar_path, entry.path());
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Skipping " << entry.path().string() << ": "
                      << e.what() << std::endl;
        }
    }
}

void writer::add_file(const fs::path& file_path, const std::string& tar_name) {
    if (!fs::exists(file_path)) {
        throw std::runtime_error("File not found: " + file_path.string());
    }

    if (!fs::is_regular_file(file_path)) {
        throw std::runtime_error("Not a regular file: " + file_path.string());
    }

    // 确定tar中的文件名
    std::string name =
        tar_name.empty() ? file_path.filename().string() : tar_name;
    write_file_entry(name, file_path);
}

void writer::add_directory(const fs::path& dir_path,
                           const std::string& tar_name) {
    if (!fs::exists(dir_path)) {
        throw std::runtime_error("Directory not found: " + dir_path.string());
    }

    if (!fs::is_directory(dir_path)) {
        throw std::runtime_error("Not a directory: " + dir_path.string());
    }

    // 确定tar中的目录名
    std::string name =
        tar_name.empty() ? dir_path.filename().string() : tar_name;

    // 写入顶层目录
    write_directory_entry(name + "/");

    // 递归添加目录内容
    add_directory_recursive(dir_path, name);
}

void writer::finish() {
    // 写入两个全零块表示结束
    char zero_block[1024] = {0};
    out_.write(zero_block, 1024);
    out_.flush();
}

std::string writer::get_data() const { return out_.str(); }

std::vector<char> writer::get_vector() const {
    std::string data = out_.str();
    return std::vector<char>(data.begin(), data.end());
}

void writer::write_to_file(const fs::path& file_path) {
    // 确保完成打包
    finish();

    std::ofstream file(file_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file for writing: " +
                                 file_path.string());
    }

    std::string data = out_.str();
    file.write(data.data(), data.size());

    if (!file) {
        throw std::runtime_error("Failed to write to file: " +
                                 file_path.string());
    }
}

void writer::clear() {
    out_.str("");  // 清空字符串流
    out_.clear();  // 清除错误状态
}

std::size_t writer::size() const { return out_.str().size(); }

bool writer::empty() const { return out_.str().empty(); }

// reader 类方法定义
reader::reader(const fs::path& archive_path) { set_source(archive_path); }

reader::reader(std::unique_ptr<std::istream> stream) {
    set_source(std::move(stream));
}

reader::reader(const std::string& data) { set_source(data); }

reader::reader(const std::vector<char>& data) { set_source(data); }

reader::reader(const char* data, std::size_t size) { set_source(data, size); }

reader::reader(reader&& other) noexcept
    : file_stream_(std::move(other.file_stream_)),
      memory_stream_(std::move(other.memory_stream_)),
      in_(other.in_) {
    other.in_ = nullptr;
}

reader& reader::operator=(reader&& other) noexcept {
    if (this != &other) {
        cleanup();
        file_stream_ = std::move(other.file_stream_);
        memory_stream_ = std::move(other.memory_stream_);
        in_ = other.in_;
        other.in_ = nullptr;
    }
    return *this;
}

void reader::set_source(std::unique_ptr<std::istream> stream) {
    cleanup();
    memory_stream_ = std::move(stream);
    in_ = memory_stream_.get();
    if (in_) {
        // 检查流是否有效
        in_->seekg(0, std::ios::end);
        std::streamsize size = in_->tellg();
        in_->seekg(0, std::ios::beg);

        if (size < 512) {  // 最小的tar文件至少有2个512字节的零块
            throw std::runtime_error("Invalid tar data: data too small");
        }

        if (!*in_) {
            throw std::runtime_error("Failed to set stream source");
        }
    }
}

void reader::set_source(const std::string& data) {
    if (data.size() < 1024) {  // 最小的tar文件至少有2个512字节的零块
        throw std::runtime_error("Invalid tar data: data too small");
    }
    set_source(std::make_unique<std::istringstream>(data));
}

void reader::set_source(const std::vector<char>& data) {
    if (data.size() < 1024) {  // 最小的tar文件至少有2个512字节的零块
        throw std::runtime_error("Invalid tar data: data too small");
    }
    set_source(std::make_unique<std::istringstream>(
        std::string(data.begin(), data.end())));
}

void reader::set_source(const char* data, std::size_t size) {
    if (size < 1024) {  // 最小的tar文件至少有2个512字节的零块
        throw std::runtime_error("Invalid tar data: data too small");
    }
    set_source(std::string(data, data + size));
}

void reader::set_source(const fs::path& archive_path) {
    cleanup();
    file_stream_ =
        std::make_unique<std::ifstream>(archive_path, std::ios::binary);
    if (!*file_stream_) {
        throw std::runtime_error("Cannot open archive: " +
                                 archive_path.string());
    }

    // 检查文件大小
    file_stream_->seekg(0, std::ios::end);
    std::streamsize size = file_stream_->tellg();
    file_stream_->seekg(0, std::ios::beg);

    if (size < 1024) {  // 最小的tar文件至少有2个512字节的零块
        throw std::runtime_error("Invalid tar archive: file too small");
    }

    in_ = file_stream_.get();
}

bool reader::is_open() const { return in_ != nullptr && *in_; }

void reader::close() { cleanup(); }

void reader::cleanup() {
    file_stream_.reset();
    memory_stream_.reset();
    in_ = nullptr;
}

bool reader::read_header(header& hdr) {
    if (!in_) {
        throw std::runtime_error("No data source is open");
    }

    in_->read(reinterpret_cast<char*>(&hdr), sizeof(header));
    std::streamsize bytes_read = in_->gcount();

    if (!*in_ || bytes_read != sizeof(header)) {
        return false;
    }

    // 检查是否为零块
    if (is_zero_block(hdr)) {
        // 再读一个头部，检查是否连续两个零块
        header next_hdr;
        in_->read(reinterpret_cast<char*>(&next_hdr), sizeof(header));
        if (is_zero_block(next_hdr)) {
            return false;  // 两个零块，文件结束
        }
        // 只有一个零块，回退
        in_->seekg(-static_cast<std::streamoff>(sizeof(header)), std::ios::cur);
    }

    return true;
}

std::string reader::get_path(const header& hdr) {
    std::string path;

    // 检查前缀
    if (hdr.prefix[0] != 0) {
        std::string prefix = hdr.prefix;
        std::size_t null_pos = prefix.find('\0');
        if (null_pos != std::string::npos) {
            prefix.resize(null_pos);
        }
        if (!prefix.empty()) {
            path = prefix + "/";
        }
    }

    // 添加文件名
    std::string name = hdr.name;
    std::size_t null_pos = name.find('\0');
    if (null_pos != std::string::npos) {
        name.resize(null_pos);
    }
    path += name;

    return path;
}

void reader::skip_data(std::uintmax_t size) {
    if (size == 0 || !in_) return;

    std::uintmax_t blocks = (size + 511) / 512;
    std::uintmax_t to_skip = blocks * 512;
    in_->seekg(to_skip, std::ios::cur);
}

void reader::extract_file(const fs::path& path, std::uintmax_t size) {
    if (!in_) {
        throw std::runtime_error("No data source is open");
    }

    // 创建父目录
    fs::create_directories(path.parent_path());

    // 创建文件
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Cannot create file: " + path.string());
    }

    // 读取文件数据
    const std::size_t buffer_size = 8192;
    char buffer[buffer_size];
    std::uintmax_t remaining = size;

    while (remaining > 0) {
        std::size_t to_read = static_cast<std::size_t>(
            std::min<std::uintmax_t>(buffer_size, remaining));

        in_->read(buffer, to_read);
        std::streamsize bytes_read = in_->gcount();

        if (bytes_read <= 0) {
            throw std::runtime_error("Failed to read file data");
        }

        out.write(buffer, bytes_read);
        if (!out) {
            throw std::runtime_error("Failed to write file");
        }

        remaining -= bytes_read;
    }

    // 跳过填充字节
    std::size_t padding = (512 - (size % 512)) % 512;
    if (padding > 0) {
        in_->seekg(padding, std::ios::cur);
    }
}

void reader::extract_all(const fs::path& output_dir) {
    if (!in_) {
        throw std::runtime_error("No data source is open");
    }

    // 保存当前流位置
    std::streampos original_pos = in_->tellg();
    in_->seekg(0, std::ios::beg);

    header hdr;

    try {
        while (read_header(hdr)) {
            // 获取文件名
            std::string filename = get_path(hdr);
            if (filename.empty()) {
                std::uintmax_t size = parse_octal(hdr.size, sizeof(hdr.size));
                skip_data(size);
                continue;
            }

            // 获取文件大小
            std::uintmax_t size = parse_octal(hdr.size, sizeof(hdr.size));

            // 获取文件类型
            char typeflag = hdr.typeflag;

            // 构建完整路径
            fs::path full_path = output_dir / filename;

            try {
                switch (typeflag) {
                    case '0':   // 普通文件
                    case '\0':  // 旧格式的普通文件
                        extract_file(full_path, size);
                        break;

                    case '5':  // 目录
                        fs::create_directories(full_path);
                        if (size > 0) {
                            skip_data(size);
                        }
                        break;

                    default:
                        // 跳过不支持的文件类型
                        skip_data(size);
                        break;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error extracting " << filename << ": " << e.what()
                          << std::endl;
                skip_data(size);
            }
        }
    } catch (...) {
        // 恢复流位置
        in_->seekg(original_pos);
        throw;
    }

    // 恢复流位置以便后续读取
    in_->seekg(original_pos);
}

void reader::list() {
    if (!in_) {
        throw std::runtime_error("No data source is open");
    }

    // 保存当前流位置
    std::streampos original_pos = in_->tellg();
    in_->seekg(0, std::ios::beg);

    header hdr;

    std::cout << "Type  Size      Modified             Name\n";
    std::cout << "----  --------  -------------------  ----\n";

    try {
        while (read_header(hdr)) {
            // 获取文件名
            std::string filename = get_path(hdr);
            if (filename.empty()) {
                std::uintmax_t size = parse_octal(hdr.size, sizeof(hdr.size));
                skip_data(size);
                continue;
            }

            // 获取文件大小
            std::uintmax_t size = parse_octal(hdr.size, sizeof(hdr.size));

            // 获取文件类型
            char typeflag = hdr.typeflag;

            // 获取修改时间
            std::uintmax_t mtime_val =
                parse_octal(hdr.mtime, sizeof(hdr.mtime));
            std::time_t mtime = static_cast<std::time_t>(mtime_val);
            char time_buf[20];
            std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S",
                          std::localtime(&mtime));

            // 输出文件信息
            std::cout << (typeflag == '5' ? "d" : "-") << "  " << std::setw(8)
                      << size << "  " << time_buf << "  " << filename
                      << std::endl;

            // 跳过数据
            skip_data(size);
        }
    } catch (...) {
        // 恢复流位置
        in_->seekg(original_pos);
        throw;
    }

    // 恢复流位置
    in_->seekg(original_pos);
}

// 便捷函数定义
void create_archive(const fs::path& archive_path,
                    const std::vector<fs::path>& files) {
    writer w;
    for (const auto& file : files) {
        w.add_file(file);
    }
    w.write_to_file(archive_path);
}

void create_archive_from_directory(const fs::path& archive_path,
                                   const fs::path& directory,
                                   const std::string& tar_name) {
    writer w;

    // 如果指定了tar中的名称，使用它，否则使用目录名
    std::string name =
        tar_name.empty() ? directory.filename().string() : tar_name;
    w.add_directory(directory, name);
    w.write_to_file(archive_path);
}

void extract_archive(const fs::path& archive_path, const fs::path& output_dir) {
    reader r(archive_path);
    r.extract_all(output_dir);
}

void extract_archive_from_memory(const std::string& data,
                                 const fs::path& output_dir) {
    reader r(data);
    r.extract_all(output_dir);
}

void extract_archive_from_memory(const std::vector<char>& data,
                                 const fs::path& output_dir) {
    reader r(data);
    r.extract_all(output_dir);
}

void list_archive(const fs::path& archive_path) {
    reader r(archive_path);
    r.list();
}

void list_archive_from_memory(const std::string& data) {
    reader r(data);
    r.list();
}

void list_archive_from_memory(const std::vector<char>& data) {
    reader r(data);
    r.list();
}

}  // namespace tar