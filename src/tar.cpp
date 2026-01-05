#include "tar/tar.hpp"  // 包含tar库的头文件

#include <algorithm>   // 用于算法操作，如std::replace
#include <cctype>      // 用于字符分类
#include <chrono>      // 用于时间处理
#include <cstring>     // 用于字符串操作
#include <ctime>       // 用于时间函数
#include <filesystem>  // 用于文件系统操作
#include <fstream>     // 用于文件流操作
#include <iomanip>     // 用于输出格式控制
#include <iostream>    // 用于输入输出
#include <limits>      // 用于类型限制
#include <memory>      // 用于智能指针
#include <sstream>     // 用于字符串流
#include <string>      // 用于字符串操作
#include <vector>      // 用于向量容器

namespace tar {                  // tar命名空间
namespace chrono = std::chrono;  // 为std::chrono创建别名

// 工具函数：解析八进制字符串
// 从字符数组中解析八进制字符串，跳过前导空格，遇到非八进制字符或空格时停止
// 参数：data - 指向字符数组的指针
//       size - 字符数组的大小
// 返回值：解析后的无符号整数值
std::uintmax_t parse_octal(const char* data, std::size_t size) {
    std::uintmax_t result = 0;  // 存储解析结果的变量
    bool started = false;       // 标记是否已经开始解析有效数字

    for (std::size_t i = 0; i < size; ++i) {  // 遍历字符数组
        char c = data[i];                     // 获取当前字符
        if (c == 0) break;                    // 遇到空字符则结束解析
        if (c == ' ') {                       // 处理空格
            if (started) break;  // 如果已经开始解析，遇到空格表示数字结束
            continue;            // 跳过前导空格
        }
        if (c < '0' || c > '7') break;  // 遇到非八进制字符则结束解析

        started = true;                   // 标记已开始解析有效数字
        result = result * 8 + (c - '0');  // 八进制转换：乘以8加上当前数字
    }
    return result;  // 返回解析结果
}

// 工具函数：将整数写入为八进制字符串
// 将无符号整数转换为八进制字符串，右对齐存储在缓冲区中
// 参数：value - 要转换的无符号整数值
//       buffer - 输出缓冲区
//       size - 缓冲区大小
void write_octal(std::uintmax_t value, char* buffer, std::size_t size) {
    std::memset(buffer, ' ', size);  // 用空格填充整个缓冲区
    buffer[size - 1] = 0;            // 在末尾添加空字符终止符

    if (value == 0) {            // 处理值为0的特殊情况
        buffer[size - 2] = '0';  // 在倒数第二位置写入'0'
        return;
    }

    char temp[32];                      // 临时缓冲区用于存储反向的八进制字符串
    char* p = temp + sizeof(temp) - 1;  // 指向临时缓冲区末尾
    *p = 0;                             // 设置字符串终止符

    do {                           // 从最低位开始生成八进制数字
        *--p = '0' + (value % 8);  // 获取当前八进制数字并转换为字符
        value /= 8;                // 移除已处理的数字
    } while (value > 0);  // 继续处理直到值为0

    std::size_t len = std::strlen(p);    // 获取生成的八进制字符串长度
    if (len > size - 1) len = size - 1;  // 确保长度不超过缓冲区容量

    // 将八进制字符串复制到输出缓冲区，右对齐
    std::memcpy(buffer + size - len - 1, p, len);
}

// 工具函数：计算tar头部校验和
// 根据tar规范计算头部的校验和，校验和字段本身用空格计算
// 参数：hdr - tar头部结构体引用
// 返回值：计算得到的校验和
unsigned int calculate_checksum(const header& hdr) {
    const unsigned char* p =
        reinterpret_cast<const unsigned char*>(&hdr);  // 将头部转换为字节数组
    unsigned int sum = 0;                              // 校验和累加器

    for (std::size_t i = 0; i < sizeof(header); ++i) {  // 遍历头部所有字节
        if (i >= offsetof(header, chksum) && i < offsetof(header, chksum) + 8) {
            sum += ' ';  // 校验和字段本身用空格计算
        } else {
            sum += p[i];  // 其他字段用实际字节值计算
        }
    }
    return sum;  // 返回计算得到的校验和
}

// 工具函数：检查头部块是否全为零
// 用于检测tar文件的结束标记（两个连续的零块）
// 参数：hdr - tar头部结构体引用
// 返回值：如果所有字节都为0则返回true，否则返回false
bool is_zero_block(const header& hdr) {
    const unsigned char* p =
        reinterpret_cast<const unsigned char*>(&hdr);   // 将头部转换为字节数组
    for (std::size_t i = 0; i < sizeof(header); ++i) {  // 遍历头部所有字节
        if (p[i] != 0) return false;  // 发现非零字节则返回false
    }
    return true;  // 所有字节都为0则返回true
}

// writer类构造函数
// 初始化内部字符串流，并设置异常处理
writer::writer() {
    out_.exceptions(std::ios::badbit |
                    std::ios::failbit);  // 设置字符串流异常处理
}

// writer类析构函数
// 如果还有未完成的数据，自动调用finish()完成打包
writer::~writer() {
    if (!out_.str().empty()) {  // 检查字符串流是否包含数据
        try {
            finish();  // 完成打包，添加结束标记
        } catch (...) {
            // 忽略析构函数中的异常，避免异常逃离析构函数
        }
    }
}

// writer类私有方法：写入文件数据
// 从文件中读取数据并写入到tar流中，自动处理512字节对齐
// 参数：path - 源文件路径
//       size - 文件大小
void writer::write_file_data(const fs::path& path, std::uintmax_t size) {
    if (size == 0) return;  // 空文件无需处理

    std::ifstream in(path, std::ios::binary);  // 以二进制模式打开源文件
    if (!in) {                                 // 检查文件是否成功打开
        throw std::runtime_error("Cannot open file: " + path.string());
    }

    const std::size_t buffer_size = 8192;  // 设置读取缓冲区大小（8KB）
    char buffer[buffer_size];              // 缓冲区数组
    std::uintmax_t remaining = size;       // 剩余要读取的字节数

    while (remaining > 0) {  // 循环读取直到处理完所有数据
        // 计算本次要读取的字节数
        std::size_t to_read = static_cast<std::size_t>(
            std::min<std::uintmax_t>(buffer_size, remaining));

        in.read(buffer, to_read);                  // 从源文件读取数据
        std::streamsize bytes_read = in.gcount();  // 获取实际读取的字节数

        if (bytes_read <= 0) {  // 检查读取是否成功
            throw std::runtime_error("Failed to read file: " + path.string());
        }

        out_.write(buffer, bytes_read);  // 将数据写入tar流
        if (!out_) {                     // 检查写入是否成功
            throw std::runtime_error("Failed to write to archive");
        }

        remaining -= bytes_read;  // 减少剩余字节数
    }

    // 填充到512字节边界
    // tar格式要求每个文件数据块必须是512字节的倍数
    std::size_t padding = (512 - (size % 512)) % 512;  // 计算需要的填充字节数
    if (padding > 0) {                                 // 如果需要填充
        std::memset(buffer, 0, padding);               // 用零填充缓冲区
        out_.write(buffer, padding);                   // 写入填充数据
    }
}

// writer类私有方法：写入目录条目
// 创建并写入目录的tar头部信息
// 参数：dir_name - 在tar中的目录名
void writer::write_directory_entry(const std::string& dir_name) {
    header hdr = {};  // 初始化头部结构体，所有字段置零

    // 确保目录名以/结尾，这是tar格式的约定
    std::string rel_path = dir_name;
    if (!rel_path.empty() && rel_path.back() != '/') {
        rel_path.push_back('/');
    }

    if (rel_path.empty()) {  // 处理空路径的情况
        rel_path = "./";
    }

    // 处理长路径：如果路径超过100字节，使用prefix字段
    if (rel_path.size() > 100) {
        std::size_t last_slash =
            rel_path.find_last_of('/');  // 查找最后一个斜杠
        if (last_slash != std::string::npos && last_slash <= 155) {
            std::string prefix =
                rel_path.substr(0, last_slash);  // 提取前缀部分
            std::string name =
                rel_path.substr(last_slash + 1);  // 提取文件名部分

            if (name.size() <= 100 &&
                prefix.size() <= 155) {  // 检查两部分是否都符合长度限制
                std::strncpy(hdr.prefix, prefix.c_str(), 155);  // 复制前缀
                std::strncpy(hdr.name, name.c_str(), 100);      // 复制文件名
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
        std::strncpy(hdr.name, rel_path.c_str(), 100);  // 普通路径直接复制
    }

    // 设置目录权限：0755表示所有者有全部权限，组和其他用户有读和执行权限
    write_octal(0755, hdr.mode, sizeof(hdr.mode));

    // 设置用户和组ID，这里使用0（root用户）
    write_octal(0, hdr.uid, sizeof(hdr.uid));
    write_octal(0, hdr.gid, sizeof(hdr.gid));

    // 目录大小为0
    write_octal(0, hdr.size, sizeof(hdr.size));

    // 设置修改时间为当前时间
    auto time = std::time(nullptr);
    write_octal(static_cast<std::uintmax_t>(time), hdr.mtime,
                sizeof(hdr.mtime));

    // 设置文件类型标志：'5'表示目录
    hdr.typeflag = '5';

    // 设置魔术字和版本，标识ustar格式
    std::memcpy(hdr.magic, "ustar", 5);
    hdr.magic[5] = 0;      // 字符串终止符
    hdr.version[0] = '0';  // 主版本号
    hdr.version[1] = '0';  // 次版本号

    // 设置用户名和组名
    std::strncpy(hdr.uname, "user", 32);
    std::strncpy(hdr.gname, "group", 32);

    // 计算校验和并写入
    unsigned int checksum = calculate_checksum(hdr);
    write_octal(checksum, hdr.chksum, sizeof(hdr.chksum));

    // 写入头部到输出流
    out_.write(reinterpret_cast<const char*>(&hdr), sizeof(header));
    if (!out_) {  // 检查写入是否成功
        throw std::runtime_error("Failed to write header");
    }
}

// writer类私有方法：写入文件条目
// 创建并写入文件的tar头部信息和文件数据
// 参数：file_path - 在tar中的文件路径
//       real_path - 实际文件系统路径
void writer::write_file_entry(const std::string& file_path,
                              const fs::path& real_path) {
    header hdr = {};  // 初始化头部结构体

    // 处理路径分隔符：将Windows风格的反斜杠替换为Unix风格的正斜杠
    std::string rel_path = file_path;
    std::replace(rel_path.begin(), rel_path.end(), '\\', '/');

    // 处理长路径（与目录处理类似）
    if (rel_path.size() > 100) {
        std::size_t last_slash = rel_path.find_last_of('/');
        if (last_slash != std::string::npos && last_slash <= 155) {
            std::string prefix = rel_path.substr(0, last_slash);
            std::string name = rel_path.substr(last_slash + 1);

            if (name.size() <= 100 && prefix.size() <= 155) {
                std::strncpy(hdr.prefix, prefix.c_str(), 155);
                std::strncpy(hdr.name, name.c_str(), 100);
            } else {
                std::strncpy(hdr.name, name.c_str(), 100);
            }
        } else {
            std::size_t start =
                rel_path.size() > 100 ? rel_path.size() - 100 : 0;
            std::strncpy(hdr.name, rel_path.c_str() + start, 100);
        }
    } else {
        std::strncpy(hdr.name, rel_path.c_str(), 100);
    }

    // 设置文件权限：0644表示所有者有读写权限，组和其他用户有读权限
    write_octal(0644, hdr.mode, sizeof(hdr.mode));

    // 设置用户和组ID
    write_octal(0, hdr.uid, sizeof(hdr.uid));
    write_octal(0, hdr.gid, sizeof(hdr.gid));

    // 设置文件大小
    std::uintmax_t size = fs::file_size(real_path);
    write_octal(size, hdr.size, sizeof(hdr.size));

    // 设置修改时间
    auto mtime = fs::last_write_time(real_path);  // 获取文件最后修改时间
    // 转换文件时间类型为system_clock时间
    auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(
        mtime - fs::file_time_type::clock::now() + chrono::system_clock::now());
    auto time = chrono::system_clock::to_time_t(sctp);  // 转换为time_t
    write_octal(static_cast<std::uintmax_t>(time), hdr.mtime,
                sizeof(hdr.mtime));

    // 设置文件类型标志：'0'表示普通文件
    hdr.typeflag = '0';

    // 设置魔术字和版本
    std::memcpy(hdr.magic, "ustar", 5);
    hdr.magic[5] = 0;
    hdr.version[0] = '0';
    hdr.version[1] = '0';

    // 设置用户名和组名
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

// writer类私有方法：递归添加目录内容
// 递归遍历目录，添加所有子目录和文件
// 参数：dir_path - 要添加的目录路径
//       base_name - 在tar中的基路径
void writer::add_directory_recursive(const fs::path& dir_path,
                                     const std::string& base_name) {
    // 使用递归目录迭代器遍历目录
    for (const auto& entry : fs::recursive_directory_iterator(
             dir_path, fs::directory_options::skip_permission_denied)) {
        try {
            // 计算相对路径，用于构建tar中的路径
            fs::path relative = fs::relative(entry.path(), dir_path);
            std::string tar_path = base_name + "/" + relative.string();
            std::replace(tar_path.begin(), tar_path.end(), '\\', '/');

            if (fs::is_directory(entry.status())) {  // 处理目录
                if (!tar_path.empty() && tar_path.back() != '/') {
                    tar_path.push_back('/');  // 确保目录名以/结尾
                }
                write_directory_entry(tar_path);               // 写入目录条目
            } else if (fs::is_regular_file(entry.status())) {  // 处理普通文件
                write_file_entry(tar_path, entry.path());      // 写入文件条目
            }
        } catch (const std::exception& e) {  // 捕获处理单个条目时的异常
            std::cerr << "Warning: Skipping " << entry.path().string() << ": "
                      << e.what() << std::endl;  // 输出警告信息
        }
    }
}

// writer类公有方法：添加文件
// 将单个文件添加到tar中
// 参数：file_path - 要添加的文件路径
//       tar_name - 在tar中的自定义名称
void writer::add_file(const fs::path& file_path, const std::string& tar_name) {
    if (!fs::exists(file_path)) {  // 检查文件是否存在
        throw std::runtime_error("File not found: " + file_path.string());
    }

    if (!fs::is_regular_file(file_path)) {  // 检查是否为普通文件
        throw std::runtime_error("Not a regular file: " + file_path.string());
    }

    // 确定tar中的文件名：如果提供了自定义名称则使用，否则使用原文件名
    std::string name =
        tar_name.empty() ? file_path.filename().string() : tar_name;
    write_file_entry(name, file_path);  // 写入文件条目
}

// writer类公有方法：添加目录
// 将整个目录（包括子目录和文件）添加到tar中
// 参数：dir_path - 要添加的目录路径
//       tar_name - 在tar中的自定义名称
void writer::add_directory(const fs::path& dir_path,
                           const std::string& tar_name) {
    if (!fs::exists(dir_path)) {  // 检查目录是否存在
        throw std::runtime_error("Directory not found: " + dir_path.string());
    }

    if (!fs::is_directory(dir_path)) {  // 检查是否为目录
        throw std::runtime_error("Not a directory: " + dir_path.string());
    }

    // 确定tar中的目录名
    std::string name =
        tar_name.empty() ? dir_path.filename().string() : tar_name;

    write_directory_entry(name + "/");        // 写入顶层目录条目
    add_directory_recursive(dir_path, name);  // 递归添加目录内容
}

// writer类私有方法：完成打包
// 写入两个全零块作为tar文件的结束标记
void writer::finish() {
    char zero_block[1024] = {0};   // 创建1024字节的零块（两个512字节块）
    out_.write(zero_block, 1024);  // 写入结束标记
    out_.flush();                  // 刷新输出流
}

// writer类公有方法：获取数据为字符串
// 返回tar数据的字符串表示
std::string writer::get_data() const { return out_.str(); }

// writer类公有方法：获取数据为字符向量
// 返回tar数据的字符向量表示
std::vector<char> writer::get_vector() const {
    std::string data = out_.str();
    return std::vector<char>(data.begin(), data.end());  // 从字符串构造向量
}

// writer类公有方法：写入到文件
// 将tar数据写入到指定文件中
// 参数：file_path - 输出文件路径
void writer::write_to_file(const fs::path& file_path) {
    finish();  // 确保打包完成

    std::ofstream file(file_path,
                       std::ios::binary);  // 以二进制模式打开输出文件
    if (!file) {                           // 检查文件是否成功打开
        throw std::runtime_error("Cannot open file for writing: " +
                                 file_path.string());
    }

    std::string data = out_.str();         // 获取tar数据
    file.write(data.data(), data.size());  // 写入数据

    if (!file) {  // 检查写入是否成功
        throw std::runtime_error("Failed to write to file: " +
                                 file_path.string());
    }
}

// writer类公有方法：清空数据
// 清空内部字符串流，以便重用writer对象
void writer::clear() {
    out_.str("");  // 清空字符串流内容
    out_.clear();  // 清除错误状态
}

// writer类公有方法：获取数据大小
// 返回当前tar数据的大小（字节数）
std::size_t writer::size() const { return out_.str().size(); }

// writer类公有方法：检查是否为空
// 检查是否有tar数据
bool writer::empty() const { return out_.str().empty(); }

// reader类构造函数：从文件路径构造
// 参数：archive_path - tar文件路径
reader::reader(const fs::path& archive_path) { set_source(archive_path); }

// reader类构造函数：从输入流构造
// 参数：stream - 输入流智能指针
reader::reader(std::unique_ptr<std::istream> stream) {
    set_source(std::move(stream));
}

// reader类构造函数：从字符串构造
// 参数：data - 包含tar数据的字符串
reader::reader(const std::string& data) { set_source(data); }

// reader类构造函数：从字符向量构造
// 参数：data - 包含tar数据的字符向量
reader::reader(const std::vector<char>& data) { set_source(data); }

// reader类构造函数：从原始指针构造
// 参数：data - 指向tar数据的指针
//       size - 数据大小
reader::reader(const char* data, std::size_t size) { set_source(data, size); }

// reader类移动构造函数
// 参数：other - 要移动的reader对象
reader::reader(reader&& other) noexcept
    : file_stream_(std::move(other.file_stream_)),      // 移动文件流
      memory_stream_(std::move(other.memory_stream_)),  // 移动内存流
      in_(other.in_) {                                  // 复制流指针
    other.in_ = nullptr;  // 将原对象的流指针置为空
}

// reader类移动赋值运算符
// 参数：other - 要移动的reader对象
reader& reader::operator=(reader&& other) noexcept {
    if (this != &other) {                                  // 避免自赋值
        cleanup();                                         // 清理当前对象的资源
        file_stream_ = std::move(other.file_stream_);      // 移动文件流
        memory_stream_ = std::move(other.memory_stream_);  // 移动内存流
        in_ = other.in_;                                   // 复制流指针
        other.in_ = nullptr;  // 将原对象的流指针置为空
    }
    return *this;  // 返回当前对象的引用
}

// reader类公有方法：设置输入流源
// 设置数据源为输入流
// 参数：stream - 输入流智能指针
void reader::set_source(std::unique_ptr<std::istream> stream) {
    cleanup();                           // 清理现有资源
    memory_stream_ = std::move(stream);  // 移动输入流
    in_ = memory_stream_.get();          // 设置当前流指针
    if (in_) {                           // 如果输入流有效
        // 检查流是否有效
        in_->seekg(0, std::ios::end);         // 移动到流末尾
        std::streamsize size = in_->tellg();  // 获取流大小
        in_->seekg(0, std::ios::beg);         // 移回流开头

        if (size < 512) {  // 最小的tar文件至少有1个头部块
            throw std::runtime_error("Invalid tar data: data too small");
        }

        if (!*in_) {  // 检查流状态
            throw std::runtime_error("Failed to set stream source");
        }
    }
}

// reader类公有方法：设置字符串源
// 设置数据源为字符串
// 参数：data - 包含tar数据的字符串
void reader::set_source(const std::string& data) {
    if (data.size() < 1024) {  // 最小的完整tar文件有两个零块
        throw std::runtime_error("Invalid tar data: data too small");
    }
    set_source(std::make_unique<std::istringstream>(data));  // 创建字符串流
}

// reader类公有方法：设置字符向量源
// 设置数据源为字符向量
// 参数：data - 包含tar数据的字符向量
void reader::set_source(const std::vector<char>& data) {
    if (data.size() < 1024) {  // 检查数据大小
        throw std::runtime_error("Invalid tar data: data too small");
    }
    set_source(std::make_unique<std::istringstream>(
        std::string(data.begin(), data.end())));  // 从向量创建字符串流
}

// reader类公有方法：设置原始指针源
// 设置数据源为原始指针
// 参数：data - 指向tar数据的指针
//       size - 数据大小
void reader::set_source(const char* data, std::size_t size) {
    if (size < 1024) {  // 检查数据大小
        throw std::runtime_error("Invalid tar data: data too small");
    }
    set_source(std::string(data, data + size));  // 从指针创建字符串
}

// reader类公有方法：设置文件源
// 设置数据源为文件
// 参数：archive_path - tar文件路径
void reader::set_source(const fs::path& archive_path) {
    cleanup();  // 清理现有资源
    file_stream_ =
        std::make_unique<std::ifstream>(archive_path, std::ios::binary);
    if (!*file_stream_) {  // 检查文件是否成功打开
        throw std::runtime_error("Cannot open archive: " +
                                 archive_path.string());
    }

    // 检查文件大小
    file_stream_->seekg(0, std::ios::end);         // 移动到文件末尾
    std::streamsize size = file_stream_->tellg();  // 获取文件大小
    file_stream_->seekg(0, std::ios::beg);         // 移回文件开头

    if (size < 1024) {  // 检查文件大小是否有效
        throw std::runtime_error("Invalid tar archive: file too small");
    }

    in_ = file_stream_.get();  // 设置当前流指针
}

// reader类公有方法：检查是否已打开
// 返回是否有有效的输入流
bool reader::is_open() const { return in_ != nullptr && *in_; }

// reader类公有方法：关闭数据源
// 关闭当前数据源
void reader::close() { cleanup(); }

// reader类私有方法：清理资源
// 释放所有流资源
void reader::cleanup() {
    file_stream_.reset();    // 释放文件流
    memory_stream_.reset();  // 释放内存流
    in_ = nullptr;           // 清空流指针
}

// reader类私有方法：读取头部
// 从输入流读取一个tar头部
// 参数：hdr - 用于存储头部数据的引用
// 返回值：成功读取返回true，遇到结束标记返回false
bool reader::read_header(header& hdr) {
    if (!in_) {  // 检查是否有有效的输入流
        throw std::runtime_error("No data source is open");
    }

    in_->read(reinterpret_cast<char*>(&hdr),
              sizeof(header));                   // 读取512字节头部
    std::streamsize bytes_read = in_->gcount();  // 获取实际读取的字节数

    if (!*in_ || bytes_read != sizeof(header)) {  // 检查读取是否成功
        return false;  // 读取失败或读取的字节数不正确
    }

    // 检查是否为零块（tar文件结束标记）
    if (is_zero_block(hdr)) {
        // 再读一个头部，检查是否连续两个零块
        header next_hdr;
        in_->read(reinterpret_cast<char*>(&next_hdr), sizeof(header));
        if (is_zero_block(next_hdr)) {
            return false;  // 两个连续零块，文件结束
        }
        // 只有一个零块，回退位置
        in_->seekg(-static_cast<std::streamoff>(sizeof(header)), std::ios::cur);
    }

    return true;  // 成功读取头部
}

// reader类私有方法：获取完整路径
// 从头部组合出完整路径
// 参数：hdr - tar头部
// 返回值：组合后的完整路径
std::string reader::get_path(const header& hdr) {
    std::string path;  // 存储完整路径

    // 检查前缀字段
    if (hdr.prefix[0] != 0) {                      // 前缀字段不为空
        std::string prefix = hdr.prefix;           // 复制前缀
        std::size_t null_pos = prefix.find('\0');  // 查找空字符终止符
        if (null_pos != std::string::npos) {
            prefix.resize(null_pos);  // 截断到空字符
        }
        if (!prefix.empty()) {    // 前缀不为空
            path = prefix + "/";  // 添加斜杠分隔符
        }
    }

    // 添加文件名
    std::string name = hdr.name;             // 复制文件名
    std::size_t null_pos = name.find('\0');  // 查找空字符终止符
    if (null_pos != std::string::npos) {
        name.resize(null_pos);  // 截断到空字符
    }
    path += name;  // 组合完整路径

    return path;  // 返回完整路径
}

// reader类私有方法：跳过文件数据
// 跳过指定大小的文件数据
// 参数：size - 要跳过的数据大小
void reader::skip_data(std::uintmax_t size) {
    if (size == 0 || !in_) return;  // 不需要跳过或没有输入流

    std::uintmax_t blocks = (size + 511) / 512;  // 计算数据块数（向上取整）
    std::uintmax_t to_skip = blocks * 512;       // 计算要跳过的总字节数
    in_->seekg(to_skip, std::ios::cur);          // 在输入流中跳过
}

// reader类私有方法：提取文件
// 从输入流中提取文件数据并写入到文件系统
// 参数：path - 目标文件路径
//       size - 文件大小
void reader::extract_file(const fs::path& path, std::uintmax_t size) {
    if (!in_) {  // 检查输入流
        throw std::runtime_error("No data source is open");
    }

    // 创建父目录
    fs::create_directories(path.parent_path());

    // 创建输出文件
    std::ofstream out(path, std::ios::binary);
    if (!out) {  // 检查文件是否成功创建
        throw std::runtime_error("Cannot create file: " + path.string());
    }

    // 读取文件数据
    const std::size_t buffer_size = 8192;  // 缓冲区大小
    char buffer[buffer_size];
    std::uintmax_t remaining = size;  // 剩余要读取的字节数

    while (remaining > 0) {  // 循环读取
        std::size_t to_read = static_cast<std::size_t>(std::min<std::uintmax_t>(
            buffer_size, remaining));  // 计算本次读取大小

        in_->read(buffer, to_read);                  // 从输入流读取
        std::streamsize bytes_read = in_->gcount();  // 获取实际读取字节数

        if (bytes_read <= 0) {  // 检查读取是否成功
            throw std::runtime_error("Failed to read file data");
        }

        out.write(buffer, bytes_read);  // 写入到输出文件
        if (!out) {                     // 检查写入是否成功
            throw std::runtime_error("Failed to write file");
        }

        remaining -= bytes_read;  // 更新剩余字节数
    }

    // 跳过填充字节
    std::size_t padding = (512 - (size % 512)) % 512;  // 计算填充字节数
    if (padding > 0) {
        in_->seekg(padding, std::ios::cur);  // 跳过填充
    }
}

// reader类公有方法：提取所有文件
// 从tar中提取所有文件到指定目录
// 参数：output_dir - 输出目录
void reader::extract_all(const fs::path& output_dir) {
    if (!in_) {  // 检查输入流
        throw std::runtime_error("No data source is open");
    }

    // 保存当前流位置，以便恢复
    std::streampos original_pos = in_->tellg();
    in_->seekg(0, std::ios::beg);  // 移动到流开头

    header hdr;  // 用于存储头部

    try {
        while (read_header(hdr)) {  // 循环读取头部
            // 获取文件名
            std::string filename = get_path(hdr);
            if (filename.empty()) {  // 文件名为空
                std::uintmax_t size = parse_octal(hdr.size, sizeof(hdr.size));
                skip_data(size);  // 跳过数据
                continue;         // 继续下一个条目
            }

            // 获取文件大小
            std::uintmax_t size = parse_octal(hdr.size, sizeof(hdr.size));

            // 获取文件类型
            char typeflag = hdr.typeflag;

            // 构建完整路径
            fs::path full_path = output_dir / filename;

            try {
                switch (typeflag) {  // 根据文件类型处理
                    case '0':        // 普通文件
                    case '\0':       // 旧格式的普通文件
                        extract_file(full_path, size);
                        break;

                    case '5':                               // 目录
                        fs::create_directories(full_path);  // 创建目录
                        if (size > 0) {  // 目录大小应为0，但安全起见
                            skip_data(size);
                        }
                        break;

                    default:              // 不支持的文件类型
                        skip_data(size);  // 跳过数据
                        break;
                }
            } catch (const std::exception& e) {  // 处理单个文件时的异常
                std::cerr << "Error extracting " << filename << ": " << e.what()
                          << std::endl;  // 输出错误信息
                skip_data(size);         // 跳过当前文件的数据
            }
        }
    } catch (...) {                // 捕获所有异常
        in_->seekg(original_pos);  // 恢复流位置
        throw;                     // 重新抛出异常
    }

    in_->seekg(original_pos);  // 恢复流位置
}

// reader类公有方法：列出内容
// 列出tar文件中的所有条目
void reader::list() {
    if (!in_) {  // 检查输入流
        throw std::runtime_error("No data source is open");
    }

    // 保存当前流位置
    std::streampos original_pos = in_->tellg();
    in_->seekg(0, std::ios::beg);  // 移动到流开头

    header hdr;  // 用于存储头部

    // 输出表头
    std::cout << "Type  Size      Modified             Name\n";
    std::cout << "----  --------  -------------------  ----\n";

    try {
        while (read_header(hdr)) {  // 循环读取头部
            // 获取文件名
            std::string filename = get_path(hdr);
            if (filename.empty()) {  // 文件名为空
                std::uintmax_t size = parse_octal(hdr.size, sizeof(hdr.size));
                skip_data(size);  // 跳过数据
                continue;         // 继续下一个条目
            }

            // 获取文件大小
            std::uintmax_t size = parse_octal(hdr.size, sizeof(hdr.size));

            // 获取文件类型
            char typeflag = hdr.typeflag;

            // 获取修改时间
            std::uintmax_t mtime_val =
                parse_octal(hdr.mtime, sizeof(hdr.mtime));
            std::time_t mtime = static_cast<std::time_t>(mtime_val);
            char time_buf[20];  // 时间格式化缓冲区
            std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S",
                          std::localtime(&mtime));  // 格式化时间

            // 输出文件信息
            std::cout << (typeflag == '5' ? "d" : "-") << "  " << std::setw(8)
                      << size << "  " << time_buf << "  " << filename
                      << std::endl;

            skip_data(size);  // 跳过文件数据
        }
    } catch (...) {                // 捕获异常
        in_->seekg(original_pos);  // 恢复流位置
        throw;                     // 重新抛出异常
    }

    in_->seekg(original_pos);  // 恢复流位置
}

// 便捷函数：创建归档文件
// 从多个文件创建tar归档文件
// 参数：archive_path - 输出归档文件路径
//       files - 要包含的文件列表
void create_archive(const fs::path& archive_path,
                    const std::vector<fs::path>& files) {
    writer w;                         // 创建writer对象
    for (const auto& file : files) {  // 遍历文件列表
        w.add_file(file);             // 添加文件
    }
    w.write_to_file(archive_path);  // 写入到文件
}

// 便捷函数：从目录创建归档文件
// 从整个目录创建tar归档文件
// 参数：archive_path - 输出归档文件路径
//       directory - 要打包的目录
//       tar_name - 在tar中的目录名
void create_archive_from_directory(const fs::path& archive_path,
                                   const fs::path& directory,
                                   const std::string& tar_name) {
    writer w;  // 创建writer对象

    // 确定tar中的目录名
    std::string name =
        tar_name.empty() ? directory.filename().string() : tar_name;
    w.add_directory(directory, name);  // 添加目录
    w.write_to_file(archive_path);     // 写入到文件
}

// 便捷函数：提取归档文件
// 从文件提取tar归档
// 参数：archive_path - 输入归档文件路径
//       output_dir - 输出目录
void extract_archive(const fs::path& archive_path, const fs::path& output_dir) {
    reader r(archive_path);     // 创建reader对象
    r.extract_all(output_dir);  // 提取所有文件
}

// 便捷函数：从内存提取归档
// 从字符串中提取tar归档
// 参数：data - 包含tar数据的字符串
//       output_dir - 输出目录
void extract_archive_from_memory(const std::string& data,
                                 const fs::path& output_dir) {
    reader r(data);             // 从字符串创建reader
    r.extract_all(output_dir);  // 提取所有文件
}

// 便捷函数：从内存提取归档
// 从字符向量中提取tar归档
// 参数：data - 包含tar数据的字符向量
//       output_dir - 输出目录
void extract_archive_from_memory(const std::vector<char>& data,
                                 const fs::path& output_dir) {
    reader r(data);             // 从字符向量创建reader
    r.extract_all(output_dir);  // 提取所有文件
}

// 便捷函数：列出归档内容
// 列出tar归档文件的内容
// 参数：archive_path - 归档文件路径
void list_archive(const fs::path& archive_path) {
    reader r(archive_path);  // 从文件创建reader
    r.list();                // 列出内容
}

// 便捷函数：从内存列出归档内容
// 从字符串列出tar归档内容
// 参数：data - 包含tar数据的字符串
void list_archive_from_memory(const std::string& data) {
    reader r(data);  // 从字符串创建reader
    r.list();        // 列出内容
}

// 便捷函数：从内存列出归档内容
// 从字符向量列出tar归档内容
// 参数：data - 包含tar数据的字符向量
void list_archive_from_memory(const std::vector<char>& data) {
    reader r(data);  // 从字符向量创建reader
    r.list();        // 列出内容
}

}  // namespace tar