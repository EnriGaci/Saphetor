#pragma once
#include <iostream>

enum class ErrorCodes {
    Success = 0,

    // Errors
    MissingEnvironmentalVariable = 10,
    CouldNotOpenVCFFile = 11,
    MissingVCFFilePath = 12,
    CouldNotOpenDatabase = 13,

    // Warnings
    InvalidNumberOfDataInLine = 100,
    UnknownInfoFieldType = 101,


    UnknownError
};

void RaiseError(ErrorCodes code, const std::string& message);

void RaiseWarning(ErrorCodes code, const std::string& message);
