#include "Logging.h"
#include "VCFProcessor.h"

#include <string>
#include <string_view>
#include <sstream>
#include <variant>
#include <fstream>

VCFProcessor::VCFProcessor(std::unique_ptr<IFileReader> fileReader, std::unique_ptr<IParser> parser, std::unique_ptr<IVCFDal> dal, int numThreads) :
    mParseWorkersThreadPool(numThreads),
    mFileReader(std::move(fileReader)),
    mParser(std::move(parser)),
    mDal(std::move(dal))
{ }


void VCFProcessor::process() {
    LOG(LogLevel::INFO, "Starting to read file and dispatch batches to workers...");

    auto batch = mFileReader->getBatch();

    while (!batch.empty()) {
        mParseWorkersThreadPool.enqueue([this, batch] {
            // Process the batch of data
            processBatch(batch);
            });
        batch = mFileReader->getBatch();
    }

    LOG(LogLevel::INFO, "Main thread finished reading file and dispatching batches. Waiting for workers to finish...");

    // Wait for all parsing and persistance workers to finish before finalizing records
    mParseWorkersThreadPool.waitForTasksToFinish();
    mStoreWorker.waitForTasksToFinish();


    LOG(LogLevel::INFO, "All workers finished. Finalizing records...");
    mDal->finalizeRecords();
}

void VCFProcessor::processBatch(const std::vector<std::string>& batch) {
    LOG(LogLevel::INFO, std::ostringstream() << "Thread " << std::this_thread::get_id() << " starting batch parsing");

    std::vector<VCFData> vcfDataBatch;
    vcfDataBatch.reserve(batch.size());

    for (const auto& line : batch) {
        try {
            VCFData data = mParser->parse(line);
            vcfDataBatch.push_back(std::move(data));
        }
        catch (const std::exception& ex) {
            LOG(LogLevel::WARNING, "Error parsing line: " + line + ". Error: " + ex.what());
        }
    }

    mStoreWorker.enqueue([this, vcfDataBatch = std::move(vcfDataBatch)]() mutable {
        mDal->storeBatchInStaging(vcfDataBatch);
        });
}
