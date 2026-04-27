#pragma once

#ifdef _MSC_VER
#pragma warning(disable: 4251)
#endif

#include <variant>
#include <unordered_map>
#include <string>
#include <iostream>

#include "CommonExport.h"


using VCFDataMapValue = std::variant<std::string, int, float, bool>;
using VCFDataMap = std::unordered_map<std::string, VCFDataMapValue>;

struct COMMON_API VCFData {
    std::string chrom{ "" };
    int pos{ 0 };
    std::string ref{ "" };
    std::string alt{ "" };
    std::string filter{ "" };
    float qual{ 0.0 };
    VCFDataMap info{};
    VCFDataMap format{};
};

std::ostream& operator<<(std::ostream& os, const VCFData& data);
