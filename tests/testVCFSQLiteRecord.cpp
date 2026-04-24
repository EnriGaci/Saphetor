#include "VCFProcessor.h"
#include "Utils.h"

#include "gtest/gtest.h"

VCFData getTestVCFData() {
    VCFData data;
    data.chrom = "chr3";
    data.pos = 109754177;
    data.ref = "T";
    data.alt = "C";
    data.info["AC"] = 2;
    data.info["AF"] = 1.0f;
    data.info["AN"] = 2;
    data.info["DP"] = 31;
    data.info["ExcessHet"] = 3.0103f;
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
    ASSERT_EQ(record.data, "{\"FILTER\":\"\",\"FORMAT\":\"null\",\"INFO\":\"{\\\"AC\\\":2,\\\"AF\\\":1.0,\\\"AN\\\":2,\\\"DP\\\":31,\\\"ExcessHet\\\":3.0102999210357666,\\\"FS\\\":0.0,\\\"MLEAC\\\":2,\\\"MLEAF\\\":1.0,\\\"MQ\\\":60.0,\\\"QD\\\":30.25,\\\"SOR\\\":0.7559999823570251}\",\"QUAL\":-107374176.0}");
}