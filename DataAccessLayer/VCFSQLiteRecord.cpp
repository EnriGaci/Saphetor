#include "VCFSQLiteRecord.h"

VCFData VCFSQLiteRecord::toVCFData() const {
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

VCFDataMap VCFSQLiteRecord::getDataInfoFromJson(nlohmann::json& info_json) const {
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

VCFDataMap VCFSQLiteRecord::getDataFormatFromJson(nlohmann::json&) const {
    VCFDataMap dataMap;
    return dataMap;
}

VCFSQLiteRecord::VCFSQLiteRecord(const VCFData& item) {
    chromosome = item.chrom;
    position = item.pos;
    ref = item.ref;
    alt = item.alt;
    data = itemToData(item);
}

std::string VCFSQLiteRecord::itemToData(const VCFData& item) {
    nlohmann::json j;
    j["FILTER"] = item.filter;
    j["QUAL"] = item.qual;

    // Convert info and format maps to JSON or delimited string
    j["INFO"] = convertVCFDataMapToString(item.info);
    j["FORMAT"] = convertVCFDataMapToString(item.format);
    return j.dump();
}

std::string VCFSQLiteRecord::convertVCFDataMapToString(const VCFDataMap& dataMap) {
    nlohmann::json j;
    for (const auto& [key, value] : dataMap) {
        std::visit([&](auto&& arg) {
            j[key] = arg;
            }, value);
    }
    return j.dump();
}
