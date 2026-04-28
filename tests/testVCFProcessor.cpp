#include "VCFProcessor.h"
#include "Utils.h"
#include "VCFReader.h"
#include "SQLiteVCFDal.h"
#include "VCFLineParser.h"

#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <string>


// Minimal mocks
class MockFileReader : public IFileReader {
public:
    std::vector<std::vector<std::string>> batches;
    size_t call = 0;
    std::vector<std::string> getBatch() override {
        if (call < batches.size()) return batches[call++];
        return {};
    }
};

class MockParser : public IParser {
public:
    VCFData parse(const std::string& line) const override {
        VCFData d;
        d.chrom = line; // just echo for test
        return d;
    }
};


class MockDal : public IVCFDal {
public:
    int finalizeCount;
    std::vector<std::vector<VCFData>> stored;
    void storeBatchInStaging(const std::vector<VCFData>& batch) override {
        stored.push_back(batch);
    }
    void finalizeRecords() override { finalizeCount++; }

    std::vector<VCFData> fetchAllVariants() override { return std::vector<VCFData>(); }

    void clearData() {
        stored.clear();
    }
};

TEST(TestVCFProcessor, ProcessCallsStoreAndFinalize) {
    auto reader = std::make_unique<MockFileReader>();
    reader->batches = {{"chr1", "chr2"}, {"chr3"}};

    auto parser = std::make_unique<MockParser>();
    auto dal = std::make_unique<MockDal>();
    int numThreads = 2;
    std::vector<std::vector<VCFData>>& stored = dal->stored;
    int* finalizeCount = &dal->finalizeCount;

    VCFProcessor proc(std::move(reader), std::move(parser), std::move(dal), numThreads);
    proc.process();

    // Flatten all stored records
    std::vector<VCFData> all;
    for (auto& batch : stored) {
        for (auto& item : batch) all.push_back(item);
    }

    // Total records correct
    ASSERT_EQ(all.size(), 3);

    // All expected chroms present, regardless of order
    std::vector<std::string> chroms;
    for (auto& item : all) chroms.push_back(item.chrom);
    EXPECT_NE(std::find(chroms.begin(), chroms.end(), "chr1"), chroms.end());
    EXPECT_NE(std::find(chroms.begin(), chroms.end(), "chr2"), chroms.end());
    EXPECT_NE(std::find(chroms.begin(), chroms.end(), "chr3"), chroms.end());

    EXPECT_EQ(*finalizeCount, 1);
}



// Delay parser to simulate slow parsing
class DelayParser : public IParser {
public:
    int delayMs;
    DelayParser(int ms) : delayMs(ms) {}
    VCFData parse(const std::string& line) const override {
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        VCFData d;
        d.chrom = line;
        return d;
    }
};

// DAL that checks for concurrent store calls and finalize timing
class SerializingDal : public IVCFDal {
public:
    static std::atomic<int> concurrentCalls;
    static std::atomic<int> maxConcurrent;
    static std::atomic<int> finalizeCalled;
    static std::atomic<bool> storeFinished;
    void storeBatchInStaging(const std::vector<VCFData>& /*batch*/) override {
        int now = ++concurrentCalls;
        if (now > maxConcurrent) maxConcurrent = now;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        --concurrentCalls;
        storeFinished = true;
    }
    void finalizeRecords() override {
        finalizeCalled++;
        EXPECT_TRUE(storeFinished);
    }
    std::vector<VCFData> fetchAllVariants() override { return {}; }
};

std::atomic<int> SerializingDal::concurrentCalls{ 0 };
std::atomic<int> SerializingDal::maxConcurrent{ 0 };
std::atomic<int> SerializingDal::finalizeCalled{ 0 };
std::atomic<bool> SerializingDal::storeFinished{ false };

TEST(TestVCFProcessor, ConcurrencyAndSerialization) {
    auto reader = std::make_unique<MockFileReader>();

    int numThreads = 10;
    for (int i = 1; i <= numThreads; i++) {
        reader->batches.push_back({ "chr" + std::to_string(i) });
    }

    int parseDelayMs = 100;
    auto parser = std::make_unique<DelayParser>(parseDelayMs);
    auto dal = std::make_unique<SerializingDal>();

    SerializingDal::concurrentCalls = 0;
    SerializingDal::maxConcurrent = 0;
    SerializingDal::finalizeCalled = 0;
    SerializingDal::storeFinished = false;

    auto start = std::chrono::steady_clock::now();
    VCFProcessor proc(std::move(reader), std::move(parser), std::move(dal), numThreads);
    proc.process();
    auto end = std::chrono::steady_clock::now();

    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Parsing should be parallel: elapsed time should be less than sum of all parse delays
    EXPECT_LT(elapsedMs, parseDelayMs*numThreads ); 
    EXPECT_GT(elapsedMs, parseDelayMs - 10); // should not be instant

    // Storage should be serialized: maxConcurrent should be 1
    EXPECT_EQ(SerializingDal::maxConcurrent, 1);
    // Finalize called once
    EXPECT_EQ(SerializingDal::finalizeCalled, 1);
}



TEST(TestVCFProcessor, TestActualFileProcessing) {
    //Setup
    std::string db_name = "test_vcf.db";
    size_t expectedRecords = 22;

    std::unique_ptr<IFileReader> reader = std::make_unique<VCFReader>(TEST_VCF_FILE_PATH, 100);
    std::unique_ptr<IParser> parser = std::make_unique<VCFLineParser>(TEST_VCF_FILE_PATH);
    std::unique_ptr<IVCFDal> dal = std::make_unique<SQLiteVCFDal>(db_name, true);

    VCFProcessor processor (std::move(reader), std::move(parser), std::move(dal), 10);

    // Act
    processor.process();

    // Assert records where persisted and are fetched in order
    dal = std::make_unique<SQLiteVCFDal>(db_name);

    auto readRecords = dal->fetchAllVariants();

    ASSERT_TRUE(readRecords.size() == expectedRecords);

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

