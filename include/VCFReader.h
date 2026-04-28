#pragma once
#include <fstream>
#include <vector>

#include "IFileReader.h"
#include "ErrorHandler.h"

#define DEFAULT_BATCH_SIZE 100

class VCFReader : public IFileReader
{
public:

    /*
    * @brief opens the VCF file and prepares it for reading in batches. It also moves the stream to the beginning of the data section, skipping headers and meta-information lines.
    *
    * @param filePath The path to the VCF file to be read.
    * @param batchSize The number of lines to read in each batch. Default is 100.
    */
    VCFReader(const std::string& filePath, size_t batchSize = DEFAULT_BATCH_SIZE);

    /*
    * @brief reads the next batch of lines from the VCF file, starting from the current position of the stream. It returns a vector of strings, where each string is a line from the VCF file. If the end of the file is reached, it returns an empty vector.
    */
    std::vector<std::string> getBatch() override;

    ~VCFReader() = default;

private:
    /*
    * @brief Moves stream passed information fields
    */
    void moveStreamToDataBegin();

private:
    std::ifstream mFileStream;
    size_t mBatchSize;
};
