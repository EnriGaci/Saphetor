#pragma once
#include "ErrorHandler.h"

#include <string>


std::string getEnvVar(const char* var);


class Configuration
{
public:

    std::string getSqliteDBName() const { return mSQLiteDbName; }


public:
    static Configuration& getInstance()
    {
        static Configuration instance;
        return instance;
    }

    // Delete copy/move constructors and assignment operators
    Configuration(const Configuration&) = delete;
    Configuration& operator=(const Configuration&) = delete;
    Configuration(Configuration&&) = delete;
    Configuration& operator=(Configuration&&) = delete;

private:
    std::string getRequiredEnvVariable(const std::string& varName)
    {
        std::string value = getEnvVar(varName.c_str());
        if (value.empty())
        {
            RaiseError(ErrorCodes::MissingEnvironmentalVariable, "Required environment variable '" + varName + "' is not set.");

        }
        return value;
    }

private:

    std::string mSQLiteDbName;

    Configuration() {
        // Load configuration parameters from environment variables or other sources
        mSQLiteDbName = getRequiredEnvVariable("SQLITEDBNAME");
    }
};

