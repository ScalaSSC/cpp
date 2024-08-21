#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/input.h>
#include <faasm/serialization.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// TODO - auto t0 = std::chrono::system_clock::now() might contains bug
// We cannot record time correctly inside the function. Always Overflow.

// We must register the function_state in scheduler!

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)
    std::vector<uint8_t> vec = faasm::getInputVec();

    size_t index = 0; // Reset index if reusing buffer
    std::map<std::string, std::map<std::string, std::string>> inputMap =
      faasm::deserializeNestedMap(vec, index);

    // concat the input string
    std::vector<std::string> inputKeys;
    for (size_t i = 0; i < inputMap.size(); i++) {
        // get the input for this spefic function invoke.
        std::string inputParStr =
          inputMap[std::to_string(i)]["partitionedAttribute"];
        inputKeys.push_back(inputParStr);
    }

    std::string inputKeysStr = faasm::concatInput(inputKeys);

    // get the functionstate
    size_t readSize =
      faasmReadPartitionedFunctionStateSizeLock(inputKeysStr.c_str());

    std::map<std::string, std::vector<uint8_t>> partitionedState;
    if (readSize != 0) {
        std::vector<uint8_t> stateBuffer(readSize);
        faasmReadPartitionedFunctionState(
          stateBuffer.data(), readSize, inputKeysStr.c_str());
        partitionedState = faasm::deserializeParState(stateBuffer);
    }

    /*
    Begin the loop
    */

    for (size_t i = 0; i < inputMap.size(); i++) {
        // get the input for this spefic function invoke.
        std::string inputParStr =
          inputMap[std::to_string(i)]["partitionedAttribute"];

        // increament the count
        int count = 0;
        if (partitionedState.find(inputParStr) != partitionedState.end()) {
            count = faasm::uint8VToUint32(partitionedState[inputParStr]);
        }
        count++;
        partitionedState[inputParStr] = faasm::uint32ToUint8V(count);
    }

    // for (const auto& pair : partitionedState) {
    //     std::cout << pair.first << ": ";
    //     int count = faasm::uint8VToUint32(pair.second);
    //     std::cout << count;
    //     std::cout << std::endl;
    // }

    /*
    After the loop
    */

    // write data back
    
    std::vector<uint8_t> partitionedStateBytes =
      faasm::serializeParState(partitionedState);
    faasmWritePartitionedFunctionStateUnlock(partitionedStateBytes.data(),
                                             partitionedStateBytes.size());

    return 0;
}
