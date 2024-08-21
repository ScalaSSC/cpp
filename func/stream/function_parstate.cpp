#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/serialization.h>
#include <iostream>
#include <map>
#include <stdio.h>
#include <string>
#include <vector>

// We must register the function_state in scheduler!

int main(int argc, char* argv[])
{
    printf("function partition state example\n");

    // get the inputMap (inputdata)
    std::vector<uint8_t> vec = faasm::getInputVec();
    // // printf the vector
    // printf("input data: ");
    // for (const auto& i : vec) {
    //     printf("%d ", i);
    // }
    // printf("\n read data finished. \n");

    size_t index = 0; // Reset index if reusing buffer
    std::map<std::string, std::map<std::string, std::string>> inputMap =
      faasm::deserializeNestedMap(vec, index);
    // printf the inputMap (inputdata)
    for (const auto& pair : inputMap) {
        printf("Key: %s Value: ", pair.first.c_str());
        for (const auto& innerPair : pair.second) {
            printf("%s %s ", innerPair.first.c_str(), innerPair.second.c_str());
        }
        printf("\n");
    }
    // get the functionstate
    size_t readSize = faasmReadFunctionStateSizeLock();
    std::map<std::string, std::vector<uint8_t>> functionState;
    printf("parstate readSize: %ld\n", readSize);
    if (readSize == 0) {
        printf("function state is still not initilized, initializing it\n");
        functionState["k2"] = { 2, 3, 4, 5, 6, 7, 8 };
        functionState["partitionStateKey"] = {};
    } else {
        printf("function state is already initialized\n");
        std::vector<uint8_t> stateBuffer(readSize);
        faasmReadFunctionState(stateBuffer.data(), readSize);
        functionState = faasm::deserializeFuncState(stateBuffer);
    }
    printf("read data finished. \n");

    // get the partitionstate from functionstate
    std::map<std::string, std::vector<uint8_t>> parFunctionState;
    if (functionState["partitionStateKey"].size() != 0) {
        parFunctionState =
          faasm::deserializeParState(functionState["partitionStateKey"]);
    }

    /*
    Begin the loop
    */

    // Print the size of inputMap
    printf("inputMap size: %ld\n", inputMap.size());
    for (size_t i = 0; i < inputMap.size(); i++) {
        // get the input for this spefic function invoke.
        std::string inputParStr = inputMap[std::to_string(i)]["partitionedAttribute"];
    
        // print the input par
        printf("the ith: %zu is inputParStr: %s\n", i, inputParStr.c_str());
        // increament the count
        int count = 0;
        if (parFunctionState.find(inputParStr) != parFunctionState.end()) {
            count = faasm::uint8VToUint32(parFunctionState[inputParStr]);
        }
        count++;
        parFunctionState[inputParStr] = faasm::uint32ToUint8V(count);
    }

    /*
    After the loop
    */
    functionState["partitionStateKey"] =
      faasm::serializeParState(parFunctionState);
    // write data back
    std::vector<uint8_t> functionStateBytes =
      faasm::serializeFuncState(functionState);
    printf("write back");
    faasmWriteFunctionStateUnlock(functionStateBytes.data(),
                                  functionStateBytes.size());

    printf("read again, and print the new data.");
    // Read it again and compare it.
    readSize = faasmReadFunctionStateSizeLock();
    functionState.clear();
    printf("parstate readSize: %ld\n", readSize);
    if (readSize == 0) {
        printf("error: function state is not written\n");
        faasmFunctionStateUnlock();
        return 1;
    } else {
        printf("function state is already initialized\n");
        std::vector<uint8_t> stateBuffer(readSize);
        faasmReadFunctionState(stateBuffer.data(), readSize);
        functionState = faasm::deserializeFuncState(stateBuffer);
    }

    // Print the function state
    printf("function state: \n");
    for (auto const& x : functionState) {
        std::cout << x.first << " : ";
        for (auto const& y : x.second) {
            std::cout << +y << " ";
        }
        std::cout << std::endl;
    }
    printf("partition state \n");
    // Print the partition state
    parFunctionState =
          faasm::deserializeParState(functionState["partitionStateKey"]);
    for (auto const& x : parFunctionState) {
        std::cout << x.first << " : ";
        for (auto const& y : x.second) {
            std::cout << +y << " ";
        }
        std::cout << std::endl;
    }

    faasmFunctionStateUnlock();
    return 0;
}
