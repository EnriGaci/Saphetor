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

inline std::ostream& operator<<(std::ostream& os, const VCFData& data) {
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

# pragma region TokenValidators

#include <unordered_set>
#include <regex>

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

namespace Validations {

    class Validator {
    public:
        Validator(std::unique_ptr<Validator> validator=nullptr) : mValidator(std::move(validator)) { }
        virtual void validate(const std::string& /*val*/) { return; };
        virtual ~Validator() = default;
    protected:
        std::unique_ptr<Validator> mValidator{ nullptr };
    };

    // Validation decorators
    class HasValue : public Validator {
    public:
        HasValue(std::unique_ptr<Validator> validator) : Validator(std::move(validator)) { }
        void validate(const std::string& token) override {
            if (token.empty()) {
                throw std::runtime_error("Required value is empty");
            }
            if (mValidator) {
                mValidator->validate(token);
            }
        }
    };

    class NoCharAllowed : public Validator {
    public:
        NoCharAllowed(std::unique_ptr<Validator> validator, char c) : Validator(std::move(validator)) , charToValidate(c) { }
        void validate(const std::string& token) override {
            if (token.find(charToValidate) != std::string::npos) {
                throw std::runtime_error(std::string("Invalid char") + charToValidate + " found");
            }
            if (mValidator) {
                mValidator->validate(token);
            }
        }

        virtual ~NoCharAllowed() = default;
    private:
        char charToValidate;

    };

    class NoWhiteSpace : public NoCharAllowed {
    public:
        NoWhiteSpace(std::unique_ptr<Validator> validator) : NoCharAllowed(std::move(validator), ' ') { }
    };

    class NoSemicolon : public NoCharAllowed {
    public:
        NoSemicolon(std::unique_ptr<Validator> validator) : NoCharAllowed(std::move(validator), ';') { }
    };

    class MatchRegex : public Validator {
    public:
        MatchRegex(std::unique_ptr<Validator> validator, const std::regex& rgx ) : Validator(std::move(validator)) , mRgx(rgx) { }

        void validate(const std::string& token) override {
            std::smatch match;

            if (!std::regex_search(token, match, mRgx)) {
                throw std::runtime_error("Invalid token regex format");
            }
            if (mValidator) {
                mValidator->validate(token);
            }
        }
    private:
        std::regex mRgx;
    };


    class NoDuplicates : public Validator {
    public:
        NoDuplicates(std::unique_ptr<Validator> validator) : Validator(std::move(validator)) { }
        void validate(const std::string& token) override {
            if (seen.count(token)) {
                throw std::runtime_error("Duplicate value found: " + token);
            }
            seen.insert(token);
            if (mValidator) {
                mValidator->validate(token);
            }
        };
    private:
        std::unordered_set<std::string> seen;
    };


    class Required : public Validator {
    public:
        Required(std::unique_ptr<Validator> validator) : Validator(std::move(validator)) { }
        void validate(const std::string& token) override {
            if (token == ".") {
                throw std::runtime_error("Required value is missing");
            }
            if (mValidator) {
                mValidator->validate(token);
            }
        }
    };

    class IsInteger : public Validator {
    public:
        IsInteger(std::unique_ptr<Validator> validator) : Validator(std::move(validator)) { }
        void validate(const std::string& token) override {
            try {
                (void)std::stoi(token);
            }
            catch (const std::exception&) {
                throw std::runtime_error("Value is not a valid integer");
            }
            if (mValidator) {
                mValidator->validate(token);
            }
        }
    };

    inline std::vector<int> idsForValidation = { TOKEN_CHROM, TOKEN_POS, TOKEN_REF, TOKEN_ALT, TOKEN_QUAL, TOKEN_FILTER };

    using ValidationsType = std::unordered_map<int, std::shared_ptr<Validator>>;

    inline ValidationsType DefaultValidations = {
        {
            TOKEN_CHROM, 
            std::make_unique<Required>(
                std::make_unique<NoWhiteSpace>(
                    std::make_unique<HasValue>(nullptr)
                )
            )
        },
        {
            TOKEN_POS, 
            std::make_unique<Required>(
                std::make_unique<IsInteger>(nullptr)
            )
        },
        {
            TOKEN_ID,
            std::make_unique<NoDuplicates>(
                std::make_unique<NoWhiteSpace>(
                    std::make_unique<NoSemicolon>(nullptr)
                )
            )
        },
        {
            TOKEN_REF,
            std::make_unique<Required>(
                std::make_unique<MatchRegex>(nullptr,std::regex("^[ACGTN]+$", std::regex::icase)
                )
            )
        },
    };

} // end namespace



#pragma endregion


class IParser {
public:

    virtual VCFData parse(const std::string& line) const = 0;

    virtual ~IParser() = default;
};

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
            RaiseWarning("Unexpected number of tokens in line: " + std::to_string(tokens.size()));
        }

        for (int identifier : Validations::idsForValidation) {
            std::string token = tokens[identifier];
            mValidators.at(identifier)->validate(token);
        }
    }

    VCFData parse(const std::string& line) const {
        VCFData data;
        // Implement parsing logic here to fill the VCFData structure
        auto tokens = split(line);

        validate(tokens);

        data.chrom = getTokenValue(tokens[TOKEN_CHROM]);
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

    std::string getTokenValue(std::string& token) const {
        return token == "." ? "" : token;
    }

private:
    std::unordered_map<std::string, InfoType> mInfoFieldTypes;
    std::unordered_map<std::string, InfoType> mFormatFieldTypes;
    Validations::ValidationsType mValidators;
};

