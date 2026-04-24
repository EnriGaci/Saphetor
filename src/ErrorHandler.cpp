#include "ErrorHandler.h"

void RaiseError(ErrorCodes code, const std::string& message) {
    std::cerr << "Error: " << message << std::endl;
    std::exit(static_cast<int>(code));
}

void RaiseWarning(ErrorCodes code, const std::string& message) {
    std::cerr << "Warning: " << message << std::endl;
}
