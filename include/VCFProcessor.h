#pragma once
#include "ThreadPool.h"
#include "ErrorHandler.h"

#include <fstream>
#include <string>
#include <string_view>
#include <sstream>
#include <variant>

class IFileReader {
public:

    virtual std::vector<std::string> getBatch() = 0;

    virtual ~IFileReader() = default;
};

class VCFReader : public IFileReader
{
public:
    VCFReader(const std::string& filePath, size_t batchSize = 100) : mBatchSize(batchSize) {
        mFileStream = std::ifstream(filePath);

        if (!mFileStream.is_open()) {
            RaiseError(ErrorCodes::CouldNotOpenVCFFile, "Could not open file: " + filePath);
        }

        moveStreamToDataBegin();
    }

    std::vector<std::string> getBatch() override {
        // Read a batch of data from the file stream and return it
        size_t i = 0;
        std::vector<std::string> batch(mBatchSize);
        while (i < mBatchSize) {
            std::string line;
            if (!std::getline(mFileStream, line)) {
                break; // End of file or error
            }
            batch[i]=line;
            i++;
        }

        if (i != mBatchSize) {
            batch.resize(i);
        }

        return batch;
    }

private:
    // Moves stream passed information fields
    void moveStreamToDataBegin() {
        std::string line;
        while (std::getline(mFileStream, line)) {
            if (!line.empty() && line.size() > 1 && line[1] != '#') {
                break;
            }
        }
    }

private:
    std::ifstream mFileStream;
    size_t mBatchSize;
};


using InfoValue = std::variant<std::string, int, float, bool>;
struct VCFData {
    std::string chrom;
    int pos;
    std::string ref;
    std::string alt;
    std::unordered_map<std::string, InfoValue> info;
    std::unordered_map<std::string, std::string> format;
};

class VCFLineParser {
public:
    VCFLineParser(std::string filePath) {
        setInfoFieldsTypes(filePath);
    }


    VCFData parse(const std::string& line) const {
        VCFData data;
        // Implement parsing logic here to fill the VCFData structure
        auto tokens = split(line);
        
        if (tokens.size() < MIN_EXPECTED_TOKENS_IN_LINE) {
            RaiseWarning(ErrorCodes::InvalidNumberOfDataInLine, "Unexpected number of tokens in line: " + line);

        }

        data.chrom = tokens[TOKEN_CHROM];
        data.pos = std::stoi(tokens[TOKEN_POS]);
        data.ref = tokens[TOKEN_REF];
        data.alt = getTokenValue(tokens[TOKEN_ALT]);

        setInfo(data, tokens[TOKEN_INFO]);

        if (tokens.size() > TOKEN_FORMAT) {
            auto format_data = split(tokens[TOKEN_FORMAT], ':');
            for (const auto& kv : format_data) {
                auto kv_pair = split(kv, '=');
                if (kv_pair.size() == 2) {
                    data.info[kv_pair[0]] = kv_pair[1];
                }
            }
        }

        return data;
    }
private:

    void setInfoFieldsTypes(std::string filePath) {
        auto fileStream = std::ifstream(filePath);

        if (!fileStream.is_open()) {
            RaiseError(ErrorCodes::CouldNotOpenVCFFile, "Could not open file: " + filePath);
        }

        std::string line;
        while (std::getline(fileStream, line)) {
            if (!line.empty() && line.size() > 1 && line[1] == '#') {
                if (line.substr(0, 8) == "##INFO=<") {
                    auto info_line = line.substr(8, line.size() - 9); // Remove "##INFO=<" and ">"
                    auto info_data = split(info_line, ',');
                    std::string id;
                    std::string type;
                    for (const auto& kv : info_data) {
                        auto kv_pair = split(kv, '=');
                        if (kv_pair.size() == 2) {
                            if (kv_pair[0] == "ID") {
                                id = kv_pair[1];
                            }
                            else if (kv_pair[0] == "Type") {
                                type = kv_pair[1];
                            }
                        }
                    }

                    if (!id.empty() && !type.empty()) {
                        if (type == "String") {
                            mInfoFieldTypes[id] = InfoType::String;
                        }
                        else if (type == "Integer") {
                            mInfoFieldTypes[id] = InfoType::Integer;
                        }
                        else if (type == "Float") {
                            mInfoFieldTypes[id] = InfoType::Float;
                        }
                        else if (type == "Flag") { // VCF specification uses "Flag" for boolean fields
                            mInfoFieldTypes[id] = InfoType::Boolean;
                        }
                    }
                }
            }
            else {
                break; // Stop reading after header lines
            }
        }
    }

    void setInfo(VCFData& data,std::string& info_token) const {
        if (info_token.empty()) { return; }
        if (info_token == ".") { return; }

        auto info_data = split(info_token, ';');
        for (const auto& kv : info_data) {
            auto kv_pair = split(kv, '=');
            if (kv_pair.size() == 2) {
                setDataInfo(data, kv_pair[0], kv_pair[1]);
                data.info[kv_pair[0]] = kv_pair[1];
            }
        }

    }

    void setDataInfo(VCFData& data, const std::string& key, const std::string& value) const {
        auto it = mInfoFieldTypes.find(key);
        if (it != mInfoFieldTypes.end()) {
            switch (it->second) {
            case InfoType::String:
                data.info[key] = value;
                break;
            case InfoType::Integer:
                data.info[key] = std::stoi(value);
                break;
            case InfoType::Float:
                data.info[key] = std::stof(value);
                break;
            case InfoType::Boolean:
                data.info[key] = (value == "true" || value == "1");
                break;
            }
        }
        else {
            RaiseWarning(ErrorCodes::UnknownInfoFieldType, "Unknown INFO field type for key: " + key + ". Defaulting to string.");
            // Default to string if type is not defined
            data.info[key] = value;
        }
    }

    std::vector<std::string> split(const std::string& line, char delim='\t') const {
        std::vector<std::string> tokens;
        std::istringstream ss(line);
        std::string token;

        while (std::getline(ss, token, delim)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    std::string getTokenValue(std::string token) const {
        return token == "." ? "" : token;
    }

private:
    static const int TOKEN_CHROM = 0;
    static const int TOKEN_POS = 1;
    static const int TOKEN_ID = 2;
    static const int TOKEN_REF = 3;
    static const int TOKEN_ALT = 4;
    static const int TOKEN_QUAL = 5;
    static const int TOKEN_FILTER = 6;
    static const int TOKEN_INFO = 7;
    static const int TOKEN_FORMAT = 8;
    static const int MIN_EXPECTED_TOKENS_IN_LINE = 9;

    enum InfoType {
        String,
        Integer,
        Float,
        Boolean
    };

private:

    std::unordered_map<std::string, InfoType> mInfoFieldTypes;

};

class VCFProcessor
{
public:
    VCFProcessor(std::unique_ptr<IFileReader> fileReader, int numThreads) : mFileReader(std::move(fileReader)), mThreadPool(numThreads)  { }

    void process() {
        // Implement VCF processing logic here

        auto batch = mFileReader->getBatch();

        while (!batch.empty()) {
            mThreadPool.enqueue([this, batch] {
                // Process the batch of data
                processBatch(batch);
                });
            batch = mFileReader->getBatch();
        }

    }

    virtual void processBatch(const std::vector<std::string>& batch) {
        // Implement the logic to process a batch of data
        return;

        //for (const auto& line : batch) {
        //    // Process each line of the batch
        //    //strore line
        //}
    }

    virtual ~VCFProcessor() = default;

private:
    ThreadPool mThreadPool;
    std::unique_ptr<IFileReader> mFileReader;
};

