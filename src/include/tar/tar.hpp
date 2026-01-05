#ifndef TAR_H
#define TAR_H

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace tar {
namespace fs = std::filesystem;

// tar 文件头结构
struct header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
};

static_assert(sizeof(header) == 512, "tar header must be exactly 512 bytes");

// 工具函数声明
std::uintmax_t parse_octal(const char* data, std::size_t size);
void write_octal(std::uintmax_t value, char* buffer, std::size_t size);
unsigned int calculate_checksum(const header& hdr);
bool is_zero_block(const header& hdr);

// tar 打包器类声明
class writer {
   private:
    std::ofstream out_;
    void write_file_data(const fs::path& path, std::uintmax_t size);
    void write_directory_entry(const std::string& dir_name);
    void write_file_entry(const std::string& file_path,
                          const fs::path& real_path);
    void add_directory_recursive(const fs::path& dir_path,
                                 const std::string& base_name);

   public:
    explicit writer(const fs::path& archive_path);
    writer(const writer&) = delete;
    writer& operator=(const writer&) = delete;
    ~writer();
    void add_file(const fs::path& file_path, const std::string& tar_name = "");
    void add_directory(const fs::path& dir_path,
                       const std::string& tar_name = "");
    void finish();
};

// tar 解包器类声明
class reader {
   private:
    std::ifstream in_;
    bool read_header(header& hdr);
    std::string get_path(const header& hdr);
    void skip_data(std::uintmax_t size);
    void extract_file(const fs::path& path, std::uintmax_t size);

   public:
    explicit reader(const fs::path& archive_path);
    reader(const reader&) = delete;
    reader& operator=(const reader&) = delete;
    void extract_all(const fs::path& output_dir = ".");
    void list();
};

// 便捷函数声明
void create_archive(const fs::path& archive_path,
                    const std::vector<fs::path>& files);
void create_archive_from_directory(const fs::path& archive_path,
                                   const fs::path& directory,
                                   const std::string& tar_name = "");
void extract_archive(const fs::path& archive_path,
                     const fs::path& output_dir = ".");
void list_archive(const fs::path& archive_path);

}  // namespace tar

#endif  // TAR_H