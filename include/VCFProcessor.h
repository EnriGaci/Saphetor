#pragma once
#include "ThreadPool.h"
#include "ErrorHandler.h"

#include <fstream>
#include <string>
#include <string_view>

class IFileReader {
public:

    virtual std::vector<std::string> getBatch() = 0;

    virtual ~IFileReader() = default;
};

class VCFReader : public IFileReader
{
public:
    VCFReader(const std::string& filePath, size_t batchSize = 100) : mBatchSize(batchSize) {
        mFileStream = std::ifstream(filePath);

        if (!mFileStream.is_open()) {
            RaiseError(ErrorCodes::CouldNotOpenVCFFile, "Could not open file: " + filePath);
        }

        moveStreamToDataBegin();
    }

    std::vector<std::string> getBatch() override {
        // Read a batch of data from the file stream and return it
        size_t i = 0;
        std::vector<std::string> batch(mBatchSize);
        while (i < mBatchSize) {
            std::string line;
            if (!std::getline(mFileStream, line)) {
                break; // End of file or error
            }
            batch[i]=line;
            i++;
        }

        if (i != mBatchSize) {
            batch.resize(i);
        }

        return batch;
    }

private:
    // Moves stream passed information fields
    void moveStreamToDataBegin() {
        std::string line;
        while (std::getline(mFileStream, line)) {
            if (!line.empty() && line.size() > 1 && line[1] != '#') {
                break;
            }
        }
    }

private:
    std::ifstream mFileStream;
    size_t mBatchSize;
};

class VCFProcessor
{
public:
    VCFProcessor(std::unique_ptr<IFileReader> fileReader, int numThreads) : mFileReader(std::move(fileReader)), mThreadPool(numThreads)  { }

    void process() {
        // Implement VCF processing logic here

        auto batch = mFileReader->getBatch();

        while (!batch.empty()) {
            mThreadPool.enqueue([this, batch] {
                // Process the batch of data
                processBatch(batch);
                });
            batch = mFileReader->getBatch();
        }

    }

    virtual void processBatch(const std::vector<std::string>& batch) {
        // Implement the logic to process a batch of data
        return;

        //for (const auto& line : batch) {
        //    // Process each line of the batch
        //    //strore line
        //}
    }

    virtual ~VCFProcessor() = default;

private:
    ThreadPool mThreadPool;
    std::unique_ptr<IFileReader> mFileReader;
};

