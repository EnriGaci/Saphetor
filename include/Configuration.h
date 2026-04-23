#ifndef __CONFIGURAION_H__
#define __CONFIGURAION_H__

#include "ErrorHandler.h"

#include <string>

class Configuration
{
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
        const char* value = std::getenv(varName.c_str());
        if (!value)
        {
            RaiseError(ErrorCodes::MissingEnvironmentalVariable, "Required environment variable '" + varName + "' is not set.");

        }
        return value;
    }

private:

    std::string database;

    Configuration() {
        // Load configuration parameters from environment variables or other sources
        database = getRequiredEnvVariable("DATABASE");
    }
}

#endif // !__CONFIGURAION_H__

