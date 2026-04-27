#include "VCFData.h"


std::ostream& operator<<(std::ostream& os, const VCFData& data) {
    os << "chrom: " << data.chrom
       << ", pos: " << data.pos
       << ", ref: " << data.ref
       << ", alt: " << data.alt
       << ", filter: " << data.filter
       << ", qual: " << data.qual;
    os << ", info: {";
    bool first = true;
    for (const auto& [key, value] : data.info) {
        if (!first) os << ", ";
        os << key << ": ";
        std::visit([&os](auto&& arg) { os << arg; }, value);
        first = false;
    }
    os << "}, format: {";
    first = true;
    for (const auto& [key, value] : data.format) {
        if (!first) os << ", ";
        os << key << ": ";
        std::visit([&os](auto&& arg) { os << arg; }, value);
        first = false;
    }
    os << "}";
    return os;
}
