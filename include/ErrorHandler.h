#pragma once
#include <iostream>


enum class ErrorCodes {

    Success = 0,
    MissingEnvironmentalVariable = 10,
    CouldNotOpenVCFFile = 11,
    MissingVCFFilePath = 12,
    UnknownError
};

void RaiseError(ErrorCodes code, const std::string& message) {
    std::cerr << "Error: " << message << std::endl;
    std::exit(static_cast<int>(code));
}
