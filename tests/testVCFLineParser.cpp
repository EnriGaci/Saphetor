#include "VCFProcessor.h"
#include "Utils.h"

#include "gtest/gtest.h"


TEST(TestVCFLineParser, TestParseOk) {
    VCFLineParser parser(TEST_VCF_FILE_PATH);

    std::string line = "chr3	109754177	.	T	C	937.77	PASS	AC=2;AF=1;AN=2;DP=31;ExcessHet=3.0103;FS=0;MLEAC=2;MLEAF=1;MQ=60;QD=30.25;SOR=0.756	GT:AB:AD:DP:GQ:PL:SAC	1/1:1:0,31:31:93:966,93,0:0,0,16,15";

    VCFData data = parser.parse(line);

    ASSERT_EQ(data.chrom, "chr3");
    ASSERT_EQ(data.pos, 109754177);
    ASSERT_EQ(data.ref, "T");
    ASSERT_EQ(data.alt, "C");

    
    //ASSERT_EQ(static_cast<int>(data.info["AC"]), 2);
    //ASSERT_EQ(data.info["AF"], 1.0);

}
