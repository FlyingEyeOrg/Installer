#include <Windows.h>
#include <fmt/core.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "embed_assets.hpp"
#include "string_convertor.hpp"
#include "tar_archive.hpp"

EMBED_UNIX_PATH(hello_txt, "src/assets/helloworld.txt")

int main() {
    std::cout << "Reading embedded file content:\n";
    std::cout << "================================\n";

    size_t file_size = binary_hello_txt_end - binary_hello_txt_start;

    if (file_size > 0) {
        std::cout.write(binary_hello_txt_start, file_size);
    }

    std::cout << "\n================================\n";
    std::cout << "File size: " << file_size << " bytes\n";
    std::cout << "File size (from symbol): " << binary_hello_txt_size
              << " bytes\n";

    return 0;
}