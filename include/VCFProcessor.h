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


using VCFDataMapValue = std::variant<std::string, int, float, bool>;
using VCFDataMap = std::unordered_map<std::string, VCFDataMapValue>;

struct VCFData {
    std::string chrom;
    int pos;
    std::string ref;
    std::string alt;
    std::string filter;
    float qual;
    VCFDataMap info;
    VCFDataMap format;
};

class IParser {
public:

    virtual VCFData parse(const std::string& line) const = 0;

    virtual ~IParser() = default;
};

class VCFLineParser : public IParser {
public:
    VCFLineParser(std::string filePath) {
        setFieldsTypes(filePath, mInfoFieldTypes, "##INFO=<");
        setFieldsTypes(filePath, mFormatFieldTypes, "##FORMAT=<");
    }

    void validate(const std::vector<std::string>& tokens) const {
        // Implement validation logic here, e.g., check for required fields, validate data types, etc.
        if (tokens.size() < MIN_EXPECTED_TOKENS_IN_LINE) {
            RaiseWarning(ErrorCodes::InvalidNumberOfDataInLine, "Unexpected number of tokens in line: " + std::to_string(tokens.size()));
        }

        std::vector<int> requiredTokens = { TOKEN_CHROM, TOKEN_POS, TOKEN_REF };

        for (int tokenIndex : requiredTokens) {
            if (tokenIndex >= tokens.size()) {
                throw std::runtime_error("Missing required token at index: " + std::to_string(tokenIndex));
            }

            std::string value = tokens[tokenIndex];
            if (value.empty()) {
                throw std::runtime_error("Required value at column " + std::to_string(tokenIndex) + " is empty");
            }

        }

        std::vector<int> noWhiteSpacesTokens = { TOKEN_CHROM, TOKEN_ALT };

        for (int tokenIndex : noWhiteSpacesTokens) {
            if (tokenIndex >= tokens.size()) {
                throw std::runtime_error("Missing required token at index: " + std::to_string(tokenIndex));
            }

            std::string value = tokens[tokenIndex];
            bool has_whitespace = std::any_of(value.begin(), value.end(), ::isspace);
            if (has_whitespace) {
                throw std::runtime_error("No whitespaces allowed for value at column " + std::to_string(tokenIndex));
            }
        }

    }