class IVCFDal {
public:

    virtual void initDb(bool clearDb = false) = 0;
    virtual void storeBatchInStaging(const std::vector<VCFData>& data) = 0;
    virtual void finalizeRecords() = 0;
    virtual std::vector<VCFData> fetchAllVariants() = 0;

    virtual ~IVCFDal() = default;

};

#include "sqlite3.h"
#include "Configuration.h"

#include <nlohmann/json.hpp>

struct VCFSQLiteRecord {
    std::string chromosome{""};
    int position{0};
    std::string ref{""};
    std::string alt{""};
    std::string data{""};

    VCFSQLiteRecord() = default;

    VCFData toVCFData() const {
        VCFData vcfData;
        vcfData.chrom = chromosome;
        vcfData.pos = position;
        vcfData.ref = ref;
        vcfData.alt = alt;

        auto jsonData = nlohmann::json::parse(this->data);
        vcfData.filter = jsonData["FILTER"].get<std::string>();
        vcfData.qual = jsonData["QUAL"].get<float>();

        // Parse INFO and FORMAT back to VCFDataMap
        vcfData.info = getDataInfoFromJson(jsonData);
        vcfData.format = getDataFormatFromJson(jsonData);

        return vcfData;
    }

    VCFDataMap getDataInfoFromJson(nlohmann::json& info_json) const {
        VCFDataMap dataMap;
        std::string infoStr = info_json["INFO"].get<std::string>();
        nlohmann::json infoObj = nlohmann::json::parse(infoStr);

        for (auto& [key, value] : infoObj.items()) {
            if (value.is_string()) {
                dataMap[key] = value.get<std::string>();
            }
            else if (value.is_number_integer()) {
                dataMap[key] = value.get<int>();
            }
            else if (value.is_number_float()) {
                dataMap[key] = value.get<float>();
            }
            else if (value.is_boolean()) {
                dataMap[key] = value.get<bool>();
            }
        }
        return dataMap;
    }

