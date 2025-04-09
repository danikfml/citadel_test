#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <sstream>
#include <algorithm>

class DataStore {
public:
    void Set(const uint8_t *pData, size_t pSize) {
        fileNames.clear();
        totalSize = pSize;

        const size_t chunkSize = 20;
        size_t numFiles = (pSize + chunkSize - 1) / chunkSize;

        for (size_t i = 0; i < numFiles; ++i) {
            std::ostringstream oss;
            oss << "data_" << i << ".bin";
            std::string fileName = oss.str();

            std::ofstream ofs(fileName, std::ios::binary);
            if (!ofs) {
                std::cerr << "Не удалось открыть файл " << fileName << " для записи." << std::endl;
                continue;
            }

            size_t start = i * chunkSize;
            size_t bytesToWrite = std::min(chunkSize, pSize - start);

            ofs.write(reinterpret_cast<const char *>(pData + start), bytesToWrite);
            ofs.close();

            fileNames.push_back(fileName);
        }
    }

    bool Get(size_t pOffset, size_t pSize, std::vector<uint8_t> &pResult) {
        if (pOffset + pSize > totalSize) {
            std::cerr << "Запрашиваемый диапазон выходит за пределы хранимых данных." << std::endl;
            return false;
        }

        pResult.clear();
        pResult.reserve(pSize);

        const size_t chunkSize = 20;
        size_t bytesLeft = pSize;
        size_t currentOffset = pOffset;

        size_t fileIndex = currentOffset / chunkSize;
        size_t offsetInFile = currentOffset % chunkSize;

        while (bytesLeft > 0 && fileIndex < fileNames.size()) {
            std::ifstream ifs(fileNames[fileIndex], std::ios::binary);
            if (!ifs) {
                std::cerr << "Не удалось открыть файл " << fileNames[fileIndex] << " для чтения." << std::endl;
                return false;
            }

            ifs.seekg(offsetInFile, std::ios::beg);

            size_t bytesInFile = chunkSize - offsetInFile;
            size_t toRead = std::min(bytesLeft, bytesInFile);

            std::vector<uint8_t> buffer(toRead);
            ifs.read(reinterpret_cast<char *>(buffer.data()), toRead);
            size_t bytesActuallyRead = ifs.gcount();
            if (bytesActuallyRead < toRead) {
                std::cerr << "Ошибка чтения файла " << fileNames[fileIndex] << std::endl;
                return false;
            }
            pResult.insert(pResult.end(), buffer.begin(), buffer.end());

            bytesLeft -= toRead;
            fileIndex++;
            offsetInFile = 0;
        }

        return true;
    }

private:
    std::vector<std::string> fileNames;
    size_t totalSize = 0;
};
