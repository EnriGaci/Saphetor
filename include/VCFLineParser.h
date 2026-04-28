#pragma once
#include "Logging.h"
#include "IParser.h"
#include "Validators.h"
#include "ErrorHandler.h"

#include <fstream>
#include <sstream>
#include <string>


class VCFLineParser : public IParser {
public:
    /*
    * @brief Initializes the parser with the types of the INFO and FORMAT fields
    *
    * @param validations map of data TYPE to Validator decorator
    */
    VCFLineParser(const std::string& filePath, const Validations::ValidationsType& validations = Validations::DefaultValidations);

    /*
    * @brief Parse and validate a line from VCF file
    *
    * @param line A string representing a single line from the VCF file, which contains variant information in a tab-delimited format.
    * @return A VCFData object that encapsulates the parsed information from the input line.
    */
    VCFData parse(const std::string& line) const;

private:

    /*
    * @brief helper enum for INFO value types
    */
    enum InfoType {
        String,
        Integer,
        Float,
        Boolean
    };

private:

    /*
    * @brief Validated VCF line's read tokens against validations map
    *
    * @throws std::runtime_error if any of the validations fail
    */
    void validate(const std::vector<std::string>& tokens) const;

    /*
    * @brief Reads the VCF file and extracts the types of the INFO and FORMAT fields, storing them in the provided maps. 
    *
    * @param filePath The path to the VCF file to be read.
    * @param fieldTypes A reference to an unordered map where the method will store the field names as keys and their corresponding types as values.
    * @param type A string indicating whether to parse INFO or FORMAT fields (e.g., "##INFO=<" or "##FORMAT=<").
    * @throws std::runtime_error if the file cannot be opened or if there is an error during parsing.
    */
    void setFieldsTypes(const std::string& filePath, std::unordered_map<std::string, InfoType>& fieldTypes, const std::string& type);

    /* 
    * @brief Set info values from infoToken in data.info
    *
    * @param data Where the info will be stored
    * @param infoToken A string representing the INFO field from the VCF line, which contains key-value pairs separated by semicolons (e.g., "DP=100;AF=0.5").
    * @throws std::runtime_error if there is an error during parsing or if a required field is missing.
    */
    void setInfo(VCFData& data,std::string& infoToken) const;

    void setFormat(VCFData& /*data*/, std::string& formatToken) const;

    /*
    * @brief Set a key-value pair in the provided data map, converting the value to the appropriate type based on the field types defined in the parser.
    */
    void setDataMap(VCFDataMap& dataMap, const std::unordered_map<std::string, InfoType>& fieldTypes, const std::string& key, const std::string& value) const;

    /*
    * @brief Split string given the delimiter
    */
    std::vector<std::string> split(const std::string& line, char delim='\t') const;

private:
    std::unordered_map<std::string, InfoType> mInfoFieldTypes;
    std::unordered_map<std::string, InfoType> mFormatFieldTypes;
    const Validations::ValidationsType& mValidators;
};
