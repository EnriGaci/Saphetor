#include "Utils.h"

#ifdef _WIN32
    const char* const TEST_VCF_FILE_PATH = "../../tests/assignment.sample.vcf";
#elif __linux__
    const char* const TEST_VCF_FILE_PATH = "../tests/assignment.sample.vcf";
#endif
