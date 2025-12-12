#pragma once

#include <span>
#include <string_view>

// 主模板声明（放在头文件中）
template <typename Tag>
class EmbeddedResource {
    // 主模板不实现，强制特化
};

// 定义资源特化的宏
#define DEFINE_EMBEDDED_RESOURCE_TEMPLATE(resource_name)                    \
    template <>                                                             \
    class EmbeddedResource<struct resource_name##_tag> {                    \
       public:                                                              \
        EmbeddedResource() = delete;                                        \
                                                                            \
        /* 获取资源数据 */                                                  \
        static constexpr std::string_view get_data() {                      \
            return std::string_view(binary_##resource_name##_start,         \
                                    binary_##resource_name##_size);         \
        }                                                                   \
                                                                            \
        /* 获取资源大小 */                                                  \
        static constexpr std::size_t get_size() {                           \
            return static_cast<std::size_t>(binary_##resource_name##_size); \
        }                                                                   \
                                                                            \
        /* 获取字节数组视图 */                                              \
        static std::span<const std::byte> get_bytes() {                     \
            return std::span<const std::byte>(                              \
                reinterpret_cast<const std::byte*>(                         \
                    binary_##resource_name##_start),                        \
                get_size());                                                \
        }                                                                   \
                                                                            \
        /* 检查资源是否为空 */                                              \
        static constexpr bool is_empty() { return get_size() == 0; }        \
                                                                            \
        /* 转换为字符串（如果资源是文本） */                                \
        static std::string to_string() { return std::string(get_data()); }  \
                                                                            \
        /* 获取开始指针 */                                                  \
        static constexpr const char* begin() {                              \
            return binary_##resource_name##_start;                          \
        }                                                                   \
                                                                            \
        /* 获取结束指针 */                                                  \
        static constexpr const char* end() {                                \
            return binary_##resource_name##_end;                            \
        }                                                                   \
    };                                                                      \
    /* 创建类型别名 */                                                      \
    using resource_name##_resource = EmbeddedResource<resource_name##_tag>;

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