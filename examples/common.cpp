#include "common.hpp"

#include <stdio.h>

std::string read_file(const char* path)
{
    const auto f = fopen(path, "r");
    fseek(f, 0, SEEK_END);
    const auto size = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::string data;
    data.resize((size_t)size);
    fread(data.data(), 1, data.size(), f);
    fclose(f);
    return data;
}