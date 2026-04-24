#include "Configuration.h"

std::string getEnvVar(const char* var) {
#ifdef _WIN32
    char* value = nullptr;
    size_t len = 0;
    _dupenv_s(&value, &len, var);
    std::string result = value ? value : "";
    free(value);
    return result;
#else
    const char* value = std::getenv(var);
    return value ? std::string(value) : std::string();
#endif
}
