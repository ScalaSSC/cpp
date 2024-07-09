#include "faasm/input.h"
#include "faasm/core.h"

#include <stdint.h>
#include <string.h>
#include <string>

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

} // namespace faasm
