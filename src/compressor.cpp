#include "compressor.hpp"

#ifndef DEBUG
#define ASSERT(n)
#else
#define ASSERT(n) /* same as before */
#endif

Compressor::Compressor(const std::string& filename, std::size_t windowSize)
    : file_name(filename),
      windowSize(windowSize),
      searchBufferSize((windowSize / 2) - 1),
      lookaheadBufferSize((windowSize / 2) + 1),
      charBuffer(windowSize) {
    file.open(file_name, std::ios::binary);
    ASSERT(file.is_open());
    file.read(charBuffer.data(), charBuffer.size());
}

std::size_t Compressor::checkFileSize() { return std::filesystem::file_size(file_name); }

std::size_t Compressor::getWindowSize() const { return windowSize; }

std::vector<char> Compressor::getsearchBuffer() {
    std::vector<char> searchBuffer;
    for (size_t i = 0; i < searchBufferSize && i < charBuffer.size(); i++) {
        searchBuffer.push_back(charBuffer[i]);
    }
    return searchBuffer;
}

std::vector<char> Compressor::getlookaheadBuffer() {
    std::vector<char> lookaheadBuffer;
    for (size_t i = 0; i < lookaheadBufferSize && searchBufferSize + i < charBuffer.size(); i++) {
        lookaheadBuffer.push_back(charBuffer[i + searchBufferSize]);
    }
    return lookaheadBuffer;
}

std::vector<int> buildlsp(std::vector<char>& pattern) {
    std::vector<int> lsp(pattern.size(), 0);
    int len = 0;
    for (int i = 1; i < pattern.size();) {
        if (pattern[i] == pattern[len]) {
            lsp[i++] = ++len;
        } else if (len > 0) {
            len = lsp[len - 1];
        } else {
            lsp[i++] = 0;
        }
    }
    return lsp;
}

std::pair<int, int> KMPMatch(std::vector<char>& searchBuffer, std::vector<char>& lookaheadBuffer) {
    std::vector<int> lsp = buildlsp(lookaheadBuffer);
    int i = 0, j = 0;
    int bestLength = 0;
    int bestDistance = -1;

    while (i < searchBuffer.size()) {
        if (searchBuffer[i] == lookaheadBuffer[j]) {
            i++;
            j++;
            if (j > bestLength) {
                bestLength = j;
                bestDistance = i - j;
            }
            if (j == lookaheadBuffer.size()) {
                break;
            }
        } else {
            if (j != 0) {
                j = lsp[j - 1];
            } else {
                i++;
            }
        }
    }
    int distance = searchBuffer.size() - bestDistance;
    return {distance, bestLength};
}

void Compressor::LZ77(const std::string& outputFilename) {
    std::deque<char> window;
    std::ofstream output(outputFilename, std::ios::binary);
    // Read initial data from file
    file.read(charBuffer.data(), windowSize);
    std::size_t bytesRead = file.gcount();
    window.insert(window.end(), charBuffer.begin(), charBuffer.begin() + bytesRead);

    bool fileEnded = (bytesRead == 0);

    while (!fileEnded || window.size() >= lookaheadBufferSize) {
        if (!fileEnded && window.size() < windowSize) {
            file.read(charBuffer.data(), windowSize);
            bytesRead = file.gcount();

            if (bytesRead == 0) {
                fileEnded = true;
            } else {
                window.insert(window.end(), charBuffer.begin(), charBuffer.begin() + bytesRead);
            }
        }

        if (window.size() < lookaheadBufferSize) break;  // Not enough to proceed

        std::size_t searchEnd = std::min(searchBufferSize, window.size());
        std::size_t lookaheadEnd = std::min(window.size(), searchBufferSize + lookaheadBufferSize);

        std::vector<char> searchBuffer(window.begin(), window.begin() + searchEnd);
        std::vector<char> lookaheadBuffer(window.begin() + searchEnd,
                                          window.begin() + lookaheadEnd);

        if (lookaheadBuffer.empty()) break;

        auto [distance, length] = KMPMatch(searchBuffer, lookaheadBuffer);
        char nextChar = (length < lookaheadBuffer.size()) ? lookaheadBuffer[length] : '\0';

        if (length < 3) {
            distance = 0;
            length = 0;
            nextChar = lookaheadBuffer[0];
        }

        uint16_t d = distance;
        uint8_t l = length;
        output.write(reinterpret_cast<char*>(&d), sizeof(d));
        output.write(reinterpret_cast<char*>(&l), sizeof(l));
        output.write(&nextChar, 1);

        // Slide the window
        std::size_t slide = (length > 0) ? length + 1 : 1;
        for (std::size_t i = 0; i < slide && !window.empty(); ++i) {
            window.pop_front();
        }
    }
}
