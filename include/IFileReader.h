#pragma once
#include <vector>
#include <string>

class IFileReader {
public:

    /*
    * @brief reads the next batch of lines from the file, starting from the current position of the stream. It returns a vector of strings, where each string is a line from the file. If the end of the file is reached, it returns an empty vector.
    */
    virtual std::vector<std::string> getBatch() = 0;

    virtual ~IFileReader() = default;
};
