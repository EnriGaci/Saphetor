#include "VCFProcessor.h"
#include "Utils.h"

#include "gtest/gtest.h"
#include <string>

TEST(TestVCFReader, TestInfoRowsAreSkipped) {
    VCFReader reader(TEST_VCF_FILE_PATH, 1);

    auto line = reader.getBatch();

    ASSERT_TRUE(line.size() > 0);
    ASSERT_TRUE(line[0].size() > 0);

    std::string expectedFirstData = "chr3";
    ASSERT_TRUE(line[0].substr(0, expectedFirstData.size()) == expectedFirstData);
}

TEST(TestVCFReader, TestBatchOrLessLinesAreRead) {
    VCFReader reader(TEST_VCF_FILE_PATH, 100);

    std::vector<std::string> batch = reader.getBatch();

    ASSERT_EQ(batch.size(), 23); // 23 data lines in sample file
    ASSERT_TRUE(batch[0] == "chr3	109754177	.	T	C	937.77	PASS	AC=2;AF=1;AN=2;DP=31;ExcessHet=3.0103;FS=0;MLEAC=2;MLEAF=1;MQ=60;QD=30.25;SOR=0.756	GT:AB:AD:DP:GQ:PL:SAC	1/1:1:0,31:31:93:966,93,0:0,0,16,15");
}
