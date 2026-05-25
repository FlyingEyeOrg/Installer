#pragma once

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// 主模板声明（放在头文件中）
template <typename Tag>
class embedded_resource {
    // 主模板不实现，强制特化
};

#ifndef _MSC_VER

// GCC / Clang: 使用 .incbin 汇编指令嵌入二进制文件
#define DEFINE_EMBEDDED_RESOURCE_TEMPLATE(resource_name)                    \
    template <>                                                             \
    class embedded_resource<struct resource_name##_tag> {                   \
       public:                                                              \
        embedded_resource() = delete;                                       \
                                                                            \
        static constexpr std::string_view data() {                          \
            return std::string_view(binary_##resource_name##_start,         \
                                    binary_##resource_name##_size);         \
        }                                                                   \
        static constexpr std::size_t size() {                               \
            return static_cast<std::size_t>(binary_##resource_name##_size); \
        }                                                                   \
        static std::span<const std::byte> bytes() {                         \
            return std::span<const std::byte>(                              \
                reinterpret_cast<const std::byte*>(                         \
                    binary_##resource_name##_start),                        \
                size());                                                    \
        }                                                                   \
        static constexpr bool is_empty() { return size() == 0; }            \
        static std::string to_string() { return std::string(data()); }      \
        static constexpr const char* begin() {                              \
            return binary_##resource_name##_start;                          \
        }                                                                   \
        static constexpr const char* end() {                                \
            return binary_##resource_name##_end;                            \
        }                                                                   \
    };                                                                      \
    using resource_name##_resource = embedded_resource<resource_name##_tag>;

#ifdef CMAKE_SOURCE_DIR_UNIX_PATH
#define EMBED_UNIX_PATH(resource_name, unix_file_path)                     \
    extern "C" {                                                           \
    extern const char binary_##resource_name##_start[];                    \
    extern const char binary_##resource_name##_end[];                      \
    extern const unsigned int binary_##resource_name##_size;               \
    }                                                                      \
    asm(".section .rdata,\"dr\"\n"                                         \
        ".globl binary_" #resource_name                                    \
        "_start\n"                                                         \
        "binary_" #resource_name                                           \
        "_start:\n"                                                        \
        "    .incbin \"" CMAKE_SOURCE_DIR_UNIX_PATH "/" unix_file_path     \
        "\"\n"                                                             \
        ".globl binary_" #resource_name                                    \
        "_end\n"                                                           \
        "binary_" #resource_name                                           \
        "_end:\n"                                                          \
        ".globl binary_" #resource_name                                    \
        "_size\n"                                                          \
        "binary_" #resource_name                                           \
        "_size:\n"                                                         \
        "    .long binary_" #resource_name "_end - binary_" #resource_name \
        "_start\n");                                                       \
    DEFINE_EMBEDDED_RESOURCE_TEMPLATE(resource_name)
#else
#define EMBED_UNIX_PATH(resource_name, unix_file_path)       \
    extern "C" {                                             \
    extern const char binary_##resource_name##_start[];      \
    extern const char binary_##resource_name##_end[];        \
    extern const unsigned int binary_##resource_name##_size; \
    }                                                        \
    DEFINE_EMBEDDED_RESOURCE_TEMPLATE(resource_name)
#endif

#else  // _MSC_VER

// MSVC: 不支持 .incbin 汇编，使用运行时文件加载
#define DEFINE_EMBEDDED_RESOURCE_TEMPLATE_MSVC(resource_name)               \
    template <>                                                             \
    class embedded_resource<struct resource_name##_tag> {                   \
        static std::vector<std::byte>& cache() {                            \
            static std::vector<std::byte> data;                             \
            return data;                                                    \
        }                                                                   \
        static void load() {                                                \
            auto& c = cache();                                              \
            if (!c.empty()) return;                                         \
            std::string path = resource_name##_file_path;                   \
            std::ifstream file(path, std::ios::binary | std::ios::ate);     \
            if (file) {                                                     \
                auto sz = file.tellg();                                     \
                file.seekg(0);                                              \
                c.resize(sz);                                               \
                file.read(reinterpret_cast<char*>(c.data()), sz);           \
            }                                                               \
        }                                                                   \
       public:                                                              \
        embedded_resource() = delete;                                       \
        static std::string_view data() {                                    \
            load();                                                         \
            auto& c = cache();                                              \
            return std::string_view(reinterpret_cast<const char*>(c.data()),\
                                    c.size());                              \
        }                                                                   \
        static std::size_t size() { load(); return cache().size(); }        \
        static std::span<const std::byte> bytes() {                         \
            load();                                                         \
            return std::span<const std::byte>(cache().data(),               \
                                              cache().size());              \
        }                                                                   \
        static bool is_empty() { return size() == 0; }                      \
        static std::string to_string() { return std::string(data()); }      \
        static const char* begin() { load(); return data().data(); }        \
        static const char* end() { load(); return data().data() + size(); } \
    };                                                                      \
    using resource_name##_resource = embedded_resource<resource_name##_tag>;

// MSVC 版本的 EMBED_UNIX_PATH：定义文件路径，运行时加载
#define EMBED_UNIX_PATH(resource_name, unix_file_path)     \
    constexpr const char resource_name##_file_path[] =     \
        CMAKE_SOURCE_DIR_UNIX_PATH "/" unix_file_path;     \
    DEFINE_EMBEDDED_RESOURCE_TEMPLATE_MSVC(resource_name)

#endif  // _MSC_VER