#pragma once

#include <vector>
#include <string>
#include <fstream>

std::string vectorToHexString(const std::vector<uint8_t> &vec);

inline std::vector<uint8_t> readVectorFromDisk(const std::string& inputFilename) {
    std::ifstream instream(inputFilename, std::ios::in | std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(instream)), std::istreambuf_iterator<char>());
    return data;
}