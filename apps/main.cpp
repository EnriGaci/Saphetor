#include "VCFProcessor.h"
#include "ErrorHandler.h"
#include "VCFReader.h"
#include "VCFLineParser.h"
#include "IVCFDal.h"
#include "SQLiteVCFDal.h"
#include "Configuration.h"
#include "Logging.h"

#include <iostream>
#include <string>
#include <tuple>
#include <thread>
#include <unordered_map>

std::tuple<std::string, int, bool, LogLevel> parseArgs(int argc, char* argv[]) {
    std::string filePath;
    unsigned int hardwareConcurrency = std::thread::hardware_concurrency();
    int threads = hardwareConcurrency ? hardwareConcurrency : 1; // Default to 1 thread
    bool reset = false;
    LogLevel logLevel = LogLevel::INFO;

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--vcf" && i + 1 < argc) {
            filePath = argv[++i];
        }
        else if (std::string(argv[i]) == "--threads" && i + 1 < argc) {
            threads = std::stoi(argv[++i]);
        }
        else if (std::string(argv[i]) == "--log-level" && i + 1 < argc) {
            std::string level = argv[++i];
            if (level == "INFO") {
                logLevel = LogLevel::INFO;
            }
            else if (level == "WARNING") {
                logLevel = LogLevel::WARNING;
            }
            else if (level == "DEBUG") {
                logLevel = LogLevel::DEBUG;
            }
            else if (level == "ERROR") {
                logLevel = LogLevel::ERROR;
            }
            else {
                LOG(LogLevel::WARNING, "Invalid log level specified. Use INFO, WARNING, DEBUG, or ERROR.");
            }
        }
        else if (std::string(argv[i]) == "--reset") {
            reset = true;
        }
        if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h") {
            std::cout << "Usage: vcf_importer [OPTIONS]\n\n"
                << "Options:\n"
                << "  --vcf PATH            Path to input VCF\n"
                << "  --threads N           Number of threads\n"
                << "  --log-level LEVEL     INFO WARNING DEBUG or ERROR\n"
                << "  --reset               When passed clears the tables in storage\n";
            std::exit(0); // Return empty path and 0 threads to indicate help was requested
        }
    }

    if (filePath.empty()) {
        RaiseError(ErrorCodes::MissingVCFFilePath, "Error: --vcf option is required.");
    }

    return { filePath, threads, reset, logLevel };
}

int main(int argv, char* argc[]) {

    auto [filePath, numberOfThreads, clearDb, logLevel] = parseArgs(argv, argc);

    Logger::getInstance().setLevel(logLevel);

    std::unique_ptr<IFileReader> reader = std::make_unique<VCFReader>(filePath);
    std::unique_ptr<IParser> parser = std::make_unique<VCFLineParser>(filePath);
    std::unique_ptr<IVCFDal> dal = std::make_unique<SQLiteVCFDal>(Configuration::getInstance().getSqliteDBName());
    dal->initDb(clearDb);

    VCFProcessor processor (std::move(reader), std::move(parser), std::move(dal), numberOfThreads);

    LOG(LogLevel::INFO, "Starting VCF processing with file: " + filePath + " using " + std::to_string(numberOfThreads) + " threads");

    processor.process();

    dal = std::make_unique<SQLiteVCFDal>(Configuration::getInstance().getSqliteDBName());

    LOG(LogLevel::INFO, "Fetching all records from DB to verify insertion");

    auto records = dal->fetchAllVariants();

    std::cout << "Total records in DB: " << records.size() << std::endl;

    return 0;
}