#include "tar_archive.hpp"

// 打包多个文件到tar包
void TarArchive::CreateTarArchive(const fs::path &output_path, const std::vector<fs::path> &files)
{
    TarArchive tar(output_path, TarArchive::Mode::Write);

    for (const auto &file : files)
    {
        if (fs::is_directory(file))
        {
            tar.AddDirectory(file);
        }
        else
        {
            tar.AddFile(file);
        }
    }

    tar.Finalize();
    fmt::print("Successfully created tar archive: {}\n", output_path.string());
}

// 打包整个目录到tar包
void TarArchive::CreateTarFromDirectory(const fs::path &output_path, const fs::path &input_dir)
{
    TarArchive tar(output_path, TarArchive::Mode::Write);
    tar.AddDirectory(input_dir);
    tar.Finalize();
    fmt::print("Successfully created tar archive from directory: {}\n", output_path.string());
}

// 解压tar包到指定目录
void TarArchive::ExtractTarArchive(const fs::path &input_path, const fs::path &output_dir)
{
    TarArchive tar(input_path, TarArchive::Mode::Read);
    tar.ExtractAll(output_dir);
    fmt::print("Successfully extracted tar archive to: {}\n", output_dir.string());
}