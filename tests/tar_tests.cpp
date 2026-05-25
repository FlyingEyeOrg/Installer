#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <vector>

#include "tar/tar.hpp"

namespace fs = std::filesystem;

namespace {

void create_file(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << content;
}

std::string read_file(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

class TarTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "tar_gtest_XXXXXX";
        // Simple unique dir using random suffix
        test_dir_ = fs::temp_directory_path() /
                    ("tar_gtest_" + std::to_string(std::rand()));
        fs::create_directories(test_dir_);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(test_dir_, ec);
    }

    fs::path dir() const { return test_dir_; }

private:
    fs::path test_dir_;
};

}  // namespace

// ---- utility tests ----

TEST(UtilTest, ParseOctal) {
    EXPECT_EQ(tar::parse_octal("777\0\0\0\0\0", 8), 0777);
    EXPECT_EQ(tar::parse_octal("0\0\0\0\0\0\0\0", 8), 0);
    EXPECT_EQ(tar::parse_octal("100644\0", 8), 0100644);
    EXPECT_EQ(tar::parse_octal("\0\0\0\0\0\0\0\0", 8), 0);
    EXPECT_EQ(tar::parse_octal("    777\0", 8), 0777);
}

TEST(UtilTest, WriteOctal) {
    char buf[12] = {};
    tar::write_octal(0644, buf, sizeof(buf));
    EXPECT_EQ(tar::parse_octal(buf, sizeof(buf)), 0644);
}

TEST(UtilTest, CalculateChecksum) {
    tar::header hdr = {};
    std::strncpy(hdr.name, "test.txt", sizeof(hdr.name));
    std::memcpy(hdr.magic, "ustar", 5);
    unsigned int sum = tar::calculate_checksum(hdr);
    EXPECT_GT(sum, 0u);
}

TEST(UtilTest, IsZeroBlock) {
    tar::header hdr = {};
    EXPECT_TRUE(tar::is_zero_block(hdr));
    hdr.name[0] = 'x';
    EXPECT_FALSE(tar::is_zero_block(hdr));
}

TEST(UtilTest, SplitLongPathShort) {
    char name[100] = {};
    char prefix[155] = {};
    tar::split_long_path("short.txt", name, prefix);
    EXPECT_STREQ(name, "short.txt");
    EXPECT_STREQ(prefix, "");
}

TEST(UtilTest, SplitLongPathLong) {
    std::string long_prefix(150, 'a');
    std::string path = long_prefix + "/file.txt";

    char name[100] = {};
    char prefix[155] = {};
    tar::split_long_path(path, name, prefix);
    EXPECT_STREQ(name, "file.txt");
    EXPECT_STREQ(prefix, long_prefix.c_str());
}

// ---- writer tests ----

TEST_F(TarTest, WriterAddFile) {
    auto file_path = dir() / "hello.txt";
    create_file(file_path, "Hello, World!");

    tar::writer w;
    w.add_file(file_path);
    EXPECT_GT(w.size(), 0u);
    EXPECT_FALSE(w.empty());
}

TEST_F(TarTest, WriterFinishAddsEndMarker) {
    auto file_path = dir() / "data.txt";
    create_file(file_path, "test");

    tar::writer w;
    w.add_file(file_path);
    auto size_before = w.size();
    w.finish();
    EXPECT_EQ(w.size(), size_before + 1024);
}

TEST_F(TarTest, WriterClear) {
    auto file_path = dir() / "data.txt";
    create_file(file_path, "test");

    tar::writer w;
    w.add_file(file_path);
    EXPECT_FALSE(w.empty());

    w.clear();
    EXPECT_TRUE(w.empty());
    EXPECT_EQ(w.size(), 0u);
}

TEST_F(TarTest, WriterRejectsNonExistentFile) {
    tar::writer w;
    EXPECT_THROW(w.add_file(dir() / "nonexistent.txt"), std::runtime_error);
}

TEST_F(TarTest, WriterRejectsDirectoryAsFile) {
    auto subdir = dir() / "sub";
    fs::create_directories(subdir);

    tar::writer w;
    EXPECT_THROW(w.add_file(subdir), std::runtime_error);
}

// ---- reader tests ----

