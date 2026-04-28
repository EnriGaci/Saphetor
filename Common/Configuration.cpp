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

std::string Configuration::getSqliteDBName() const { return mSQLiteDbName; }

Configuration& Configuration::getInstance()
{
    static Configuration instance;
    return instance;
}

std::string Configuration::getRequiredEnvVariable(const std::string& varName)
{
    std::string value = getEnvVar(varName.c_str());
    if (value.empty())
    {
        RaiseError(ErrorCodes::MissingEnvironmentalVariable, "Required environment variable '" + varName + "' is not set.");

    }
    return value;
}

Configuration::Configuration() {
    // Load configuration parameters from environment variables or other sources
    mSQLiteDbName = getRequiredEnvVariable("SQLITEDBNAME");
}