    VCFDataMap getDataFormatFromJson(nlohmann::json& /*info_json*/) const {
        VCFDataMap dataMap;
        return dataMap;
    }

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


class SQLiteConnectionPool {
public:
    SQLiteConnectionPool(const std::string& dbName, size_t poolSize)
        : dbName_(dbName)
    {
        for (size_t i = 0; i < poolSize; ++i) {
            sqlite3* db = nullptr;
            if (sqlite3_open(dbName.c_str(), &db) != SQLITE_OK) {
                throw std::runtime_error("Failed to open SQLite database: " + dbName);
            }
            pool_.push(db);
        }
    }

    ~SQLiteConnectionPool() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!pool_.empty()) {
            sqlite3* db = pool_.front();
            pool_.pop();
            if (db) sqlite3_close(db);
        }
    }

    // Acquire a connection from the pool (blocks if none available)
    sqlite3* acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return !pool_.empty(); });
        sqlite3* db = pool_.front();
        pool_.pop();
        return db;
    }

    // Return a connection to the pool
    void release(sqlite3* db) {
        std::unique_lock<std::mutex> lock(mutex_);
        pool_.push(db);
        cond_.notify_one();
    }

    // Not copyable or movable
    SQLiteConnectionPool(const SQLiteConnectionPool&) = delete;
    SQLiteConnectionPool& operator=(const SQLiteConnectionPool&) = delete;

private:
    std::string dbName_;
    std::queue<sqlite3*> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

class SQLiteQuery {
public:
    SQLiteQuery(SQLiteConnectionPool& pool) : mConnectionPool(pool) {
        mDb = pool.acquire();
    }

    void exec(const std::string& sql) {
        char* errMsg = nullptr;
        auto status = sqlite3_exec(mDb, sql.c_str(), nullptr, nullptr, &errMsg);
        if (status != SQLITE_OK) {
            std::string error = errMsg;
            sqlite3_free(errMsg);
            throw std::runtime_error(error);
        }
    }

    ~SQLiteQuery()
    {
        if (mDb) {
            mConnectionPool.release(mDb);
        }
    }

protected:
    sqlite3* mDb{ nullptr };
    SQLiteConnectionPool& mConnectionPool;
};

class SQLiteStmt : public SQLiteQuery {
public:

    SQLiteStmt(SQLiteConnectionPool& pool, sqlite3_stmt* stmt) : SQLiteQuery(pool) {

        if (!stmt) {
            throw std::runtime_error("Invalid statement");
        }

        exec("BEGIN TRANSACTION");
        mStmt = stmt;
    }

    void bind_text(int pos, const char* value) {
        sqlite3_bind_text(mStmt, pos, value, -1, SQLITE_STATIC);
    }

    void bind_int(int pos, int value) {
        sqlite3_bind_int(mStmt, pos, value);
    }

    void finalizeBindings() {
        sqlite3_step(mStmt);   // Execute
        sqlite3_reset(mStmt);  // Reset for the next set of values
        sqlite3_clear_bindings(mStmt);
    }

    ~SQLiteStmt() {
        // 4. Commit the Transaction flushing all rows to disk at once
        exec("COMMIT;");
    }
private:
    sqlite3_stmt* mStmt;
};

class SQLiteSelect {
public:
    SQLiteSelect(sqlite3_stmt* stmt) : mStmt(stmt){ }

    int step() {
        return sqlite3_step(mStmt);
    }

    std::string column_text(int index) {
        const char* text = reinterpret_cast<const char*>(sqlite3_column_text(mStmt, index));
        return text;
    }

    int column_int(int index) {
        return sqlite3_column_int(mStmt, index);
    }

    ~SQLiteSelect() = default;
private:
    sqlite3_stmt* mStmt{ nullptr };
};

class SQLiteVCFDal : public IVCFDal {
public:
    SQLiteVCFDal(const std::string& dbName) : mConnectionPool(dbName, 10) {
        prepareStatments();
    }

    // Non-copyable
    SQLiteVCFDal(const SQLiteVCFDal&) = delete;
    SQLiteVCFDal& operator=(const SQLiteVCFDal&) = delete;
    SQLiteVCFDal(SQLiteVCFDal&& other) = delete;
    SQLiteVCFDal& operator=(SQLiteVCFDal && other) = delete;

