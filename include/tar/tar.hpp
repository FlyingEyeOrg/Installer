#ifndef TAR_H
#define TAR_H

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace tar {
namespace fs = std::filesystem;

// POSIX ustar header (512 bytes)
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

// Parse octal string to unsigned integer
std::uintmax_t parse_octal(const char* data, std::size_t size);

// Write unsigned integer as octal string (right-aligned, space-padded)
void write_octal(std::uintmax_t value, char* buffer, std::size_t size);

// Calculate tar header checksum
unsigned int calculate_checksum(const header& hdr);

// Check if block is all zeros (end-of-archive marker)
bool is_zero_block(const header& hdr);

// Split a long path into prefix + name for ustar format
void split_long_path(const std::string& path, char* name, char* prefix);

// Initialize common ustar header fields (magic, version, uname, gname)
void init_ustar_fields(header& hdr);

class writer {
public:
    writer();
    ~writer() = default;

    writer(const writer&) = delete;
    writer& operator=(const writer&) = delete;

    void add_file(const fs::path& file_path, const std::string& tar_name = "");
    void add_directory(const fs::path& dir_path,
                       const std::string& tar_name = "");

    // Finalize the archive (write end-of-archive marker)
    void finish();

    std::string get_data() const;
    std::vector<char> get_vector() const;
    void write_to_file(const fs::path& file_path);
    void clear();

    std::ostringstream& stream() { return out_; }
    const std::ostringstream& stream() const { return out_; }

    std::size_t size() const;
    bool empty() const;

private:
    std::ostringstream out_;

    void write_file_data(const fs::path& path, std::uintmax_t size);
    void write_directory_entry(const std::string& dir_name);
    void write_file_entry(const std::string& file_path,
                          const fs::path& real_path);
    void add_directory_recursive(const fs::path& dir_path,
                                 const std::string& base_name);
};

class reader {
public:
    explicit reader(const fs::path& archive_path);
    explicit reader(std::unique_ptr<std::istream> stream);
    explicit reader(const std::string& data);
    explicit reader(const std::vector<char>& data);
    reader(const char* data, std::size_t size);

    reader() = default;
    reader(const reader&) = delete;
    reader& operator=(const reader&) = delete;
    reader(reader&& other) noexcept;
    reader& operator=(reader&& other) noexcept;

    void set_source(std::unique_ptr<std::istream> stream);
    void set_source(const std::string& data);
    void set_source(const std::vector<char>& data);
    void set_source(const char* data, std::size_t size);
    void set_source(const fs::path& archive_path);

    bool is_open() const;
    void close();

    void extract_all(const fs::path& output_dir = ".");
    void list();

private:
    std::unique_ptr<std::ifstream> file_stream_;
    std::unique_ptr<std::istream> memory_stream_;
    std::istream* in_ = nullptr;

    bool read_header(header& hdr);
    std::string get_path(const header& hdr);
    void skip_data(std::uintmax_t size);
    void extract_file(const fs::path& path, std::uintmax_t size);
    void cleanup();
};

// Convenience functions
void create_archive(const fs::path& archive_path,
                    const std::vector<fs::path>& files);
void create_archive_from_directory(const fs::path& archive_path,
                                   const fs::path& directory,
                                   const std::string& tar_name = "");

void extract_archive(const fs::path& archive_path,
                     const fs::path& output_dir = ".");
void extract_archive_from_memory(const std::string& data,
                                 const fs::path& output_dir = ".");
void extract_archive_from_memory(const std::vector<char>& data,
                                 const fs::path& output_dir = ".");

void list_archive(const fs::path& archive_path);
void list_archive_from_memory(const std::string& data);
void list_archive_from_memory(const std::vector<char>& data);

}  // namespace tar

#endif  // TAR_H
