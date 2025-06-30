#ifndef COMPRESSOR_HPP
#define COMPRESSOR_HPP

#include <cctype>
#include <cstddef>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
struct Compressor {
    std::ifstream file;
    std::string file_name;
    std::vector<char> charBuffer;
    std::size_t windowSize;
    std::size_t searchBufferSize;
    std::size_t lookaheadBufferSize;

    Compressor(const std::string& filename, std::size_t windowSize = 32 * 1024);

    std::size_t checkFileSize();
    std::size_t getWindowSize() const;
    std::vector<char> getsearchBuffer();
    std::vector<char> getlookaheadBuffer();
    void LZ77(const std::string& outputFilename);
};
struct Match {
    int distance;
    int length;
    char nextChar;
};
#endif  // COMPRESSOR_HPP
