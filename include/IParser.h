#pragma once
#include "VCFData.h"

#include <string>



class IParser {
public:

    virtual VCFData parse(const std::string& line) const = 0;

    virtual ~IParser() = default;
};
