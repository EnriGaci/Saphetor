#pragma once
#include "IFileReader.h"
#include "ThreadPool.h"
#include "ErrorHandler.h"
#include "VCFData.h"
#include "Validators.h"
#include "IParser.h"
#include "IVCFDal.h"

#include <string>
#include <string_view>
#include <sstream>
#include <variant>
#include <fstream>

#define STORAGE_WRITER_THREAD_POOL_SIZE 1 // use 1 to avoid parallel insertion in the storage

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
                vcfDataBatch.push_back(std::move(data));
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
    ThreadPool mStoreWorker{ STORAGE_WRITER_THREAD_POOL_SIZE }; 
    std::unique_ptr<IFileReader> mFileReader;
    std::unique_ptr<IParser> mParser;
    std::unique_ptr<IVCFDal> mDal;
};

