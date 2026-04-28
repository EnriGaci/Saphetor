#include "Utils.h"
#include "Logging.h"

#ifdef _WIN32
    const char* const TEST_VCF_FILE_PATH = "../../tests/assignment.sample.vcf";
#elif __linux__
    const char* const TEST_VCF_FILE_PATH = "../tests/assignment.sample.vcf";
#endif

namespace {
    struct LogLevelInitializer {
        LogLevelInitializer() {
            Logger::getInstance().setLevel(LogLevel::TEST);
        }
    };
    static LogLevelInitializer logLevelInit;
}