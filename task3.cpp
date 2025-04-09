#include <iostream>
#include <fstream>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <string>
#include <zlib.h>

constexpr size_t BUFFER_SIZE = 16384;

volatile std::sig_atomic_t g_interrupted = 0;

void signalHandler(int signum) {
    g_interrupted = 1;
    std::cerr << "\nОперация прервана по сигналу " << signum << "\n";
}

bool compressFile(const std::string &inputFile, const std::string &outputFile) {
    unsigned char inBuffer[BUFFER_SIZE];
    unsigned char outBuffer[BUFFER_SIZE];

    std::ifstream in(inputFile, std::ios::binary);
    if (!in) {
        std::cerr << "Ошибка открытия входного файла: " << inputFile << "\n";
        return false;
    }
    std::ofstream out(outputFile, std::ios::binary);
    if (!out) {
        std::cerr << "Ошибка открытия выходного файла: " << outputFile << "\n";
        return false;
    }

    z_stream strm;
    std::memset(&strm, 0, sizeof(strm));

    if (deflateInit(&strm, Z_DEFAULT_COMPRESSION) != Z_OK) {
        std::cerr << "Ошибка инициализации сжатия.\n";
        return false;
    }

    int ret;
    int flush;

    do {
        if (g_interrupted) {
            std::cerr << "Сжатие прервано пользователем.\n";
            deflateEnd(&strm);
            return false;
        }

        in.read(reinterpret_cast<char *>(inBuffer), BUFFER_SIZE);
        std::streamsize bytesRead = in.gcount();
        if (in.bad()) {
            std::cerr << "Ошибка чтения входного файла.\n";
            deflateEnd(&strm);
            return false;
        }

        flush = in.eof() ? Z_FINISH : Z_NO_FLUSH;
        strm.avail_in = static_cast<uInt>(bytesRead);
        strm.next_in = inBuffer;

        do {
            strm.avail_out = BUFFER_SIZE;
            strm.next_out = outBuffer;
            ret = deflate(&strm, flush);
            if (ret == Z_STREAM_ERROR) {
                std::cerr << "Ошибка сжатия.\n";
                deflateEnd(&strm);
                return false;
            }
            size_t have = BUFFER_SIZE - strm.avail_out;
            out.write(reinterpret_cast<char *>(outBuffer), have);
            if (!out) {
                std::cerr << "Ошибка записи в выходной файл.\n";
                deflateEnd(&strm);
                return false;
            }
        } while (strm.avail_out == 0);
    } while (flush != Z_FINISH);

    deflateEnd(&strm);
    return true;
}

bool decompressFile(const std::string &inputFile, const std::string &outputFile) {
    unsigned char inBuffer[BUFFER_SIZE];
    unsigned char outBuffer[BUFFER_SIZE];

    std::ifstream in(inputFile, std::ios::binary);
    if (!in) {
        std::cerr << "Ошибка открытия входного файла: " << inputFile << "\n";
        return false;
    }
    std::ofstream out(outputFile, std::ios::binary);
    if (!out) {
        std::cerr << "Ошибка открытия выходного файла: " << outputFile << "\n";
        return false;
    }

    z_stream strm;
    std::memset(&strm, 0, sizeof(strm));

    if (inflateInit(&strm) != Z_OK) {
        std::cerr << "Ошибка инициализации распаковки.\n";
        return false;
    }

    int ret;

    do {
        if (g_interrupted) {
            std::cerr << "Распаковка прервана пользователем.\n";
            inflateEnd(&strm);
            return false;
        }

        in.read(reinterpret_cast<char *>(inBuffer), BUFFER_SIZE);
        std::streamsize bytesRead = in.gcount();
        if (in.bad()) {
            std::cerr << "Ошибка чтения входного файла.\n";
            inflateEnd(&strm);
            return false;
        }
        if (bytesRead == 0)
            break;

        strm.avail_in = static_cast<uInt>(bytesRead);
        strm.next_in = inBuffer;

        do {
            strm.avail_out = BUFFER_SIZE;
            strm.next_out = outBuffer;
            ret = inflate(&strm, Z_NO_FLUSH);
            if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
                std::cerr << "Ошибка распаковки.\n";
                inflateEnd(&strm);
                return false;
            }
            size_t have = BUFFER_SIZE - strm.avail_out;
            out.write(reinterpret_cast<char *>(outBuffer), have);
            if (!out) {
                std::cerr << "Ошибка записи в выходной файл.\n";
                inflateEnd(&strm);
                return false;
            }
        } while (strm.avail_out == 0);
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);
    return (ret == Z_STREAM_END);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Использование:\n"
                << "  Архивирование: " << argv[0] << " a <original file> <archive file>\n"
                << "  Распаковка:    " << argv[0] << " e <archive file> <original file>\n";
        return EXIT_FAILURE;
    }

    std::string mode = argv[1];
    std::string file1 = argv[2];
    std::string file2 = argv[3];

    std::signal(SIGINT, signalHandler);

    bool success = false;

    if (mode == "a") {
        std::cout << "Архивирование файла " << file1 << " в " << file2 << "\n";
        success = compressFile(file1, file2);
    } else if (mode == "e") {
        std::cout << "Распаковка файла " << file1 << " в " << file2 << "\n";
        success = decompressFile(file1, file2);
    } else {
        std::cerr << "Неверный режим работы: " << mode << "\n";
        return EXIT_FAILURE;
    }

    if (success) {
        std::cout << "Операция завершена успешно.\n";
        return EXIT_SUCCESS;
    } else {
        std::cerr << "Операция завершилась с ошибкой.\n";
        return EXIT_FAILURE;
    }
}
