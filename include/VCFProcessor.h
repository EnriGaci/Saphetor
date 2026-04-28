#pragma once
#include "IFileReader.h"
#include "ThreadPool.h"
#include "IParser.h"
#include "IVCFDal.h"

#include <string>
#include <memory>
#include <vector>

#define STORAGE_WRITER_THREAD_POOL_SIZE 1 // use 1 to avoid parallel insertion in the storage

class VCFProcessor
{
public:
    VCFProcessor(
        std::unique_ptr<IFileReader> fileReader,
        std::unique_ptr<IParser> parser,
        std::unique_ptr<IVCFDal> dal,
        int numThreads);

    /*
    * @brief Single threaded reads the file in batches, process each batch in a worker thread, once all workers are done, finalize the records
    */
    void process();

    /*
    * @brief Parse and store batch in staging, this method is called by the worker threads
    */
    void processBatch(const std::vector<std::string>& batch);

    ~VCFProcessor() = default;

private:
    ThreadPool mParseWorkersThreadPool; // thread pool for parsing and processing the data
    ThreadPool mStoreWorker{ STORAGE_WRITER_THREAD_POOL_SIZE }; // thread pool for storing the data 
    std::unique_ptr<IFileReader> mFileReader; 
    std::unique_ptr<IParser> mParser;
    std::unique_ptr<IVCFDal> mDal;
};