    ~SQLiteVCFDal() {
        if (mFetchAllVariantsStmt) finalize_stmt(mFetchAllVariantsStmt);
        if (mInsertIntoStagingVCFRecordsStmt) finalize_stmt(mInsertIntoStagingVCFRecordsStmt);
    }

public:

    void initDb(bool clearDb = false) override {
        // Implement database initialization logic here, e.g., create tables if they don't exist
        SQLiteQuery query(mConnectionPool);

        // Performance Tuning
        query.exec("PRAGMA journal_mode = WAL;"); // Better concurrency
        query.exec("PRAGMA synchronous = NORMAL;"); // Faster writes

        if (clearDb) {
            const std::string droptables = R"(
                DROP TABLE IF EXISTS variants;
                DROP TABLE IF EXISTS staging_variants;
            )";
            query.exec(droptables);
        }

        const std::string createVariantsTable = R"(
            CREATE TABLE IF NOT EXISTS variants (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                chromosome TEXT,
                position INTEGER,
                ref TEXT,
                alt TEXT,
                data TEXT
            );

            -- Nnon-unique index to keep sorting fast
            CREATE INDEX IF NOT EXISTS idx_coords ON variants (chromosome, position);

        )";
        query.exec(createVariantsTable);

        const std::string constraints = R"(
            ALTER TABLE variants 
            ADD CONSTRAINT valid_json 
            CHECK (json_valid(data));
        )";

        try {
            query.exec(constraints);
        }
        catch (const std::exception& ex) {
            // Ignore error if constraint already exists
            if (std::string(ex.what()).find("already exists") == std::string::npos) {
                throw; // ignore if it's the already exists
            }
        }

        const std::string createVCFTempTable = R"(
            CREATE TABLE IF NOT EXISTS staging_variants (
                chromosome TEXT,
                position INTEGER,
                ref TEXT,
                alt TEXT,
                data TEXT
            );
        )";
        query.exec(createVCFTempTable);

    }

    void prepareStatments() {
        // prepare statements 
        const char* sqlInsertStagingVCFRecord = "INSERT INTO staging_variants VALUES (?, ?, ?, ?, ?);";
        prepare_stmt(sqlInsertStagingVCFRecord, &mInsertIntoStagingVCFRecordsStmt);

        const char* sqlFetchAllVariantsStmt= R"(
            SELECT CHROMOSOME, POSITION, REF, ALT, DATA
            FROM variants
            ORDER BY CHROMOSOME, POSITION; 
        )";

        prepare_stmt(sqlFetchAllVariantsStmt, &mFetchAllVariantsStmt);
    }

    [[nodiscard]]
    std::vector<VCFData> fetchAllVariants() override {
        std::vector<VCFData> result;

        SQLiteSelect select(mFetchAllVariantsStmt);

        while (select.step() == SQLITE_ROW) {
            VCFSQLiteRecord record;

            record.chromosome = select.column_text(0);
            record.position = select.column_int(1);
            record.ref = select.column_text(2);
            record.alt = select.column_text(3);
            record.data = select.column_text(4);

            result.emplace_back(record.toVCFData());
        }

        return result;
    }

    /*
    * Store batch in a staging table for quick insertion
    */
    void storeBatchInStaging(const std::vector<VCFData>& data) override {
        // Implement batch storage logic here
        std::vector<VCFSQLiteRecord> records;

        for (const auto& item : data) {
            VCFSQLiteRecord record(item);
            records.push_back(std::move(record));
        }

        storeInVCFStaging(records);
    }

    /*
    * Finalize records by moving them from staging to main table in sorted order
    */
    void finalizeRecords() override {

        SQLiteQuery query(mConnectionPool); 

        const char* sql = R"(
        INSERT INTO variants (chromosome, position, ref, alt, data)
            SELECT CHROMOSOME, POSITION, REF, ALT, DATA
            FROM staging_variants
            ORDER BY CHROMOSOME, POSITION ;

        DELETE FROM staging_variants;
        )";

        query.exec(sql);
    }

