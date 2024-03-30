#pragma once

#include <vector>
#include <string>
#include <fstream>

std::string vectorToHexString(const std::vector<uint8_t> &vec);

inline std::vector<uint8_t> readVectorFromDisk(const std::string& inputFilename) {
    std::ifstream instream(inputFilename, std::ios::in | std::ios::binary);
    if (instream.rdstate() & std::ios_base::failbit) {
        throw std::ios_base::failure("Could not open \"" + inputFilename + "\"");
    }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(instream)), std::istreambuf_iterator<char>());
    return data;
}