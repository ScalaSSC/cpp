#include "faasm/core.h"
#include "faasm/faasm.h"
#include "faasm/input.h"
#include <faasm/serialization.h>
#include <iostream>
#include <map>
#include <random>
#include <stdio.h>
#include <string>
#include <vector>

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)
    std::vector<uint8_t> vec = faasm::getInputVec();
    size_t index = 0;
    std::map<std::string, std::map<std::string, std::string>> inputMap =
      faasm::deserializeNestedMap(vec, index);

    // Iterate over the input data and split each sentence
    for (int i = 0; i < inputMap.size(); i++) {
        // Split the inputSentence by space and store in a vector. Then chained
        // call next function.
        std::string inputSentence = inputMap[std::to_string(i)]["sentence"];
        // Print the inputSentence
        // printf("WordCount split: Received sentence: %s\n", inputSentence.c_str());
        
        std::string word;

        for (char x : inputSentence) {
            if (x == ' ') {
                if (!word.empty()) {
                    // Prepare for chain call
                    std::map<std::string, std::string> chainedInput;
                    chainedInput["partitionedAttribute"] = word;
                    std::vector<uint8_t> chainedInputBytes;
                    faasm::serializeMap(chainedInputBytes, chainedInput);
                    faasmChainNamedId("wordcountindiv_count",
                                      chainedInputBytes.data(),
                                      chainedInputBytes.size(),
                                      i);
                    word.clear();
                }
            } else {
                word = word + x;
            }
        }
        if (!word.empty()) {
            // Prepare for chain call
            std::map<std::string, std::string> chainedInput;
            chainedInput["partitionedAttribute"] = word;
            std::vector<uint8_t> chainedInputBytes;
            faasm::serializeMap(chainedInputBytes, chainedInput);

            faasmChainNamedId("wordcountindiv_count",
                              chainedInputBytes.data(),
                              chainedInputBytes.size(),
                              i);
        }
    }

    faasmChainInvoke();
    return 0;
}