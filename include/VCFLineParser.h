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
    VCFLineParser(const std::string& filePath, const Validations::ValidationsType& validations = Validations::DefaultValidations):
        mValidators(validations)
    {
        setFieldsTypes(filePath, mInfoFieldTypes, "##INFO=<");
        setFieldsTypes(filePath, mFormatFieldTypes, "##FORMAT=<");
    }

    void validate(const std::vector<std::string>& tokens) const {
        // Implement validation logic here, e.g., check for required fields, validate data types, etc.
        if (tokens.size() < MIN_EXPECTED_TOKENS_IN_LINE) {
            LOG(LogLevel::WARNING, std::string("Unexpected number of tokens in line: ") + std::to_string(tokens.size()) + ". Expected at least: " + std::to_string(MIN_EXPECTED_TOKENS_IN_LINE));
        }

        for (auto [identifier, validator] : mValidators) {
            std::string token = tokens[identifier];
            validator->validate(token);
        }
    }

    VCFData parse(const std::string& line) const {
        VCFData data;

        auto tokens = split(line);

        validate(tokens);

        data.chrom = tokens[TOKEN_CHROM];
        data.pos = std::stoi(tokens[TOKEN_POS]);
        data.ref = tokens[TOKEN_REF];
        data.alt = tokens[TOKEN_ALT];
        data.filter = tokens[TOKEN_FILTER];
        data.qual = std::stof(tokens[TOKEN_QUAL]);

        setInfo(data, tokens[TOKEN_INFO]);
        setFormat(data, tokens[TOKEN_FORMAT]);

        return data;
    }

private:

    enum InfoType {
        String,
        Integer,
        Float,
        Boolean
    };

private:

    void setFieldsTypes(const std::string& filePath, std::unordered_map<std::string, InfoType>& fieldTypes, const std::string& type) {
        auto fileStream = std::ifstream(filePath);

        if (!fileStream.is_open()) {
            RaiseError(ErrorCodes::CouldNotOpenVCFFile, "Could not open file: " + filePath);
        }

        std::string line;
        while (std::getline(fileStream, line)) {
            if (!line.empty() && line.size() > 1 && line[1] == '#') {
                if (line.substr(0, type.size()) == type) {
                    auto info_line = line.substr(type.size(), line.size() - (type.size()+1)); // Remove "##INFO=<" and ">"
                    auto info_data = split(info_line, ',');
                    std::string id;
                    std::string type_;
                    for (const auto& kv : info_data) {
                        auto kv_pair = split(kv, '=');
                        if (kv_pair.size() == 2) {
                            if (kv_pair[0] == "ID") {
                                id = kv_pair[1];
                            }
                            else if (kv_pair[0] == "Type") {
                                type_ = kv_pair[1];
                            }
                        }
                    }

                    if (!id.empty() && !type_.empty()) {
                        if (type_ == "String") {
                            fieldTypes[id] = InfoType::String;
                        }
                        else if (type_ == "Integer") {
                            fieldTypes[id] = InfoType::Integer;
                        }
                        else if (type_ == "Float") {
                            fieldTypes[id] = InfoType::Float;
                        }
                        else if (type_ == "Flag") { // VCF specification uses "Flag" for boolean fields
                            fieldTypes[id] = InfoType::Boolean;
                        }
                    }
                }
            }
            else {
                break; // Stop reading after header lines
            }
        }
    }

    void setInfo(VCFData& data,std::string& infoToken) const {
        if (infoToken.empty()) { return; }
        if (infoToken == ".") { return; }

        auto infoData = split(infoToken, ';');
        for (const auto& kv : infoData) {
            auto kv_pair = split(kv, '=');
            if (kv_pair.size() == 2) {
                setDataMap(data.info, mInfoFieldTypes, kv_pair[0], kv_pair[1]);
            }
        }

    }

    void setFormat(VCFData& /*data*/, std::string& formatToken) const {
        if (formatToken.empty()) { return; }
        if (formatToken == ".") { return; }

        //auto info_data = split(formatToken, ';');
        //for (const auto& kv : info_data) {
        //    auto kv_pair = split(kv, '=');
        //    if (kv_pair.size() == 2) {
        //        setDataMap(data.info, mInfoFieldTypes, kv_pair[0], kv_pair[1]);
        //    }
        //}

    }

    void setDataMap(VCFDataMap& dataMap, const std::unordered_map<std::string, InfoType>& fieldTypes, const std::string& key, const std::string& value) const {
        auto it = fieldTypes.find(key);
        if (it != fieldTypes.end()) {
            switch (it->second) {
            case InfoType::String:
                dataMap[key] = value;
                break;
            case InfoType::Integer:
                dataMap[key] = std::stoi(value);
                break;
            case InfoType::Float:
                dataMap[key] = std::stof(value);
                break;
            case InfoType::Boolean:
                dataMap[key] = (value == "true" || value == "1");
                break;
            }
        }
        else {
            RaiseWarning("Unknown INFO field type for key: " + key + ". Defaulting to string.");
            // Default to string if type is not defined
            dataMap[key] = value;
        }
    }

    //std::vector<std::string_view> split(const std::string_view line, char delim = '\t') const {
    //    std::vector<std::string_view> tokens;
    //    size_t start = 0;
    //    while (start < line.size()) {
    //        size_t end = line.find(delim, start);
    //        if (end == std::string_view::npos) {
    //            tokens.emplace_back(line.substr(start));
    //            break;
    //        }
    //        tokens.emplace_back(line.substr(start, end - start));
    //        start = end + 1;
    //    }
    //    return tokens;
    //}

    std::vector<std::string> split(const std::string& line, char delim='\t') const {
        std::vector<std::string> tokens;
        std::istringstream ss(line);
        std::string token;

        while (std::getline(ss, token, delim)) {
            tokens.push_back(token);
        }
        return tokens;
    }

private:
    std::unordered_map<std::string, InfoType> mInfoFieldTypes;
    std::unordered_map<std::string, InfoType> mFormatFieldTypes;
    Validations::ValidationsType mValidators;
};