TEST_F(TarTest, RoundTripSingleFile) {
    auto src = dir() / "src.txt";
    create_file(src, "Hello, Tar!");

    tar::writer w;
    w.add_file(src, "archive_name.txt");
    w.write_to_file(dir() / "test.tar");

    tar::reader r(dir() / "test.tar");
    r.extract_all(dir() / "out");

    auto extracted = dir() / "out" / "archive_name.txt";
    EXPECT_TRUE(fs::exists(extracted));
    EXPECT_EQ(read_file(extracted), "Hello, Tar!");
}

TEST_F(TarTest, RoundTripMultipleFiles) {
    create_file(dir() / "a.txt", "content A");
    create_file(dir() / "b.txt", "content B");
    create_file(dir() / "c.txt", "content C");

    tar::writer w;
    w.add_file(dir() / "a.txt");
    w.add_file(dir() / "b.txt");
    w.add_file(dir() / "c.txt");
    w.write_to_file(dir() / "test.tar");

    tar::reader r(dir() / "test.tar");
    r.extract_all(dir() / "out");

    EXPECT_EQ(read_file(dir() / "out/a.txt"), "content A");
    EXPECT_EQ(read_file(dir() / "out/b.txt"), "content B");
    EXPECT_EQ(read_file(dir() / "out/c.txt"), "content C");
}

TEST_F(TarTest, RoundTripDirectory) {
    create_file(dir() / "src/root.txt", "root");
    create_file(dir() / "src/sub/file.txt", "nested");
    create_file(dir() / "src/sub/deep/data.txt", "deep");

    tar::writer w;
    w.add_directory(dir() / "src", "mydir");
    w.write_to_file(dir() / "test.tar");

    tar::reader r(dir() / "test.tar");
    r.extract_all(dir() / "out");

    EXPECT_EQ(read_file(dir() / "out/mydir/root.txt"), "root");
    EXPECT_EQ(read_file(dir() / "out/mydir/sub/file.txt"), "nested");
    EXPECT_EQ(read_file(dir() / "out/mydir/sub/deep/data.txt"), "deep");
    EXPECT_TRUE(fs::is_directory(dir() / "out/mydir/sub"));
}

TEST_F(TarTest, RoundTripLargeFile) {
    auto large = dir() / "large.bin";
    {
        std::ofstream file(large, std::ios::binary);
        std::mt19937 rng(42);
        std::uniform_int_distribution<unsigned short> dist(0, 255);
        std::vector<char> buf(65536);
        for (int i = 0; i < 16; ++i) {  // 1MB
            for (auto& c : buf) c = static_cast<char>(dist(rng));
            file.write(buf.data(), buf.size());
        }
    }

    tar::writer w;
    w.add_file(large);
    w.write_to_file(dir() / "large.tar");

    tar::reader r(dir() / "large.tar");
    r.extract_all(dir() / "out");

    auto original_sz = fs::file_size(large);
    auto extracted_sz = fs::file_size(dir() / "out/large.bin");
    EXPECT_EQ(original_sz, extracted_sz);
}

// ---- memory sources ----

TEST_F(TarTest, ReaderFromString) {
    auto src = dir() / "data.txt";
    create_file(src, "memory test");

    tar::writer w;
    w.add_file(src);
    auto data = w.get_data();

    tar::reader r(data);
    r.extract_all(dir() / "out");
    EXPECT_EQ(read_file(dir() / "out/data.txt"), "memory test");
}

TEST_F(TarTest, ReaderFromVector) {
    auto src = dir() / "data.txt";
    create_file(src, "vector test");

    tar::writer w;
    w.add_file(src);
    auto vec = w.get_vector();

    tar::reader r(vec);
    r.extract_all(dir() / "out");
    EXPECT_EQ(read_file(dir() / "out/data.txt"), "vector test");
}

TEST_F(TarTest, ReaderFromStream) {
    auto src = dir() / "data.txt";
    create_file(src, "stream test");

    tar::writer w;
    w.add_file(src);
    auto stream = std::make_unique<std::istringstream>(w.get_data());

    tar::reader r(std::move(stream));
    r.extract_all(dir() / "out");
    EXPECT_EQ(read_file(dir() / "out/data.txt"), "stream test");
}

TEST_F(TarTest, ReaderFromRawPtr) {
    auto src = dir() / "data.txt";
    create_file(src, "raw ptr test");

    tar::writer w;
    w.add_file(src);
    auto vec = w.get_vector();

    tar::reader r(vec.data(), vec.size());
    r.extract_all(dir() / "out");
    EXPECT_EQ(read_file(dir() / "out/data.txt"), "raw ptr test");
}

