#pragma once
#include <vector>
#include <string>

class IFileReader {
public:

    virtual std::vector<std::string> getBatch() = 0;

    virtual ~IFileReader() = default;
};
