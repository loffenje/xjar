#pragma once

#include <string_view>
#include <fstream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>

namespace xjar {
std::vector<char> ReadFile(std::string_view filename) {
    std::ifstream f(filename.data(), std::ios::ate | std::ios::binary);
    if (!f.is_open()) {
        fprintf(stderr, "Failed to open file\n");
        exit(EXIT_FAILURE);
    }

    size_t fileSize = (size_t)f.tellg();
    std::vector<char> buffer(fileSize);
    f.seekg(0);
    f.read(buffer.data(), fileSize);
    f.close();

    return buffer;
}

}

