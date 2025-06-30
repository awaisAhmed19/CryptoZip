#include <chrono>
#include <filesystem>
#include <iostream>

#include "compressor.hpp"

int main() {
    using namespace std::chrono;

    auto start = high_resolution_clock::now();

    Compressor zip("bigfile.txt");
    zip.LZ77("compressed.lz77");

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start).count();

    std::size_t originalSize = std::filesystem::file_size("bigfile.txt");
    std::size_t compressedSize = std::filesystem::file_size("compressed.lz77");

    double ratio = (double)compressedSize / (double)originalSize;

    std::cout << "Original Size: " << originalSize << " bytes\n";
    std::cout << "Compressed Size: " << compressedSize << " bytes\n";
    std::cout << "Compression Ratio: " << ratio << "\n";
    std::cout << "Time Taken: " << duration << " ms\n";

    return 0;
}
