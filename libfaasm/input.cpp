#include "faasm/input.h"
#include "faasm/core.h"
#include "faasm/serialization.h"

#include <stdint.h>
#include <string.h>
#include <string>
#include <stdexcept>

namespace faasm {
const char* getStringInput(const char* defaultValue)
{
    long inputSize = faasmGetInputSize();
    if (inputSize == 0) {
        return defaultValue;
    }

    auto inputBuffer = new uint8_t[inputSize + 1];
    faasmGetInput(inputBuffer, inputSize);

    // Force null-termination
    inputBuffer[inputSize] = 0;

    char* strIn = reinterpret_cast<char*>(inputBuffer);

    return strIn;
}

// We use Vec here since, sometimes input cannot cast to string
const std::vector<uint8_t> getInputVec()
{
    long inputSize = faasmGetInputSize();
    if (inputSize == 0) {
        return std::vector<uint8_t>();
    }
    std::vector<uint8_t> inputBuffer(inputSize);
    faasmGetInput(inputBuffer.data(), inputSize);

    // Convert to string if returning is necessary
    return inputBuffer;
}

const std::map<std::string, std::map<std::string, std::string>> getInputMap()
{
    // get the inputMap (inputdata)
    std::vector<uint8_t> vec = getInputVec();

    size_t index = 0; // Reset index if reusing buffer
    auto inputMap = faasm::deserializeNestedMap(vec, index);

    return inputMap;
}

int getIntInput()
{
    const char* inputStr = faasm::getStringInput("0");
    int intVal = std::stoi(inputStr);
    return intVal;
}

void setStringOutput(const char* val)
{
    faasmSetOutput(val, strlen(val));
}

int* parseStringToIntArray(const char* strIn, int nInts)
{
    char* strCopy = new char[strlen(strIn)];
    strcpy(strCopy, strIn);

    char* nextSubstr = strtok(strCopy, " ");
    int* result = new int[nInts];

    int i = 0;
    while (nextSubstr != NULL) {
        result[i] = std::stoi(nextSubstr);
        nextSubstr = strtok(NULL, " ");
        i++;
    }

    return result;
}

const std::string concatInput(const std::vector<std::string>& input)
{
    std::string result;
    bool first = true; // To avoid leading delimiter

    for (const auto& str : input) {
        if (str.find('|') != std::string::npos) {
            throw std::invalid_argument(
              "Input string contains an delimiter character: '|'");
        }

        if (!str.empty()) {
            if (!first) {
                result += '|';
            }
            result += str;
            first = false;
        }
    }
    return result;
}

// Function to split a string by a delimiter and store the elements in a set
std::set<std::string> splitStringToSet(const std::string& str,
                                       const std::string& delimiter)
{
    std::set<std::string> resultSet;
    std::size_t start = 0;
    std::size_t end;
    std::size_t delimiter_length = delimiter.length();

    while ((end = str.find(delimiter, start)) != std::string::npos) {
        std::string token = str.substr(start, end - start);
        if (!token.empty()) {
            resultSet.insert(std::move(token));
        }
        start = end + delimiter_length;
    }

    // Add the last token if it's not empty
    std::string token = str.substr(start);
    if (!token.empty()) {
        resultSet.insert(std::move(token));
    }

    // Return the set using std::move to avoid reconstruction
    return std::move(resultSet);
}

} // namespace faasm
