#pragma once
#include "DALExport.h"
#include "VCFData.h"

#include <string>
#include <nlohmann/json.hpp>

struct DAL_API VCFSQLiteRecord {
    std::string chromosome{""};
    int position{0};
    std::string ref{""};
    std::string alt{""};
    std::string data{""};

    VCFSQLiteRecord() = default;

    /*
    * @brief Converts the VCFSQLiteRecord to a VCFData object by parsing the JSON data field.
    *
     * @return A VCFData object containing the parsed information from the VCFSQLiteRecord.
     * @throws std::runtime_error if the JSON parsing fails or required fields are missing.
    */
    VCFData toVCFData() const;


    /*
    * @brief Parses the INFO field from the JSON data and returns a VCFDataMap containing the key-value pairs.
    * 
    * For example, if the INFO field in the JSON data is:
    * {
    *   "DP": 100,
    *   "AF": 0.5
    * }
     * The returned VCFDataMap will contain the key-value pairs: {"DP": "100", "AF": "0.5"}.
    */
    VCFDataMap getDataInfoFromJson(nlohmann::json& info_json) const;

    /*
    * @brief Parses the FORMAT field from the JSON data and returns a VCFDataMap containing the key-value pairs.
     * The FORMAT field is expected to be a JSON object where each key is a sample name and the value is another JSON object with key-value pairs for that sample.
     * For example:
     * {
     *   "sample1": {"GT": "0/1", "DP": 10},
     *   "sample2": {"GT": "1/1", "DP": 20}
     * }
    */
    VCFDataMap getDataFormatFromJson(nlohmann::json& /*info_json*/) const;

    /*
    * @brief Constructs a VCFSQLiteRecord from a given VCFData object by converting the INFO and FORMAT fields to JSON string.
    *
    * For example if the VCFData object has the following fields:
    * chrom: "chr1"
    * pos: 12345
    * ref: "A"
    * alt: "T"
    * filter: "PASS"
    * qual: 99.0
    * info: {"DP": "100", "AF": "0.5"}
    * format: {"GT": "0/1", "DP": 10}}
     * The resulting VCFSQLiteRecord will have:
     * chromosome: "chr1"
     * position: 12345
     * ref: "A"
     * alt: "T"
     * data: A JSON string representation of the filter, qual, info, and format fields.
    * 
    */
    VCFSQLiteRecord(const VCFData& item);

    /*
    * @brief Converts a VCFData object to a JSON string representation that can be stored in the data field of the VCFSQLiteRecord.
    */
    std::string itemToData(const VCFData& item);

    /*
    * @brief Converts a VCFDataMap to a JSON string representation that can be stored in the data field of the VCFSQLiteRecord.
     * For example, if the VCFDataMap contains the key-value pairs: {"DP": "100", "AF": "0.5"}, the resulting JSON string will be:
     * {
     *   "DP": "100",
     *   "AF": "0.5"
     * }
     * This method can be used to convert both INFO and FORMAT maps to JSON strings for storage.
    */
    std::string convertVCFDataMapToString(const VCFDataMap& dataMap);

};
