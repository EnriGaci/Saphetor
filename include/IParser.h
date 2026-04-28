#pragma once
#include "VCFData.h"

#include <string>



class IParser {
public:

    /*
    * @brief parses a line from the VCF file and converts it into a VCFData object. The implementation of this method should handle the specific format of the VCF file, including splitting the line into fields, validating the data, and populating the VCFData structure accordingly.
     *
     * @param line A string representing a single line from the VCF file, which contains variant information in a tab-delimited format.
     * @return A VCFData object that encapsulates the parsed information from the input line.
    */
    virtual VCFData parse(const std::string& line) const = 0;

    virtual ~IParser() = default;
};
