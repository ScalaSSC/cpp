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

    auto inputMap = faasm::getInputMap();

    // Iterate over the input data and split each sentence
    for (int i = 0; i < inputMap.size(); i++) {
        // Split the inputSentence by space and store in a vector. Then chained
        // call next function.
        std::string input = inputMap[std::to_string(i)]["input"];

        int count = input.length();

        std::map<std::string, std::string> chainedInput;
        chainedInput["input"] = input;
        chainedInput["count"] = std::to_string(count);
        std::vector<uint8_t> chainedInputBytes;
        faasm::serializeMap(chainedInputBytes, chainedInput);

        faasmChainNamedId(
          "lineten_four", chainedInputBytes.data(), chainedInputBytes.size(), i);
    }

    faasmChainInvoke();
    return 0;
}