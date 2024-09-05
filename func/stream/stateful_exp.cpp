#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/input.h>
#include <faasm/serialization.h>
#include <faasm/state.h>
#include <faasm/time.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <chrono>

// We must register the function_state in scheduler!
std::vector<uint8_t> serialize(int value) {
    std::vector<uint8_t> serializedData(sizeof(int));
    for (size_t i = 0; i < sizeof(int); ++i) {
        serializedData[i] = (value >> (i * 8)) & 0xFF;
    }
    return serializedData;
}

int deserialize(const std::vector<uint8_t>& data) {
    int value = 0;
    for (size_t i = 0; i < sizeof(int); ++i) {
        value |= (static_cast<int>(data[i]) << (i * 8));
    }
    return value;
}

int main(int argc, char* argv[])
{
    uint64_t start = faasmGetMicros();
    // get the inputMap (inputdata)
    auto inputMap = faasm::getInputMap();

    // Get and initialize the Function
    size_t readSize = faasmReadFunctionStateSizeLock();
    int count;
    if (readSize == 0) {
        long count = 0;
    } else {
        std::vector<uint8_t> stateBytes(readSize);
        faasmReadFunctionState(stateBytes.data(), readSize);
        count = deserialize(stateBytes);
    }

    // std::cout << "input size is : " << inputMap.size() << std::endl;
    // Process each request
    for (int i = 0; i < inputMap.size(); i++) {
        count++;
        // std::cout << "count is : " << count << std::endl;
        int simulator = 0;
        for (int j = 0; j < 100000; j++) {
            simulator += j * i;
            simulator += i;
            simulator = simulator % (i + j +1) + 1;
        }
        printf("simulator is : %d\n", simulator);
    }

    std::vector<uint8_t> stateBytes = serialize(count);
    faasmWriteFunctionStateUnlock(stateBytes.data(), stateBytes.size());
    // Record the end time
    uint64_t end = faasmGetMicros();
    uint64_t diff = end - start;
    // Print start, end, and duration in microseconds
    std::string output = "stateful_exp_duration:" + std::to_string(diff);
    faasmSetOutputId(output.c_str(), output.size(), 0);
    return 0;
}
