#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/input.h>
#include <faasm/serialization.h>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

// TODO - auto t0 = std::chrono::system_clock::now() might contains bug
// We cannot record time correctly inside the function. Always Overflow.

// We must register the function_state in scheduler!

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
            resultSet.insert(token);
        }
        start = end + delimiter_length;
    }

    // Add the last token if it's not empty
    std::string token = str.substr(start);
    if (!token.empty()) {
        resultSet.insert(token);
    }

    return resultSet;
}

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)
    std::vector<uint8_t> vec = faasm::getInputVec();

    size_t index = 0; // Reset index if reusing buffer
    std::map<std::string, std::map<std::string, std::string>> inputMap =
      faasm::deserializeNestedMap(vec, index);

    // concat the input string
    std::vector<std::string> todoKeys;
    std::map<std::string, int> todoKeysMap;
    for (size_t i = 0; i < inputMap.size(); i++) {
        // get the input for this spefic function invoke.
        std::string inputParStr =
          inputMap[std::to_string(i)]["partitionedAttribute"];
        if (todoKeysMap.find(inputParStr) != todoKeysMap.end()) {
            todoKeysMap[inputParStr]++;
        } else {
            todoKeysMap[inputParStr] = 1;
        }
    }

    uint64_t start = faasmGetMicros();

    // BEGIN the loop
    while (todoKeysMap.size() > 0) {
        todoKeys.clear();
        for (const auto& pair : todoKeysMap) {
            todoKeys.push_back(pair.first);
        }
        std::string todoKeysStr = faasm::concatInput(todoKeys);
        // Initialize the locked keys
        int lockedKeysSize = todoKeysStr.size() + 1;
        auto lockedKeys = new uint8_t[lockedKeysSize];
        // get the functionstate
        size_t readSize =
          faasmReadIndivFunctionStateSizeLock(todoKeysStr.c_str(), lockedKeys);

        std::string lockedKeysStr(reinterpret_cast<char*>(lockedKeys));
        auto lockedKeysSet = splitStringToSet(lockedKeysStr, "|");

        std::map<std::string, std::vector<uint8_t>> partitionedState;
        if (readSize != 0) {
            std::vector<uint8_t> stateBuffer(readSize);
            faasmReadIndivFunctionState(
              stateBuffer.data(), readSize, lockedKeysStr.c_str());
            partitionedState = faasm::deserializeParState(stateBuffer);
        }

        for (const std::string& key : lockedKeysSet) {
            int count = 0;
            if (partitionedState.find(key) != partitionedState.end()) {
                count = faasm::uint8VToUint32(partitionedState[key]);
            }
            count = count + todoKeysMap[key];
            todoKeysMap.erase(key);
            partitionedState[key] = faasm::uint32ToUint8V(count);
        }
        // write data back
        for (const auto& pair : partitionedState) {
            std::cout << pair.first << ": ";
            int count = faasm::uint8VToUint32(pair.second);
            std::cout << count;
            std::cout << std::endl;
        }
        std::vector<uint8_t> partitionedStateBytes =
          faasm::serializeParState(partitionedState);
        faasmWriteIndivFunctionStateUnlock(partitionedStateBytes.data(),
                                           partitionedStateBytes.size());
    }

    uint64_t end = faasmGetMicros();
    uint64_t diff = end - start;
    // Print start, end, and duration in microseconds

    int inputSize = inputMap.size();
    std::string output =
      "wordcount_count_lock_input_size: " + std::to_string(inputSize) + " and duration:" + std::to_string(diff);
    for (size_t i = 0; i < inputMap.size(); i++) {
        faasmSetOutputId(output.c_str(), output.size(), i);
    }
    // printf("finished");

    return 0;
}
