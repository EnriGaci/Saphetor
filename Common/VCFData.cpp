#include "VCFData.h"
#include <sstream>

std::string VCFData::toString() {
    std::ostringstream oss;
    oss << "chrom: " << chrom
        << ", pos: " << pos
        << ", ref: " << ref
        << ", alt: " << alt
        << ", filter: " << filter
        << ", qual: " << qual
        << ", info: {";
    bool first = true;
    for (const auto& [key, value] : info) {
        if (!first) oss << ", ";
        oss << key << ": ";
        std::visit([&oss](auto&& arg) { oss << arg; }, value);
        first = false;
    }
    oss << "}, format: {";
    first = true;
    for (const auto& [key, value] : format) {
        if (!first) oss << ", ";
        oss << key << ": ";
        std::visit([&oss](auto&& arg) { oss << arg; }, value);
        first = false;
    }
    oss << "}";
    return oss.str();
}
