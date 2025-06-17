#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/input.h>
#include <faasm/serialization.h>

#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// TODO - auto t0 = std::chrono::system_clock::now() might contains bug
// We cannot record time correctly inside the function. Always Overflow.

// We must register the function_state in scheduler!

// Serializes a uint32_t into a vector of 4 bytes
std::vector<uint8_t> serialize(uint32_t value)
{
    return faasm::uint32ToUint8V(value);
}

// Deserializes a vector of 4 bytes into a uint32_t
uint32_t deserialize(const std::vector<uint8_t>& bytes)
{
    return faasm::uint8VToUint32(bytes);
}

int main(int argc, char* argv[])
{
    auto inputMap = faasm::getInputMap();
    // <partitioned Attribute, <msgIdx, inputTuple>>
    auto todoKeysMap =
      faasm::generateTodoKeysMap(inputMap, "word");

    auto initState =
      [](const std::string& key,
         const std::map<std::string, std::vector<uint8_t>>& partitionedState)
      -> int {
        int count = 0;
        if (partitionedState.find(key) != partitionedState.end()) {
            count = deserialize(partitionedState.at(key));
        }
        return count;
    };

    auto processOperator =
      [](const std::string& key,
         const std::tuple<size_t, std::map<std::string, std::string>>& todoData,
         int& state) {
          state++;
          // std::ostringstream oss;
          // oss << "New statistics: key: " << key << " count: " << state;
          // std::cout << oss.str() << std::endl;
      };

    // uint64_t start = faasmGetMicros();

    faasm::processTodoMap<int>(
      todoKeysMap, processOperator, initState, serialize);

    // uint64_t end = faasmGetMicros();
    // uint64_t diff = end - start;
    // Print start, end, and duration in microseconds

    // int inputSize = inputMap.size();
    // std::string output =
    //   "wordcount_count_lock_input_size: " + std::to_string(inputSize) +
    //   " and duration:" + std::to_string(diff);
    // for (size_t i = 0; i < inputMap.size(); i++) {
    //     faasmSetOutputId(output.c_str(), output.size(), i);
    // }
    // printf("finished");

    return 0;
}
