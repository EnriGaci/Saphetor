#include "VCFProcessor.h"
#include "Utils.h"
#include "VCFSQLiteRecord.h"
#include "SQLiteVCFDal.h"

#include "gtest/gtest.h"
#include <random>

VCFData getRandomVCFData() {
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<int> chromDist(1, 22);
    static std::uniform_int_distribution<int> posDist(100000, 200000000);
    static std::uniform_int_distribution<int> baseDist(0, 3);
    static std::uniform_int_distribution<int> intDist(0, 100);
    static std::uniform_real_distribution<float> floatDist(0.0f, 100.0f);

    const char* bases = "ACGT";

    VCFData data;
    data.chrom = "chr" + std::to_string(chromDist(rng));
    data.pos = posDist(rng);
    data.ref = std::string(1, bases[baseDist(rng)]);
    data.alt = std::string(1, bases[baseDist(rng)]);
    data.filter = "";
    data.qual = floatDist(rng);

    data.info["AC"] = intDist(rng);
    data.info["AF"] = floatDist(rng);
    data.info["AN"] = intDist(rng);
    data.info["DP"] = intDist(rng);
    data.info["ExcessHet"] = floatDist(rng);
    data.info["FS"] = floatDist(rng);
    data.info["MLEAC"] = intDist(rng);
    data.info["MLEAF"] = floatDist(rng);
    data.info["MQ"] = floatDist(rng);
    data.info["QD"] = floatDist(rng);
    data.info["SOR"] = floatDist(rng);

    return data;
}

VCFData getTestVCFData() {
    VCFData data;
    data.chrom = "chr3";
    data.pos = 109754177;
    data.ref = "T";
    data.alt = "C";
    data.qual = 937.77f;
    data.info["AC"] = 2;
    data.info["AF"] = 1.0f;
    data.info["AN"] = 2;
    data.info["DP"] = 31;
    data.info["ExcessHet"] = 3.0103f;
    data.info["FS"] = 0.0f;
    data.info["FS"] = 0.0f;
    data.info["MLEAC"] = 2;
    data.info["MLEAF"] = 1.0f;
    data.info["MQ"] = 60.0f;
    data.info["QD"] = 30.25f;
    data.info["SOR"] = 0.756f;

    return data;
}

TEST(TestVCFSQLiteRecord, TestSQLiteRecordCreation) {
    VCFData data = getTestVCFData();

    VCFSQLiteRecord record(data);

    ASSERT_EQ(record.chromosome, "chr3");
    ASSERT_EQ(record.position, 109754177);
    ASSERT_EQ(record.ref, "T");
    ASSERT_EQ(record.alt, "C");

    auto jsonData = nlohmann::json::parse(record.data);
    auto infoMap =record.getDataInfoFromJson(jsonData);

    ASSERT_NEAR( jsonData["QUAL"].get<float>(), 937.77f, 1e-5);
    ASSERT_EQ(std::get<int>(infoMap["AC"]), 2);
    ASSERT_NEAR(std::get<float>(infoMap["AF"]), 1.0f, 1e-5);
    ASSERT_EQ(std::get<int>(infoMap["AN"]), 2);
    ASSERT_EQ(std::get<int>(infoMap["DP"]), 31);
    ASSERT_NEAR(std::get<float>(infoMap["ExcessHet"]), 3.0103f, 1e-5);
    ASSERT_NEAR(std::get<float>(infoMap["FS"]), 0.0f, 1e-5);
    ASSERT_EQ(std::get<int>(infoMap["MLEAC"]), 2);
    ASSERT_NEAR(std::get<float>(infoMap["MLEAF"]), 1.0f, 1e-5);
    ASSERT_NEAR(std::get<float>(infoMap["MQ"]), 60.0f, 1e-5);
    ASSERT_NEAR(std::get<float>(infoMap["QD"]), 30.25f, 1e-5);
    ASSERT_NEAR(std::get<float>(infoMap["SOR"]), 0.756f, 1e-5);
}

TEST(TestSQLiteVCFDal, TestDbReadDataOk) {
    SQLiteVCFDal dal("test_vcf.db", true);

    std::vector<VCFData> dataList;
    dataList.push_back(getTestVCFData());

    dal.storeBatchInStaging(dataList);
    dal.finalizeRecords();

    auto readRecords = dal.fetchAllVariants();

    ASSERT_EQ(readRecords.size(), 1);

    auto data = readRecords[0];
    ASSERT_TRUE(data.chrom == "chr3");
    ASSERT_TRUE(data.pos == 109754177);
    ASSERT_TRUE(data.ref == "T");
    ASSERT_TRUE(data.alt == "C");
    ASSERT_EQ(std::get<int>(data.info["AC"]), 2);
    ASSERT_EQ(std::get<float>(data.info["AF"]), 1.0);
    ASSERT_EQ(std::get<int>(data.info["AN"]), 2);
    ASSERT_EQ(std::get<int>(data.info["DP"]), 31);
    ASSERT_NEAR(std::get<float>(data.info["ExcessHet"]), 3.0103, 1e-5);
    ASSERT_EQ(std::get<float>(data.info["FS"]), 0.0);
    ASSERT_EQ(std::get<int>(data.info["MLEAC"]), 2);
    ASSERT_EQ(std::get<float>(data.info["MLEAF"]), 1.0);
    ASSERT_EQ(std::get<float>(data.info["MQ"]), 60.0);
    ASSERT_EQ(std::get<float>(data.info["QD"]), 30.25);
    ASSERT_NEAR(std::get<float>(data.info["SOR"]), 0.756, 1e-5);
}

/*
* Insert records in batches
* Call finalize
* Read persisted records
* Verify the order 
*/
TEST(TestSQLiteVCFDal, TestDbCreateInBatchesOK) {
    SQLiteVCFDal dal("test_vcf.db", true);

    std::vector<VCFData> dataList;
    size_t numRecords = 100; // 1 million records take 10 mins

    for (size_t i = 0; i < numRecords; ++i) {
        dataList.push_back(getRandomVCFData());
    }

    dal.storeBatchInStaging(dataList);

    // Since we haven't finalized yet, there should be 
    // no records in the main table
    auto commitedRecords = dal.fetchAllVariants();
    ASSERT_EQ(commitedRecords.size(), 0);

    dataList.clear();


    for (size_t i = 0; i < numRecords; ++i) {
        dataList.push_back(getRandomVCFData());
    }
    dal.storeBatchInStaging(dataList);

    dal.finalizeRecords();

    auto readRecords = dal.fetchAllVariants();

    ASSERT_EQ(readRecords.size(), 2 * numRecords);


    VCFData current_record = readRecords[0];
    // assert the order
    for (size_t i = 1; i < readRecords.size(); ++i) {
        ASSERT_TRUE(current_record.chrom <= readRecords[i].chrom);

        if (current_record.chrom == readRecords[i].chrom) {
            ASSERT_TRUE(current_record.pos <= readRecords[i].pos);
        }
        current_record = readRecords[i];
    }
}