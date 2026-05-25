#ifndef TAR_H
#define TAR_H

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace tar {
namespace fs = std::filesystem;  // 为std::filesystem创建别名，简化代码

// tar 文件头结构（遵循 POSIX ustar 格式规范）
// 每个 tar 头部块为 512 字节，包含文件的元数据信息
struct header {
    char name[100];      // 文件名（最大 100 字符）
    char mode[8];        // 文件权限（八进制字符串表示）
    char uid[8];         // 用户 ID（八进制字符串表示）
    char gid[8];         // 组 ID（八进制字符串表示）
    char size[12];       // 文件大小（八进制字符串表示）
    char mtime[12];      // 修改时间（八进制字符串，Unix 时间戳）
    char chksum[8];      // 头部校验和（八进制字符串表示）
    char typeflag;       // 文件类型标志：0=普通文件，5=目录，其他类型见规范
    char linkname[100];  // 链接目标（仅用于符号链接和硬链接）
    char magic[6];       // 魔术字 "ustar" 表示 ustar 格式
    char version[2];     // 版本 "00"
    char uname[32];      // 用户名
    char gname[32];      // 组名
    char devmajor[8];    // 主设备号（用于特殊文件）
    char devminor[8];    // 从设备号（用于特殊文件）
    char prefix[155];    // 文件前缀（用于支持长路径名）
    char padding[12];    // 填充字节，使结构体大小为 512 字节
};

// 编译时断言，确保头部结构体大小正好为 512 字节
static_assert(sizeof(header) == 512, "tar header must be exactly 512 bytes");

// 工具函数声明

// 解析八进制字符串为无符号整数
// 参数：data - 包含八进制字符串的字符数组
//        size - 字符数组的大小
// 返回值：解析后的无符号整数值
std::uintmax_t parse_octal(const char* data, std::size_t size);

// 将无符号整数写入缓冲区为八进制字符串
// 参数：value - 要写入的无符号整数值
//        buffer - 输出缓冲区
//        size - 缓冲区大小
void write_octal(std::uintmax_t value, char* buffer, std::size_t size);

// 计算 tar 头部的校验和
// 参数：hdr - tar 头部引用
// 返回值：校验和（所有字节的简单求和，校验和字段本身用空格计算）
unsigned int calculate_checksum(const header& hdr);

// 检查头部块是否全为零（表示 tar 文件结束）
// 参数：hdr - tar 头部引用
// 返回值：如果头部所有字节都为 0 则返回 true，否则返回 false
bool is_zero_block(const header& hdr);

// tar 打包器类声明
// 功能：将文件和目录打包到内存中，支持输出到文件或获取原始数据
class writer {
   private:
    std::ostringstream out_;  // 内部字符串流，用于在内存中构建 tar 数据

    // 将文件数据写入输出流
    // 参数：path - 源文件路径
    //        size - 文件大小
    void write_file_data(const fs::path& path, std::uintmax_t size);

    // 写入目录条目到 tar
    // 参数：dir_name - 在 tar 中的目录名
    void write_directory_entry(const std::string& dir_name);

    // 写入文件条目到 tar
    // 参数：file_path - 在 tar 中的文件路径
    //        real_path - 实际文件系统路径
    void write_file_entry(const std::string& file_path,
                          const fs::path& real_path);

    // 递归添加目录内容
    // 参数：dir_path - 要添加的目录路径
    //        base_name - 在 tar 中的基路径
    void add_directory_recursive(const fs::path& dir_path,
                                 const std::string& base_name);

    // 完成 tar 打包，添加两个全零块作为结束标志
    void finish();  // 改为私有

   public:
    // 构造函数，初始化字符串流
    writer();  // 不再需要传入文件路径

    // 禁止拷贝构造和拷贝赋值
    writer(const writer&) = delete;
    writer& operator=(const writer&) = delete;

    // 析构函数，如果还有数据，自动完成打包
    ~writer();

    // 添加文件到 tar
    // 参数：file_path - 要添加的文件路径
    //        tar_name - （可选）在 tar 中的自定义名称，为空则使用文件名
    void add_file(const fs::path& file_path, const std::string& tar_name = "");

    // 添加目录到 tar
    // 参数：dir_path - 要添加的目录路径
    //        tar_name - （可选）在 tar 中的自定义名称，为空则使用目录名
    void add_directory(const fs::path& dir_path,
                       const std::string& tar_name = "");

    // 新的数据访问接口

    // 获取所有 tar 数据作为字符串
    // 返回值：包含完整 tar 数据的字符串
    std::string get_data() const;

    // 获取所有 tar 数据作为字符向量
    // 返回值：包含完整 tar 数据的字符向量
    std::vector<char> get_vector() const;

    // 将 tar 数据写入文件
    // 参数：file_path - 输出文件路径
    void write_to_file(const fs::path& file_path);

    // 清空数据以便重复使用
    void clear();

    // 流接口

    // 获取内部输出流引用（用于直接操作）
    std::ostringstream& stream() { return out_; }
    const std::ostringstream& stream() const { return out_; }

    // 工具函数

    // 获取当前数据大小
    // 返回值：当前 tar 数据的大小（字节数）
    std::size_t size() const;

    // 判断是否为空
    // 返回值：如果 tar 数据为空则返回 true，否则返回 false
    bool empty() const;
};

// tar 解包器类声明
// 功能：从各种数据源读取 tar 文件，支持解压和列出内容
class reader {
   public:
    // 构造函数：从文件读取 tar 数据
    // 参数：archive_path - tar 文件路径
    explicit reader(const fs::path& archive_path);

    // 构造函数：从字符串流读取 tar 数据
    // 参数：stream - 字符串流智能指针
    explicit reader(std::unique_ptr<std::istream> stream);

    // 构造函数：从字符串读取 tar 数据
    // 参数：data - 包含 tar 数据的字符串
    explicit reader(const std::string& data);

    // 构造函数：从 vector<char> 读取 tar 数据
    // 参数：data - 包含 tar 数据的字符向量
    explicit reader(const std::vector<char>& data);

    // 构造函数：从原始指针和大小读取 tar 数据
    // 参数：data - 指向 tar 数据的指针
    //        size - 数据大小
    reader(const char* data, std::size_t size);

    // 禁止拷贝构造和拷贝赋值
    reader(const reader&) = delete;
    reader& operator=(const reader&) = delete;

    // 默认构造函数（创建空 reader，需后续调用 set_source 设置数据源）
    reader() = default;

    // 移动构造函数
    // 参数：other - 要移动的 reader 对象
    reader(reader&& other) noexcept;

    // 移动赋值运算符
    // 参数：other - 要移动的 reader 对象
    // 返回值：移动后的 reader 引用
    reader& operator=(reader&& other) noexcept;

    // 设置数据源接口

    // 设置数据源为字符串流
    // 参数：stream - 字符串流智能指针
    void set_source(std::unique_ptr<std::istream> stream);

    // 设置数据源为字符串
    // 参数：data - 包含 tar 数据的字符串
    void set_source(const std::string& data);

    // 设置数据源为字符向量
    // 参数：data - 包含 tar 数据的字符向量
    void set_source(const std::vector<char>& data);

    // 设置数据源为原始指针和大小
    // 参数：data - 指向 tar 数据的指针
    //        size - 数据大小
    void set_source(const char* data, std::size_t size);

    // 设置数据源为文件
    // 参数：archive_path - tar 文件路径
    void set_source(const fs::path& archive_path);

    // 检查是否有有效数据源
    // 返回值：如果已设置有效数据源则返回 true，否则返回 false
    bool is_open() const;

    // 释放当前数据源
    void close();

    // 解压所有文件到指定目录
    // 参数：output_dir - 输出目录，默认为当前目录
    void extract_all(const fs::path& output_dir = ".");

    // 列出 tar 文件内容
    void list();

   private:
    // 文件流（用于文件源）
    std::unique_ptr<std::ifstream> file_stream_;

    // 内存流（用于内存源）
    std::unique_ptr<std::istream> memory_stream_;

    // 当前活动流指针
    std::istream* in_ = nullptr;

    // 读取一个头部块
    // 参数：hdr - 用于存储头部数据的引用
    // 返回值：成功读取返回 true，遇到结束标志返回 false
    bool read_header(header& hdr);

    // 从头部获取完整路径
    // 参数：hdr - tar 头部
    // 返回值：组合后的完整路径字符串
    std::string get_path(const header& hdr);

    // 跳过文件数据块
    // 参数：size - 要跳过的数据大小
    void skip_data(std::uintmax_t size);

    // 提取文件
    // 参数：path - 目标文件路径
    //        size - 文件大小
    void extract_file(const fs::path& path, std::uintmax_t size);

    // 清理资源
    void cleanup();
};

// 便捷函数声明

// 创建 tar 归档文件（从多个文件）
// 参数：archive_path - 输出的 tar 文件路径
//        files - 要包含的文件路径列表
void create_archive(const fs::path& archive_path,
                    const std::vector<fs::path>& files);

// 创建 tar 归档文件（从目录）
// 参数：archive_path - 输出的 tar 文件路径
//        directory - 要打包的目录路径
//        tar_name - （可选）在 tar 中的目录名，为空则使用原目录名
void create_archive_from_directory(const fs::path& archive_path,
                                   const fs::path& directory,
                                   const std::string& tar_name = "");

// 从文件解压 tar 归档
// 参数：archive_path - 输入的 tar 文件路径
//        output_dir - 解压目标目录，默认为当前目录
void extract_archive(const fs::path& archive_path,
                     const fs::path& output_dir = ".");

// 从字符串解压 tar 归档
// 参数：data - 包含 tar 数据的字符串
//        output_dir - 解压目标目录，默认为当前目录
void extract_archive_from_memory(const std::string& data,
                                 const fs::path& output_dir = ".");

// 从字符向量解压 tar 归档
// 参数：data - 包含 tar 数据的字符向量
//        output_dir - 解压目标目录，默认为当前目录
void extract_archive_from_memory(const std::vector<char>& data,
                                 const fs::path& output_dir = ".");

// 列出 tar 归档文件内容
// 参数：archive_path - tar 文件路径
void list_archive(const fs::path& archive_path);

// 从字符串列出 tar 归档内容
// 参数：data - 包含 tar 数据的字符串
void list_archive_from_memory(const std::string& data);

// 从字符向量列出 tar 归档内容
// 参数：data - 包含 tar 数据的字符向量
void list_archive_from_memory(const std::vector<char>& data);

}  // namespace tar

#endif  // TAR_H