private:

    void storeInVCFStaging(const std::vector<VCFSQLiteRecord>& records) {
        SQLiteStmt stmt(mConnectionPool, mInsertIntoStagingVCFRecordsStmt);

        for (auto record : records) {
            stmt.bind_text(1, record.chromosome.c_str());
            stmt.bind_int(2, record.position);
            stmt.bind_text(3, record.ref.c_str());
            stmt.bind_text(4, record.alt.c_str());
            stmt.bind_text(5, record.data.c_str());

            stmt.finalizeBindings();
        }
    }

private:

    void prepare_stmt(const char* sql, sqlite3_stmt** stmt) {
        sqlite3* db = mConnectionPool.acquire();

        char* errMsg = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, stmt, NULL) != SQLITE_OK) {
            std::string error = errMsg;
            sqlite3_free(errMsg);
            throw std::runtime_error(error);
        }

        mConnectionPool.release(db);
    }

    void finalize_stmt(sqlite3_stmt* stmt) {
        sqlite3* db = mConnectionPool.acquire();

        char* errMsg = nullptr;
        if (sqlite3_finalize(stmt) != SQLITE_OK) {
            std::string error = errMsg;
            sqlite3_free(errMsg);
            throw std::runtime_error(error);
        }

        mConnectionPool.release(db);
    }

private:
    sqlite3_stmt* mInsertIntoStagingVCFRecordsStmt{ nullptr };
    sqlite3_stmt* mFetchAllVariantsStmt{ nullptr };
    SQLiteConnectionPool mConnectionPool;
};


class VCFProcessor
{
public:
    VCFProcessor(
        std::unique_ptr<IFileReader> fileReader,
        std::unique_ptr<IParser> parser,
        std::unique_ptr<IVCFDal> dal,
        int numThreads) : 
            mParseWorkersThreadPool(numThreads),
            mFileReader(std::move(fileReader)),
            mParser(std::move(parser)),
            mDal(std::move(dal))
        { }

    /*
    * Single threaded read file in batches
    * Process each batch in a worker thread
    * Once all workers are done, finalize the records
    */
    void process() {
        auto batch = mFileReader->getBatch();

        while (!batch.empty()) {
            mParseWorkersThreadPool.enqueue([this, batch] {
                // Process the batch of data
                processBatch(batch);
                });
            batch = mFileReader->getBatch();
        }

        // Wait for all parsing and persistance workers to finish before finalizing records
        mParseWorkersThreadPool.waitForTasksToFinish();
        mStoreWorker.waitForTasksToFinish();

        mDal->finalizeRecords();
    }

    void processBatch(const std::vector<std::string>& batch) {
        std::vector<VCFData> vcfDataBatch;
        vcfDataBatch.reserve(batch.size());

        for (const auto& line : batch) {
            try {
                VCFData data = mParser->parse(line);
                vcfDataBatch.emplace(vcfDataBatch.end(), std::move(data));
            } catch (const std::exception& /*ex*/) {
                //RaiseWarning("Error parsing line: " + line + ". Error: " + ex.what());
            }
        }

        mStoreWorker.enqueue([this, vcfDataBatch = std::move(vcfDataBatch)]() mutable {
            mDal->storeBatchInStaging(vcfDataBatch);
            });
    }

    ~VCFProcessor() = default;

private:
    ThreadPool mParseWorkersThreadPool;
    ThreadPool mStoreWorker{ 1 }; // used to avoid parallel insertion in the storage
    std::unique_ptr<IFileReader> mFileReader;
    std::unique_ptr<IParser> mParser;
    std::unique_ptr<IVCFDal> mDal;
};

