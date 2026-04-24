#include "VCFProcessor.h"
#include "ErrorHandler.h"

#include <iostream>
#include <string>
#include <tuple>
#include <thread>
#include <unordered_map>

std::tuple<std::string, int> parseArgs(int argc, char* argv[]) {
    std::string filePath;
    unsigned int hardwareConcurrency = std::thread::hardware_concurrency();
    int threads = hardwareConcurrency ? hardwareConcurrency : 1; // Default to 1 thread

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--vcf" && i + 1 < argc) {
            filePath = argv[++i];
        }
        else if (std::string(argv[i]) == "--threads" && i + 1 < argc) {
            threads = std::stoi(argv[++i]);
        }
        if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h") {
            std::cout << "Usage: vcf_importer [OPTIONS]\n\n"
                << "Options:\n"
                << "  --vcf PATH      Path to input VCF\n"
                << "  --threads N     Number of threads\n";
            std::exit(0); // Return empty path and 0 threads to indicate help was requested
        }
    }

    if (filePath.empty()) {
        RaiseError(ErrorCodes::MissingVCFFilePath, "Error: --vcf option is required.");
    }

    return { filePath, threads };
}

int main(int argv, char* argc[]) {

    auto [filePath, threads] = parseArgs(argv, argc);

    std::unique_ptr<IFileReader> reader = std::make_unique<VCFReader>(filePath);
    std::unique_ptr<IParser> parser = std::make_unique<VCFLineParser>(filePath);
    std::unique_ptr<IVCFDal> dal = std::make_unique<SQLiteVCFDal>(Configuration::getInstance().getSqliteDBName());

    VCFProcessor processor (std::move(reader), std::move(parser), std::move(dal), threads);

    processor.process();

}