// ---- reader lifecycle ----

TEST_F(TarTest, ReaderDefaultUnopened) {
    tar::reader r;
    EXPECT_FALSE(r.is_open());
    EXPECT_THROW(r.extract_all(dir() / "out"), std::runtime_error);
}

TEST_F(TarTest, ReaderMoveConstructor) {
    auto src = dir() / "data.txt";
    create_file(src, "move test");

    tar::writer w;
    w.add_file(src);

    tar::reader r1(w.get_data());
    EXPECT_TRUE(r1.is_open());

    tar::reader r2 = std::move(r1);
    EXPECT_TRUE(r2.is_open());
    EXPECT_FALSE(r1.is_open());
}

TEST_F(TarTest, ReaderMoveAssignment) {
    auto src = dir() / "data.txt";
    create_file(src, "move assign test");

    tar::writer w;
    w.add_file(src);

    tar::reader r1(w.get_data());
    tar::reader r2;
    r2 = std::move(r1);
    EXPECT_TRUE(r2.is_open());
    EXPECT_FALSE(r1.is_open());
}

TEST_F(TarTest, ReaderClose) {
    auto src = dir() / "data.txt";
    create_file(src, "close test");

    tar::writer w;
    w.add_file(src);

    tar::reader r(w.get_data());
    EXPECT_TRUE(r.is_open());
    r.close();
    EXPECT_FALSE(r.is_open());
}

// ---- error handling ----

TEST_F(TarTest, ReaderRejectsNonExistentFile) {
    EXPECT_THROW(tar::reader(fs::path(dir() / "nope.tar")), std::runtime_error);
}

TEST_F(TarTest, ReaderRejectsTooSmallString) {
    EXPECT_THROW(tar::reader(std::string("abc")), std::runtime_error);
}

TEST_F(TarTest, ReaderRejectsTooSmallVector) {
    std::vector<char> tiny(10, 0);
    tar::reader r;
    EXPECT_THROW(r.set_source(tiny), std::runtime_error);
}

TEST_F(TarTest, WriterRejectsNonExistentDir) {
    tar::writer w;
    EXPECT_THROW(w.add_directory(dir() / "nope_dir"), std::runtime_error);
}

// ---- convenience functions ----

TEST_F(TarTest, CreateArchiveFromFiles) {
    create_file(dir() / "1.txt", "one");
    create_file(dir() / "2.txt", "two");

    tar::create_archive(dir() / "test.tar",
                        {dir() / "1.txt", dir() / "2.txt"});

    tar::reader r(dir() / "test.tar");
    r.extract_all(dir() / "out");
    EXPECT_EQ(read_file(dir() / "out/1.txt"), "one");
    EXPECT_EQ(read_file(dir() / "out/2.txt"), "two");
}

TEST_F(TarTest, CreateArchiveFromDirectory) {
    create_file(dir() / "src/a.txt", "A");

    tar::create_archive_from_directory(dir() / "test.tar", dir() / "src",
                                       "pkg");

    tar::reader r(dir() / "test.tar");
    r.extract_all(dir() / "out");
    EXPECT_EQ(read_file(dir() / "out/pkg/a.txt"), "A");
}

TEST_F(TarTest, ExtractAndListArchive) {
    create_file(dir() / "x.txt", "X");
    tar::create_archive(dir() / "test.tar", {dir() / "x.txt"});

    // list should not throw
    testing::internal::CaptureStdout();
    tar::list_archive(dir() / "test.tar");
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("x.txt"), std::string::npos);

    // extract
    tar::extract_archive(dir() / "test.tar", dir() / "out");
    EXPECT_TRUE(fs::exists(dir() / "out/x.txt"));
}

TEST_F(TarTest, MemoryConvenienceFunctions) {
    create_file(dir() / "m.txt", "M");

    tar::writer w;
    w.add_file(dir() / "m.txt");

    tar::extract_archive_from_memory(w.get_data(), dir() / "out_str");
    EXPECT_EQ(read_file(dir() / "out_str/m.txt"), "M");

    tar::extract_archive_from_memory(w.get_vector(), dir() / "out_vec");
    EXPECT_EQ(read_file(dir() / "out_vec/m.txt"), "M");

    testing::internal::CaptureStdout();
    tar::list_archive_from_memory(w.get_data());
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("m.txt"), std::string::npos);
}