    VCFData parse(const std::string& line) const {
        VCFData data;
        // Implement parsing logic here to fill the VCFData structure
        auto tokens = split(line);

        validate(tokens);

        data.chrom = tokens[TOKEN_CHROM];
        data.pos = std::stoi(getTokenValue(tokens[TOKEN_POS]));
        data.ref = getTokenValue(tokens[TOKEN_REF]);
        data.alt = getTokenValue(tokens[TOKEN_ALT]);
        data.filter = getTokenValue(tokens[TOKEN_FILTER]);
        data.qual = std::stof(getTokenValue(tokens[TOKEN_QUAL]));

        setInfo(data, tokens[TOKEN_INFO]);
        setFormat(data, tokens[TOKEN_FORMAT]);

        return data;
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
                            fieldTypes[id] = InfoType::String;
                        }
                        else if (type == "Integer") {
                            fieldTypes[id] = InfoType::Integer;
                        }
                        else if (type == "Float") {
                            fieldTypes[id] = InfoType::Float;
                        }
                        else if (type == "Flag") { // VCF specification uses "Flag" for boolean fields
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

    void setFormat(VCFData& data, std::string& formatToken) const {
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
        auto it = mInfoFieldTypes.find(key);
        if (it != mInfoFieldTypes.end()) {
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
            RaiseWarning(ErrorCodes::UnknownInfoFieldType, "Unknown INFO field type for key: " + key + ". Defaulting to string.");
            // Default to string if type is not defined
            dataMap[key] = value;
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
    std::unordered_map<std::string, InfoType> mInfoFieldTypes;
    std::unordered_map<std::string, InfoType> mFormatFieldTypes;
};

class IVCFDal {
public:

    virtual void storeBatch(const std::vector<VCFData>& data) = 0;

    virtual ~IVCFDal() = default;

};

#include "sqlite3.h"
#include "Configuration.h"

#include <nlohmann/json.hpp>

struct VCFSQLiteRecord {
    std::string chromosome;
    int position;
    std::string ref;
    std::string alt;
    std::string data;


    VCFSQLiteRecord(const VCFData& item) {
        chromosome = item.chrom;
        position = item.pos;
        ref = item.ref;
        alt = item.alt;
        data = itemToData(item);
    }

    std::string itemToData(const VCFData& item) {
        nlohmann::json j;
        j["FILTER"] = item.filter;
        j["QUAL"] = item.qual;

        // Convert info and format maps to JSON or delimited string
        j["INFO"] = convertVCFDataMapToString(item.info);
        j["FORMAT"] = convertVCFDataMapToString(item.format);
        return j.dump();
    }

    std::string convertVCFDataMapToString(const VCFDataMap& dataMap) {
        nlohmann::json j;
        for (const auto& [key, value] : dataMap) {
            std::visit([&](auto&& arg) {
                j[key] = arg;
                }, value);
        }
        return j.dump();
    }


};

class SQLiteVCFDal : public IVCFDal {
public:
    SQLiteVCFDal(const std::string& dbName) {
        // Implement SQLite connection and setup here
        if (sqlite3_open(dbName.c_str(), &db) != SQLITE_OK) {
            RaiseError(ErrorCodes::CouldNotOpenDatabase, "Could not open database: " + dbName);
        }
    }

    // Non-copyable
    SQLiteVCFDal(const SQLiteVCFDal&) = delete;
    SQLiteVCFDal& operator=(const SQLiteVCFDal&) = delete;

    // Movable
    SQLiteVCFDal(SQLiteVCFDal&& other) noexcept : db(other.db) { other.db = nullptr; }
    SQLiteVCFDal& operator=(SQLiteVCFDal&& other) noexcept {
        if (this != &other) {
            if (db) sqlite3_close(db);
            db = other.db;
            other.db = nullptr;
        }
        return *this;
    }

    ~SQLiteVCFDal() {
        if (db) sqlite3_close(db);
    }

public:

    void storeBatch(const std::vector<VCFData>& data) override {
        // Implement batch storage logic here
        std::vector<VCFSQLiteRecord> records;

        for (const auto& item : data) {
            VCFSQLiteRecord record(item);
            records.push_back(std::move(record));
        }
    }

private:

private:
    void exec(const std::string& sql) {
        char* errMsg = nullptr;
        if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::string error = errMsg;
            sqlite3_free(errMsg);
            throw std::runtime_error(error);
        }
    }



private:
    sqlite3* db;
};


class VCFProcessor
{
public:
    VCFProcessor(std::unique_ptr<IFileReader> fileReader, std::unique_ptr<IParser> parser, std::unique_ptr<IVCFDal> dal, int numThreads) : mFileReader(std::move(fileReader)), mParser(std::move(parser)), mDal(std::move(dal)), mParseWorkersThreadPool(numThreads), mStoreWorker(1)  { }

    void process() {
        // Implement VCF processing logic here

        auto batch = mFileReader->getBatch();

        while (!batch.empty()) {
            mParseWorkersThreadPool.enqueue([this, batch] {
                // Process the batch of data
                processBatch(batch);
                });
            batch = mFileReader->getBatch();
        }

    }

    void processBatch(const std::vector<std::string>& batch) {
        std::vector<VCFData> vcfDataBatch;
        vcfDataBatch.reserve(batch.size());

        for (const auto& line : batch) {
            try {

                VCFData data = mParser->parse(line);
                vcfDataBatch.emplace(vcfDataBatch.end(), std::move(data));
            } catch (const std::exception& ex) {
                RaiseWarning(ErrorCodes::ParsingError, "Error parsing line: " + line + ". Error: " + ex.what());
            }
        }

        mStoreWorker.enqueue([this, vcfDataBatch = std::move(vcfDataBatch)]() mutable {
            mDal->storeBatch(vcfDataBatch);
            });
    }

    ~VCFProcessor() = default;

private:
    ThreadPool mParseWorkersThreadPool;
    ThreadPool mStoreWorker;
    std::unique_ptr<IFileReader> mFileReader;
    std::unique_ptr<IParser> mParser;
    std::unique_ptr<IVCFDal> mDal;
};

