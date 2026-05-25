#include "tar/tar.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>

namespace tar {
namespace chrono = std::chrono;

// ---- utility functions ----

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
    const auto* p = reinterpret_cast<const unsigned char*>(&hdr);
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
    const auto* p = reinterpret_cast<const unsigned char*>(&hdr);
    for (std::size_t i = 0; i < sizeof(header); ++i) {
        if (p[i] != 0) return false;
    }
    return true;
}

void split_long_path(const std::string& path, char* name, char* prefix) {
    if (path.size() <= 100) {
        std::strncpy(name, path.c_str(), 100);
        return;
    }

    std::size_t last_slash = path.find_last_of('/');
    if (last_slash == std::string::npos || last_slash > 155) {
        std::size_t start = path.size() > 100 ? path.size() - 100 : 0;
        std::strncpy(name, path.c_str() + start, 100);
        return;
    }

    std::string pre = path.substr(0, last_slash);
    std::string nm = path.substr(last_slash + 1);

    if (nm.size() <= 100 && pre.size() <= 155) {
        std::strncpy(name, nm.c_str(), 100);
        std::strncpy(prefix, pre.c_str(), 155);
    } else {
        std::strncpy(name, nm.c_str(), 100);
    }
}

void init_ustar_fields(header& hdr) {
    std::memcpy(hdr.magic, "ustar", 5);
    hdr.magic[5] = 0;
    hdr.version[0] = '0';
    hdr.version[1] = '0';
    std::strncpy(hdr.uname, "user", 32);
    std::strncpy(hdr.gname, "group", 32);
}

// ---- writer ----

static constexpr std::size_t kIOBufferSize = 65536;  // 64KB

writer::writer() {
    out_.exceptions(std::ios::badbit | std::ios::failbit);
}

void writer::write_file_data(const fs::path& path, std::uintmax_t size) {
    if (size == 0) return;

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Cannot open file: " + path.string());
    }

    alignas(std::max_align_t) char buffer[kIOBufferSize];
    std::uintmax_t remaining = size;

    while (remaining > 0) {
        std::size_t to_read = static_cast<std::size_t>(
            std::min<std::uintmax_t>(kIOBufferSize, remaining));
        in.read(buffer, to_read);
        out_.write(buffer, in.gcount());
        remaining -= in.gcount();
    }

    // Pad to 512-byte boundary
    std::size_t padding = (512 - (size % 512)) % 512;
    if (padding > 0) {
        static constexpr char zeros[512] = {};
        out_.write(zeros, padding);
    }
}

void writer::write_directory_entry(const std::string& dir_name) {
    header hdr = {};

    std::string rel_path = dir_name;
    if (!rel_path.empty() && rel_path.back() != '/') {
        rel_path.push_back('/');
    }

    // Normalize backslashes
    std::replace(rel_path.begin(), rel_path.end(), '\\', '/');
    split_long_path(rel_path, hdr.name, hdr.prefix);

    write_octal(0755, hdr.mode, sizeof(hdr.mode));
    write_octal(0, hdr.uid, sizeof(hdr.uid));
    write_octal(0, hdr.gid, sizeof(hdr.gid));
    write_octal(0, hdr.size, sizeof(hdr.size));

    auto time = std::time(nullptr);
    write_octal(static_cast<std::uintmax_t>(time), hdr.mtime, sizeof(hdr.mtime));

    hdr.typeflag = '5';
    init_ustar_fields(hdr);

    unsigned int checksum = calculate_checksum(hdr);
    write_octal(checksum, hdr.chksum, sizeof(hdr.chksum));

    out_.write(reinterpret_cast<const char*>(&hdr), sizeof(header));
}

