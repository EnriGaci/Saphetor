#include "VCFProcessor.h"
#include "Utils.h"
#include "VCFLineParser.h"

#include "gtest/gtest.h"


TEST(TestVCFLineParser, TestParseOk) {
    VCFLineParser parser(TEST_VCF_FILE_PATH);

    std::string line = "chr3	109754177	.	T	C	937.77	PASS	AC=2;AF=1;AN=2;DP=31;ExcessHet=3.0103;FS=0;MLEAC=2;MLEAF=1;MQ=60;QD=30.25;SOR=0.756	GT:AB:AD:DP:GQ:PL:SAC	1/1:1:0,31:31:93:966,93,0:0,0,16,15";

    VCFData data = parser.parse(line);

    ASSERT_EQ(data.chrom, "chr3");
    ASSERT_EQ(data.pos, 109754177);
    ASSERT_EQ(data.ref, "T");
    ASSERT_EQ(data.alt, "C");

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

TEST(TestVCFLineParser, TestParseWithCustomValidations) {
    // Custom: Only check that CHROM and POS are required, others are ignored
    Validations::ValidationsType customValidations = {
        {TOKEN_CHROM, std::make_unique<Validations::Required>(nullptr)},
        {TOKEN_POS, std::make_unique<Validations::IsInteger>(nullptr)}
    };
    VCFLineParser parser(TEST_VCF_FILE_PATH, customValidations);

    std::string line = "chr1\t12345\t.\tA\tT\t99.9\tPASS\tINFO\tFORMAT";
    EXPECT_NO_THROW(parser.parse(line));

    // Should throw if CHROM is missing
    std::string badLine = "\t\t12345\t.\tA\tT\t99.9\tPASS\tINFO\tFORMAT";
    EXPECT_ANY_THROW(parser.parse(badLine));

    // Should throw if POS is not integer
    std::string badLine2 = "chr1\tabc\t.\tA\tT\t99.9\tPASS\tINFO\tFORMAT";
    EXPECT_ANY_THROW(parser.parse(badLine2));
}

