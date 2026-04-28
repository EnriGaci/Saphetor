#pragma once
#include "CommonExport.h"

#include <iostream>

enum class COMMON_API ErrorCodes {
    Success = 0,

    // Errors
    MissingEnvironmentalVariable = 10,
    CouldNotOpenVCFFile = 11,
    MissingVCFFilePath = 12,
    CouldNotOpenDatabase = 13,

    // Warnings
    InvalidNumberOfDataInLine = 100,
    UnknownInfoFieldType = 101,
    ParsingError = 101,


    UnknownError
};

COMMON_API void RaiseError(ErrorCodes code, const std::string& message);

COMMON_API void RaiseWarning(const std::string& message);