void writer::write_file_entry(const std::string& file_path,
                              const fs::path& real_path) {
    header hdr = {};

    std::string rel_path = file_path;
    std::replace(rel_path.begin(), rel_path.end(), '\\', '/');
    split_long_path(rel_path, hdr.name, hdr.prefix);

    write_octal(0644, hdr.mode, sizeof(hdr.mode));
    write_octal(0, hdr.uid, sizeof(hdr.uid));
    write_octal(0, hdr.gid, sizeof(hdr.gid));

    std::uintmax_t size = fs::file_size(real_path);
    write_octal(size, hdr.size, sizeof(hdr.size));

    auto mtime = fs::last_write_time(real_path);
    auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(
        mtime - fs::file_time_type::clock::now() + chrono::system_clock::now());
    auto time = chrono::system_clock::to_time_t(sctp);
    write_octal(static_cast<std::uintmax_t>(time), hdr.mtime, sizeof(hdr.mtime));

    hdr.typeflag = '0';
    init_ustar_fields(hdr);

    unsigned int checksum = calculate_checksum(hdr);
    write_octal(checksum, hdr.chksum, sizeof(hdr.chksum));

    out_.write(reinterpret_cast<const char*>(&hdr), sizeof(header));
    write_file_data(real_path, size);
}

void writer::add_directory_recursive(const fs::path& dir_path,
                                     const std::string& base_name) {
    for (const auto& entry : fs::recursive_directory_iterator(
             dir_path, fs::directory_options::skip_permission_denied)) {
        try {
            fs::path relative = fs::relative(entry.path(), dir_path);
            std::string tar_path = base_name + "/" + relative.string();
            std::replace(tar_path.begin(), tar_path.end(), '\\', '/');

            if (fs::is_directory(entry.status())) {
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
    std::string name =
        tar_name.empty() ? dir_path.filename().string() : tar_name;
    write_directory_entry(name + "/");
    add_directory_recursive(dir_path, name);
}

void writer::finish() {
    static constexpr char zero_block[1024] = {};
    out_.write(zero_block, 1024);
}

std::string writer::get_data() const { return out_.str(); }

std::vector<char> writer::get_vector() const {
    std::string data = out_.str();
    return std::vector<char>(data.begin(), data.end());
}

void writer::write_to_file(const fs::path& file_path) {
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
    out_.str("");
    out_.clear();
}

std::size_t writer::size() const { return out_.str().size(); }

bool writer::empty() const { return out_.str().empty(); }

// ---- reader ----

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
        in_->seekg(0, std::ios::end);
        auto sz = in_->tellg();
        in_->seekg(0, std::ios::beg);
        if (sz < static_cast<std::streamsize>(sizeof(header))) {
            throw std::runtime_error("Invalid tar data: data too small");
        }
        if (!*in_) {
            throw std::runtime_error("Failed to set stream source");
        }
    }
}

void reader::set_source(const std::string& data) {
    if (data.size() < sizeof(header)) {
        throw std::runtime_error("Invalid tar data: data too small");
    }
    set_source(std::make_unique<std::istringstream>(data));
}

void reader::set_source(const std::vector<char>& data) {
    if (data.size() < sizeof(header)) {
        throw std::runtime_error("Invalid tar data: data too small");
    }
    set_source(std::make_unique<std::istringstream>(
        std::string(data.begin(), data.end())));
}

void reader::set_source(const char* data, std::size_t size) {
    if (size < sizeof(header)) {
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
    file_stream_->seekg(0, std::ios::end);
    auto sz = file_stream_->tellg();
    file_stream_->seekg(0, std::ios::beg);
    if (sz < static_cast<std::streamsize>(sizeof(header))) {
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

// Read a C-string safely from a fixed-size char array
static std::string read_field(const char* field, std::size_t max_size) {
    const char* end = static_cast<const char*>(std::memchr(field, '\0', max_size));
    std::size_t len = end ? static_cast<std::size_t>(end - field) : max_size;
    return std::string(field, len);
}

std::string reader::get_path(const header& hdr) {
    std::string prefix = read_field(hdr.prefix, sizeof(hdr.prefix));
    std::string name = read_field(hdr.name, sizeof(hdr.name));

    if (!prefix.empty()) {
        return prefix + "/" + name;
    }
    return name;
}

bool reader::read_header(header& hdr) {
    if (!in_) {
        throw std::runtime_error("No data source is open");
    }

    in_->read(reinterpret_cast<char*>(&hdr), sizeof(header));
    if (in_->gcount() != sizeof(header)) {
        return false;
    }

    // End-of-archive: two consecutive zero blocks
    if (is_zero_block(hdr)) {
        header next;
        in_->read(reinterpret_cast<char*>(&next), sizeof(header));
        if (is_zero_block(next)) {
            return false;
        }
        // Single zero block — seek back (not end of archive)
        in_->seekg(-static_cast<std::streamoff>(sizeof(header)), std::ios::cur);
    }

    // Validate checksum
    unsigned int expected = parse_octal(hdr.chksum, sizeof(hdr.chksum));
    unsigned int actual = calculate_checksum(hdr);
    if (expected != actual) {
        throw std::runtime_error("Corrupt tar header: checksum mismatch "
                                 "(expected " + std::to_string(expected) +
                                 ", got " + std::to_string(actual) + ")");
    }

    // Validate magic
    if (std::strncmp(hdr.magic, "ustar", 5) != 0) {
        // Non-ustar archives may still work; just skip magic validation
    }

    return true;
}

void reader::skip_data(std::uintmax_t size) {
    if (size == 0 || !in_) return;
    std::uintmax_t blocks = (size + 511) / 512;
    in_->seekg(blocks * 512, std::ios::cur);
}

void reader::extract_file(const fs::path& path, std::uintmax_t size) {
    if (!in_) {
        throw std::runtime_error("No data source is open");
    }

    fs::create_directories(path.parent_path());

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Cannot create file: " + path.string());
    }

    alignas(std::max_align_t) char buffer[kIOBufferSize];
    std::uintmax_t remaining = size;

    while (remaining > 0) {
        std::size_t to_read = static_cast<std::size_t>(
            std::min<std::uintmax_t>(kIOBufferSize, remaining));
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

    std::size_t padding = (512 - (size % 512)) % 512;
    if (padding > 0) {
        in_->seekg(padding, std::ios::cur);
    }
}

void reader::extract_all(const fs::path& output_dir) {
    if (!in_) {
        throw std::runtime_error("No data source is open");
    }

    std::streampos original_pos = in_->tellg();
    in_->seekg(0, std::ios::beg);

    header hdr;
    try {
        while (read_header(hdr)) {
            std::string filename = get_path(hdr);
            if (filename.empty()) {
                skip_data(parse_octal(hdr.size, sizeof(hdr.size)));
                continue;
            }

            std::uintmax_t size = parse_octal(hdr.size, sizeof(hdr.size));
            fs::path full_path = output_dir / filename;

            try {
                switch (hdr.typeflag) {
                    case '0':
                    case '\0':
                        extract_file(full_path, size);
                        break;
                    case '5':
                        fs::create_directories(full_path);
                        if (size > 0) skip_data(size);
                        break;
                    default:
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
        in_->seekg(original_pos);
        throw;
    }

    in_->seekg(original_pos);
}

void reader::list() {
    if (!in_) {
        throw std::runtime_error("No data source is open");
    }

    std::streampos original_pos = in_->tellg();
    in_->seekg(0, std::ios::beg);

    std::cout << "Type  Size      Modified             Name\n";
    std::cout << "----  --------  -------------------  ----\n";

    header hdr;
    try {
        while (read_header(hdr)) {
            std::string filename = get_path(hdr);
            if (filename.empty()) {
                skip_data(parse_octal(hdr.size, sizeof(hdr.size)));
                continue;
            }

            std::uintmax_t size = parse_octal(hdr.size, sizeof(hdr.size));

            auto mtime = static_cast<std::time_t>(
                parse_octal(hdr.mtime, sizeof(hdr.mtime)));
            char time_buf[20];
            std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S",
                          std::localtime(&mtime));

            std::cout << (hdr.typeflag == '5' ? "d" : "-") << "  "
                      << std::setw(8) << size << "  " << time_buf << "  "
                      << filename << std::endl;

            skip_data(size);
        }
    } catch (...) {
        in_->seekg(original_pos);
        throw;
    }

    in_->seekg(original_pos);
}

// ---- convenience functions ----

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
