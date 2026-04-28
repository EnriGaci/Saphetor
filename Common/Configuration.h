#pragma once
#include "CommonExport.h"
#include "ErrorHandler.h"

#include <string>


COMMON_API std::string getEnvVar(const char* var);


class COMMON_API Configuration
{
public:

    std::string getSqliteDBName() const;

public:
    static Configuration& getInstance();

    // Delete copy/move constructors and assignment operators
    Configuration(const Configuration&) = delete;
    Configuration& operator=(const Configuration&) = delete;
    Configuration(Configuration&&) = delete;
    Configuration& operator=(Configuration&&) = delete;

private:
    std::string getRequiredEnvVariable(const std::string& varName);

private:
    std::string mSQLiteDbName;
    Configuration();
};

