#pragma once

#define EMBED_FILE(var_name, file_path)                 \
    extern "C" {                                        \
    extern const char binary_##var_name##_start[];      \
    extern const char binary_##var_name##_end[];        \
    extern const unsigned int binary_##var_name##_size; \
    }                                                   \
    asm(".section .rdata,\"dr\"\n"                      \
        ".globl binary_" #var_name                      \
        "_start\n"                                      \
        "binary_" #var_name                             \
        "_start:\n"                                     \
        "    .incbin \"" file_path                      \
        "\"\n"                                          \
        ".globl binary_" #var_name                      \
        "_end\n"                                        \
        "binary_" #var_name                             \
        "_end:\n"                                       \
        ".globl binary_" #var_name                      \
        "_size\n"                                       \
        "binary_" #var_name                             \
        "_size:\n"                                      \
        "    .long binary_" #var_name "_end - binary_" #var_name "_start\n");
