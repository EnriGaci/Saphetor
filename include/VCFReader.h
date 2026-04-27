#pragma once
#include <fstream>
#include <vector>

#include "IFileReader.h"
#include "ErrorHandler.h"


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

    ~VCFReader() = default;

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
