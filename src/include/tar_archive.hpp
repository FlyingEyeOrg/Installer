#include <fmt/core.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#include "microtar.hpp"
#include "string_convertor.hpp"


namespace fs = std::filesystem;

class TarArchive {
   public:
    enum class Mode { Read, Write, Append };

    TarArchive(const fs::path& path, Mode mode) {
        const char* mode_str = "";
        switch (mode) {
            case Mode::Read:
                mode_str = "r";
                break;
            case Mode::Write:
                mode_str = "w";
                break;
            case Mode::Append:
                mode_str = "a";
                break;
        }

        int res =
            mtar_open(&m_tar, StringConvertor::UTF8ToGBK(path.string()).c_str(),
                      mode_str);
        if (res != MTAR_ESUCCESS) {
            throw std::runtime_error(fmt::format(
                "Failed to open tar archive: {}", mtar_strerror(res)));
        }
    }

    ~TarArchive() {
        if (m_is_open) {
            mtar_close(&m_tar);
        }
    }

    // 禁止拷贝
    TarArchive(const TarArchive&) = delete;
    TarArchive& operator=(const TarArchive&) = delete;

    // 移动构造
    TarArchive(TarArchive&& other) noexcept
        : m_tar(other.m_tar), m_is_open(other.m_is_open) {
        other.m_is_open = false;
    }

    // 添加单个文件到tar包
    void AddFile(const fs::path& file_path, const fs::path& archive_path = "") {
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

        // mtar 不支持中文字符串，所以要进行 utf8 2 gbk 转换
        // const auto gbk_string =
        // StringConvertor::UTF8ToGBK(final_archive_path.string()).c_str();

        // 写入文件头
        int res = mtar_write_file_header(
            &m_tar, final_archive_path.string().c_str(), file_size);
        if (res != MTAR_ESUCCESS) {
            throw std::runtime_error(fmt::format(
                "Failed to write file header: {}", mtar_strerror(res)));
        }

        // 写入文件内容
        std::vector<char> buffer(file_size);
        file.read(buffer.data(), file_size);
        res = mtar_write_data(&m_tar, buffer.data(), file_size);
        if (res != MTAR_ESUCCESS) {
            throw std::runtime_error(fmt::format(
                "Failed to write file data: {}", mtar_strerror(res)));
        }
    }

    // 添加整个目录到tar包
    void AddDirectory(const fs::path& dir_path,
                      const fs::path& archive_base = "") {
        if (!fs::exists(dir_path)) {
            throw std::runtime_error(
                fmt::format("Directory not found: {}", dir_path.string()));
        }

        if (!fs::is_directory(dir_path)) {
            throw std::runtime_error(
                fmt::format("Not a directory: {}", dir_path.string()));
        }

        fmt::print("Adding directory: {}\n", dir_path.string());

        for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
            if (entry.is_regular_file()) {
                fs::path relative_path = fs::relative(entry.path(), dir_path);
                if (!archive_base.empty()) {
                    relative_path = archive_base / relative_path;
                }
                AddFile(entry.path(), relative_path);
            } else if (entry.is_directory()) {
                // 对于目录，我们需要创建一个目录条目
                fs::path relative_path = fs::relative(entry.path(), dir_path);
                if (!archive_base.empty()) {
                    relative_path = archive_base / relative_path;
                }

                // 确保目录路径以/结尾
                std::string dir_entry_path = relative_path.string();
                if (dir_entry_path.back() != '/') {
                    dir_entry_path += '/';
                }

                fmt::print("Adding directory entry: {}\n", dir_entry_path);

                // mtar 不支持中文字符串，所以需要将字符串转换为 gbk
                // const auto gbk_string =
                // StringConvertor::UTF8ToGBK(dir_entry_path.c_str()).c_str();

                int res = mtar_write_dir_header(&m_tar, dir_entry_path.c_str());
                if (res != MTAR_ESUCCESS) {
                    throw std::runtime_error(
                        fmt::format("Failed to write directory header: {}",
                                    mtar_strerror(res)));
                }
            }
        }
    }

    // 解压整个tar包到指定目录
    void ExtractAll(const fs::path& output_dir) {
        if (!fs::exists(output_dir)) {
            fs::create_directories(output_dir);
        }

        mtar_header_t h;
        while ((mtar_read_header(&m_tar, &h)) != MTAR_ENULLRECORD) {
            fs::path file_path = output_dir / h.name;

            // 如果是目录条目
            if (h.type == MTAR_TDIR || (h.name[strlen(h.name) - 1] == '/')) {
                fmt::print("Creating directory: {}\n", file_path.string());
                fs::create_directories(file_path);
                continue;
            }

            // 创建父目录
            fs::create_directories(file_path.parent_path());

            fmt::print("Extracting file: {} (size: {} bytes)\n",
                       file_path.string(), h.size);

            // 打开输出文件
            std::ofstream file(file_path, std::ios::binary);
            if (!file) {
                throw std::runtime_error(fmt::format(
                    "Failed to create file: {}", file_path.string()));
            }

            // 读取文件内容
            std::vector<char> buffer(h.size);
            int res = mtar_read_data(&m_tar, buffer.data(), h.size);
            if (res != MTAR_ESUCCESS) {
                throw std::runtime_error(fmt::format(
                    "Failed to read file data: {}", mtar_strerror(res)));
            }

            // 写入文件内容
            file.write(buffer.data(), h.size);
        }
    }

    // 完成写入操作
    void Finalize() {
        int res = mtar_finalize(&m_tar);
        if (res != MTAR_ESUCCESS) {
            throw std::runtime_error(fmt::format(
                "Failed to finalize tar archive: {}", mtar_strerror(res)));
        }
    }

    // 打包多个文件到tar包
    static void CreateTarArchive(const fs::path& output_path,
                                 const std::vector<fs::path>& files);

    // 打包整个目录到tar包
    static void CreateTarFromDirectory(const fs::path& output_path,
                                       const fs::path& input_dir);

    // 解压tar包到指定目录
    static void ExtractTarArchive(const fs::path& input_path,
                                  const fs::path& output_dir);

   private:
    mtar_t m_tar;
    bool m_is_open = true;
};
